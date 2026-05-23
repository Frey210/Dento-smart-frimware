#pragma once

#include <Arduino.h>

#include "config/AppConfig.h"

namespace DebugHelpers {
inline void waitForSerial() {
  Serial.begin(AppConfig::SERIAL_BAUD);
  const uint32_t serialWaitStarted = millis();
  while (!Serial && (millis() - serialWaitStarted) < 3000UL) {
    delay(10);
  }
  delay(200);
}

inline void printDivider() {
  Serial.println("----------------------------------------");
}
}  // namespace DebugHelpers
