#include <Arduino.h>

#include "config/AppConfig.h"
#include "debug/DebugHelpers.h"
#include "drivers/GsrSensor.h"

namespace {
GsrSensor gsrSensor(AppConfig::PIN_GSR);
uint32_t lastPrintMs = 0;
}

void setup() {
  DebugHelpers::waitForSerial();
  analogReadResolution(12);
  gsrSensor.begin();

  Serial.println("GSR debug mode");
  Serial.printf("GPIO %u\r\n", AppConfig::PIN_GSR);
  DebugHelpers::printDivider();
}

void loop() {
  const uint32_t nowMs = millis();
  if ((nowMs - lastPrintMs) >= 500UL) {
    const int raw = gsrSensor.readRaw();
    const float normalized = gsrSensor.readNormalized();
    Serial.printf("[%10lu ms] raw=%4d normalized=%.4f\r\n", nowMs, raw, normalized);
    lastPrintMs = nowMs;
  }
  delay(20);
}
