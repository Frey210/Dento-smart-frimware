#pragma once

#include <Arduino.h>

class Hx710bSensor {
 public:
  Hx710bSensor(uint8_t clockPin, uint8_t dataPin);
  void begin();
  bool isReady() const;
  int32_t readRaw();
  bool tryReadRaw(int32_t& valueOut);
  float readPressureMmhg();
  bool tryReadPressureMmhg(float& pressureOut);

 private:
  uint8_t clockPin_;
  uint8_t dataPin_;
  int32_t offset_ = 0;
  float scale_ = 12000.0f;
};
