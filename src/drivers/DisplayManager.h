#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

#include "models/SensorData.h"

class DisplayManager {
 public:
  DisplayManager();
  bool begin(uint8_t i2cAddress);
  bool isReady() const;
  void renderMenu(uint8_t selectedIndex);
  void renderLiveMonitor(const SensorSnapshot& snapshot, float anxietyScore);
  void renderBpProgress(BpState state, uint8_t progress, float pressureMmhg,
                        const BloodPressureReading& reading);
  void renderWifiStatus(const String& status, bool connected, bool portalActive, const String& ssid,
                        const String& ipAddress, const String& portalName);
  void renderMessage(const String& title, const String& body);

 private:
  void drawHeader_(const char* title);
  void drawFooterHint_(const char* hint);
  void drawWrappedText_(int16_t x, int16_t y, uint8_t maxCharsPerLine, const String& text);
  const char* bpStateLabel_(BpState state) const;

  U8G2_SH1106_128X64_NONAME_F_HW_I2C display_;
  bool ready_ = false;
};
