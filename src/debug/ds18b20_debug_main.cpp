#include <Arduino.h>

#include "config/AppConfig.h"
#include "debug/DebugHelpers.h"
#include "drivers/Ds18b20Sensor.h"

namespace {
Ds18b20Sensor ds18b20Sensor(AppConfig::PIN_DS18B20);
uint32_t lastPrintMs = 0;
}

void setup() {
  DebugHelpers::waitForSerial();
  ds18b20Sensor.begin();

  Serial.println("DS18B20 debug mode");
  Serial.printf("GPIO %u\r\n", AppConfig::PIN_DS18B20);
  Serial.println("Jika sensor tidak terbaca, nilai biasanya -127.00 C.");
  DebugHelpers::printDivider();
}

void loop() {
  const uint32_t nowMs = millis();
  ds18b20Sensor.update(nowMs);

  if ((nowMs - lastPrintMs) >= 1000UL) {
    Serial.printf("[%10lu ms] devices=%u valid=%d temperature=%.2f C\r\n", nowMs,
                  ds18b20Sensor.deviceCount(), ds18b20Sensor.hasValidReading(),
                  ds18b20Sensor.latestCelsius());
    lastPrintMs = nowMs;
  }
  delay(20);
}
