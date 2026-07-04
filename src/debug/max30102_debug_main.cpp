#include <Arduino.h>
#include <Wire.h>

#include "config/AppConfig.h"
#include "debug/DebugHelpers.h"
#include "drivers/Max30102Sensor.h"

namespace {
Max30102Sensor max30102Sensor;
uint32_t lastPrintMs = 0;
bool sensorReady = false;
}

void setup() {
  DebugHelpers::waitForSerial();
  Wire.begin(AppConfig::PIN_I2C_SDA, AppConfig::PIN_I2C_SCL, AppConfig::I2C_FREQUENCY);
  sensorReady = max30102Sensor.begin(Wire);

  Serial.println("MAX30102 debug mode");
  Serial.printf("SDA=%u SCL=%u initialized=%d\r\n", AppConfig::PIN_I2C_SDA, AppConfig::PIN_I2C_SCL,
                sensorReady);
  Serial.println("Letakkan jari pada sensor untuk pembacaan heart rate.");
  DebugHelpers::printDivider();
}

void loop() {
  const uint32_t nowMs = millis();
  if (sensorReady) {
    max30102Sensor.update(nowMs);
  }

  if ((nowMs - lastPrintMs) >= 500UL) {
    Serial.printf("[%10lu ms] ir=%lu finger=%d heart_rate=%d bpm\r\n", nowMs,
                  static_cast<unsigned long>(max30102Sensor.latestIr()),
                  max30102Sensor.fingerDetected(),
                  max30102Sensor.latestHeartRate());
    lastPrintMs = nowMs;
  }
  delay(20);
}
