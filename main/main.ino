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
    float pm2f
    float pm4;
    float pm10;
    float humidity;
    float temp;
    float voc; 
    float nox
};

enum measurement_err_t {
    MEAS_OK,
    MEAS_FAIL
};

RTC_DATA_ATTR int sampleCount = 0;
RTC_DATA_ATTR sensor_reading_t sensor_readings[SAMPLES_PER_TRANSMISSION] = {};

SensirionI2CSen5x sen5x;
static struct sensor_reading_t sensor_reading;

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

    if (error) {
        Serial.print("Error trying to execute readMeasuredValues(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
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
    Serial.println("\nConnected!");
}

void init_db() {
    Supabase db;
    db.begin(SUPABASE_URL, SUPABASE_ANON_KEY);
    db.urlQuery_reset();
    db.login_email(EMAIL, PASSWORD);
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
}

void loop() {
    static measurement_err_t = measure(&sensor_reading);
    Serial.printf("Humidity: %f \nTemp: %f \nVOC: %f \nNOX: %f \nPM1: %f \nPM2: %f \nPM4: %f \nPM10: %f\n", sensor_reading.humidity, sensor_reading.temp, sensor_reading.voc, sensor_reading.nox, sensor_reading.pm1, sensor_reading.pm2, sensor_reading.pm4, sensor_reading.pm10);
    Serial.println("--------------------------------------------------------------------");
}