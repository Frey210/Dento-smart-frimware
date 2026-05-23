#include <Arduino.h>

#include "config/AppConfig.h"
#include "debug/DebugHelpers.h"
#include "drivers/Hx710bSensor.h"

namespace {
Hx710bSensor pressureSensor(AppConfig::PIN_HX710B_SCK, AppConfig::PIN_HX710B_DOUT);
uint32_t lastPrintMs = 0;
}

void setup() {
  DebugHelpers::waitForSerial();
  pressureSensor.begin();

  Serial.println("HX710B debug mode");
  Serial.printf("SCK=%u DOUT=%u\r\n", AppConfig::PIN_HX710B_SCK, AppConfig::PIN_HX710B_DOUT);
  DebugHelpers::printDivider();
}

void loop() {
  const uint32_t nowMs = millis();
  if ((nowMs - lastPrintMs) >= 500UL) {
    if (!pressureSensor.isReady()) {
      Serial.printf("[%10lu ms] sensor not ready\r\n", nowMs);
    } else {
      const int32_t raw = pressureSensor.readRaw();
      const float mmhg = pressureSensor.readPressureMmhg();
      Serial.printf("[%10lu ms] raw=%ld pressure=%.2f mmHg\r\n", nowMs, static_cast<long>(raw),
                    mmhg);
    }
    lastPrintMs = nowMs;
  }
  delay(20);
}
