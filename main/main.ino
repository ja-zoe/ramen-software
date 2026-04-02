#include <Arduino.h>
#include <SensirionI2CSen5x.h>
#include <Wire.h>
#include <ESPSupabase.h>
#include <WiFi.h>
#include "config.h"
#include <ArduinoJson.h>

struct sensor_reading_t {
    float pm1;
    float pm2;
    float pm4;
    float pm10;
    float humidity;
    float temp;
    float voc; 
};

RTC_DATA_ATTR int sampleCount = 0;
RTC_DATA_ATTR sensor_reading_t sensor_readings[SAMPLES_PER_TRANSMISSION] = {};

SensirionI2CSen5x sen5x;

void init_sen54() {
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

void measure(sensor_reading_t* sensor_reading) {
    uint16_t error;
    char errorMessage[256];

    // Wait for data to be ready (up to 10 seconds)
    bool dataReady = false;
    Serial.println("Waiting for sensor data...");
    
    for (int i = 0; i < 100; i++) {
        delay(100);
        error = sen5x.readDataReady(dataReady);
        if (error) {
            Serial.print("Error trying to execute startMeasurement(): ");
            errorToString(error, errorMessage, 256);
            Serial.println(errorMessage);
        }

        if(dataReady) {
            Serial.println("Sensor Data Ready!!");
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

    if (error) {
        Serial.print("Error trying to execute readMeasuredValues(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else { 
        sensor_reading->pm1 = massConcentrationPm1p0;
        sensor_reading->pm2 = massConcentrationPm2p5;
        sensor_reading->pm4 = massConcentrationPm4p0;
        sensor_reading->pm10 = massConcentrationPm10p0;
        sensor_reading->humidity = ambientHumidity;
        sensor_reading->temp = ambientTemperature;
        sensor_reading->voc = vocIndex;
        
        Serial.println("PM2.5: " + String(massConcentrationPm2p5));
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

void setup() {
}

void loop() {
}