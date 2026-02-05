#include <Arduino.h>
#include <SensirionI2CSen5x.h>
#include <Wire.h>
#include <ESPSupabase.h>
#include <WiFi.h>
#include "config.h"
#include <ArduinoJson.h>

#define BATPIN 2

struct sensor_reading_t {
    float pm1;
    float pm2;
    float pm4;
    float pm10;
    float humidity;
    float temp;
    float voc;
};

RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR int sampleCount = 0;
RTC_DATA_ATTR sensor_reading_t sensor_readings[SAMPLES_PER_TRANSMISSION] = {};

SensirionI2CSen5x sen5x;

void init_sen54() {
    sen5x.begin(Wire);

    uint16_t error;
    char errorMessage[256];
    error = sen5x.deviceReset();
    if (error) {}

    float tempOffset = 0.0;
    error = sen5x.setTemperatureOffsetSimple(tempOffset);
    if (error) {} 

    // Start Measurement
    error = sen5x.startMeasurement();
    if (error) {}
}

void measure(sensor_reading_t* sensor_reading) {
    uint16_t error;
    char errorMessage[256];

    // Wait for data to be ready (up to 10 seconds)
    bool dataReady = false;
    
    for (int i = 0; i < 100; i++) {
        delay(100);
        error = sen5x.readDataReady(dataReady);
        if (error) {}

        if(dataReady) {
            delay(1000);
            break;
        }
    }

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

    if (error) {} 
    else { 
        sensor_reading->pm1 = massConcentrationPm1p0;
        sensor_reading->pm2 = massConcentrationPm2p5;
        sensor_reading->pm4 = massConcentrationPm4p0;
        sensor_reading->pm10 = massConcentrationPm10p0;
        sensor_reading->humidity = ambientHumidity;
        sensor_reading->temp = ambientTemperature;
        sensor_reading->voc = vocIndex;
    }
}

void init_wifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
      delay(100);
  }
}

void setup() {
  ++bootCount;
  pinMode(BATPIN, INPUT);

  /*-------- Initiate I2C and Sen54 Air quality sensor --------*/ 
  Wire.begin(6,7);
  init_sen54();

  /*-------- Take a measurement --------*/ 
  sensor_reading_t sensor_reading = {};
  measure(&sensor_reading);
  sensor_readings[sampleCount] = sensor_reading;
  sampleCount++;

  /*-------- Take a measurement --------*/ 
    uint32_t millivolts = 3700;
    float voltage = (millivolts / 1000.0) * 3;

  /*-------- Decide if its time to send data over wifi --------*/ 
  if ( bootCount % SAMPLES_PER_TRANSMISSION == 0 ) {
    init_wifi();
    
    /* -- Initialize database -- */
    Supabase db;
    db.begin(SUPABASE_URL, SUPABASE_ANON_KEY);
    db.urlQuery_reset();
    db.login_email(EMAIL, PASSWORD);
    
    String json;
    JsonDocument doc;
    doc["pm1"] = sensor_reading.pm1;
    doc["pm2p5"] = sensor_reading.pm2;
    doc["pm4"] = sensor_reading.pm4;
    doc["pm10"] = sensor_reading.pm10;
    doc["ambienthumidity"] = sensor_reading.humidity;
    doc["ambienttemp"] = sensor_reading.temp;
    doc["vocindex"] = sensor_reading.voc;
    doc["battvoltage"] = voltage;
    doc["bootcount"] = bootCount;
    serializeJson(doc, json);

    int code = db.insert(DB_TABLE, json, false);
    db.urlQuery_reset();

    sampleCount = 0; // Reset sample count
  } 
  /*-------- Configure the wake up source to a timer --------*/
  esp_sleep_enable_timer_wakeup(SLEEP_INTERVAL * 1000000); // Time(s) * Conversion factor for seconds to micro seconds

  /*-------- Enter Deep Sleep --------*/ 
  delay(500);
  Serial.flush(); 
  esp_deep_sleep_start();
}

void loop() {
}