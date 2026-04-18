#include <Arduino.h>
#include <SensirionI2CSen5x.h>
#include <Wire.h>
#include <ESPSupabase.h>
#include <WiFi.h>
#include "config.h"
#include <ArduinoJson.h>

#define SDA_PIN 6
#define SCL_PIN 7
#define SENSOR_READING_TIMEOUT 10 //Amount of time (in seconds) software will attempt to read sensor before timing out

struct sensor_reading_t {
    float pm1;
    float pm2;
    float pm4;
    float pm10;
    float humidity;
    float temp;
    float voc; 
    float nox;
    String captured_at; // Added for timestamping
};

enum measurement_err_t {
    MEAS_OK,
    MEAS_FAIL
};

static int sampleCount = 0;
static sensor_reading_t sensor_readings[SAMPLES_PER_TRANSMISSION] = {};

static SensirionI2CSen5x sen5x;
static struct sensor_reading_t sensor_reading;
static Supabase db;
static String macAddress;

void init_sen5x() {
    sen5x.begin(Wire);

    uint16_t error;
    char errorMessage[256];
    error = sen5x.deviceReset();
    if (error) {
        Serial.print("Error trying to execute deviceReset(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }

    float tempOffset = 0.0;
    error = sen5x.setTemperatureOffsetSimple(tempOffset);
    if (error) {
        Serial.print("Error trying to execute setTemperatureOffsetSimple(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else {
        Serial.print("Temperature Offset set to ");
        Serial.print(tempOffset);
        Serial.println(" deg. Celsius (SEN54/SEN55 only)");
    }

    // Start Measurement
    error = sen5x.startMeasurement();
    if (error) {
        Serial.print("Error trying to execute startMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }
}

measurement_err_t measure(sensor_reading_t *sensor_reading) {
    uint16_t error;
    char errorMessage[256];

    // Wait for data to be ready (up to SENSOR_READING_TIMEOUT seconds)
    bool dataReady = false;
    Serial.println("Waiting for sensor data...");
    
    for (int i = 0; i < SENSOR_READING_TIMEOUT * 100; i++) {
        error = sen5x.readDataReady(dataReady);
        if (error) {
            Serial.print("Error trying to execute startMeasurement(): ");
            errorToString(error, errorMessage, 256);
            Serial.println(errorMessage);
            dataReady = false;
        }

        if(dataReady) {
            Serial.println("Sensor Data Ready!!");
            delay(10);
            break;
        }
        delay(10);
    }
    if(!dataReady) {
        Serial.printf("Sensor measurement timeout exceeded after %u seconds", SENSOR_READING_TIMEOUT);
        return MEAS_FAIL;
    };

    // Now read the measurement
    float massConcentrationPm1p0;
    float massConcentrationPm2p5;
    float massConcentrationPm4p0;
    float massConcentrationPm10p0;
    float ambientHumidity;
    float ambientTemperature;
    float vocIndex;
    float noxIndex;

    error = sen5x.readMeasuredValues(
        massConcentrationPm1p0, massConcentrationPm2p5, massConcentrationPm4p0,
        massConcentrationPm10p0, ambientHumidity, ambientTemperature, vocIndex,
        noxIndex);

    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        sensor_reading->captured_at = ""; // Or handle as null
    } else {
        char timeString[64];
        // Format for Postgres: YYYY-MM-DD HH:MM:SS
        strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
        sensor_reading->captured_at = String(timeString);
    }

    if (error) {
        Serial.print("Error trying to execute readMeasuredValues(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
        // Log the specific sensor error before returning
        log_event("SENSOR_ERROR", "CRITICAL", "Failed to read from SEN5x");
        return MEAS_FAIL;
    } else { 
        sensor_reading->pm1 = massConcentrationPm1p0;
        sensor_reading->pm2 = massConcentrationPm2p5;
        sensor_reading->pm4 = massConcentrationPm4p0;
        sensor_reading->pm10 = massConcentrationPm10p0;
        sensor_reading->humidity = ambientHumidity;
        sensor_reading->temp = ambientTemperature;
        sensor_reading->voc = vocIndex;
        sensor_reading->nox = noxIndex;

        return MEAS_OK;
    }
}

void init_wifi() {
    Serial.print("Connecting to WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(100);
        Serial.print(".");
    }

    macAddress = WiFi.macAddress();
    Serial.println("\nConnected!");
}

void init_supabase() {
    db.begin(SUPABASE_URL, SUPABASE_ANON_KEY);
    db.urlQuery_reset();
    db.login_email(EMAIL, PASSWORD);
}

void register_node() {
    db.urlQuery_reset(); // Start clean
    Serial.println("Checking for existing node...");
    
    String read = db.from(NODES_TABLE).select("mac_address").eq("mac_address", macAddress).doSelect();
    
    // Check if the response is "[]" (empty array) or actually empty
    if(read == "[]" || read == "") {
        Serial.println("Mac address doesn't exist. Registering...");
        
        JsonDocument doc;
        doc["mac_address"] = macAddress; // Match your DB column name
        doc["sensor_model"] = SENSOR_MODEL;
        doc["firmware_version"] = FIRMWARE_VERSION;
        
        String json;
        serializeJson(doc, json);
        
        // Use your insert method
        db.insert(NODES_TABLE, json, false); 
    } else {
        Serial.println("Node already registered.");
    }
    
    db.urlQuery_reset();
}

void insert_sensor_reading() {
    db.urlQuery_reset();
    
    // DEBUG: Check values before putting them in JSON
    Serial.print("Debug - PM1 value: "); Serial.println(sensor_reading.pm1);
    Serial.print("Debug - MAC: "); Serial.println(WiFi.macAddress());

    JsonDocument doc; // Ensure you are using ArduinoJson 7

    doc["node_id"] = macAddress;
    doc["pm1_0"] = sensor_reading.pm1;
    doc["pm2_5"] = sensor_reading.pm2;
    doc["pm4_0"] = sensor_reading.pm4;
    doc["pm10_0"] = sensor_reading.pm10;
    doc["humidity"] = sensor_reading.humidity;
    doc["temperature"] = sensor_reading.temp;
    doc["voc_index"] = sensor_reading.voc;
    doc["nox_index"] = sensor_reading.nox;
    
    // Check if timestamp is actually set
    if (sensor_reading.captured_at.length() > 0) {
        doc["captured_at"] = sensor_reading.captured_at;
    }

    String json;
    serializeJson(doc, json);

    // DEBUG: This will tell us if the JSON was actually built
    Serial.print("Built JSON: ");
    Serial.println(json);

    if (json == "{}") {
        Serial.println("CRITICAL ERROR: JSON is empty! Check ArduinoJson version or memory.");
        return; 
    }

    db.insert(MEASUREMENTS_TABLE, json, false);
    db.urlQuery_reset();
}

void log_event(String type, String sev, String msg) {
    db.urlQuery_reset();
    JsonDocument doc;
    
    doc["node_id"] = macAddress;
    doc["event_type"] = type;
    doc["severity"] = sev;
    doc["message"] = msg;
    
    String json;
    serializeJson(doc, json);
    
    Serial.printf("Logging [%s]: %s\n", sev.c_str(), msg.c_str());
    db.insert(LOGS_TABLE, json, false);
    db.urlQuery_reset();
}

void setup() {
    /* Initialize Serial bus*/
    Serial.begin(115200);
    while (!Serial) {
        delay(100);
    }
    /* Initialize I2C */
    Wire.begin(SDA_PIN, SCL_PIN);
    delay(100);
    /* Initialize sen5x */
    init_sen5x();
    delay(100);
    /* Initialize WiFi */
    init_wifi();
    // This string handles the EST/EDT switch automatically for the US East Coast
    configTime(-18000, 3600, "pool.ntp.org"); 
    
    init_supabase();
    register_node();

    log_event("BOOT", "INFO", "System initialized in EST");
}

void loop() {
    measurement_err_t measure_err = measure(&sensor_reading);
    
    if (measure_err == MEAS_OK) {
        Serial.println("Measurement successful. Uploading...");
        insert_sensor_reading();
    } else {
        Serial.println("Measurement failed. Logging error to database...");
        
        log_event("SENSOR_READ_ERROR", "CRITICAL", "SEN5x timed out or failed to return data.");
        
        delay(5000); 
    }

    Serial.println("--------------------------------------------------------------------");
    
    delay(MEASUREMENT_INTERVAL * 1000);
}