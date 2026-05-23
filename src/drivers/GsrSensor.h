#pragma once

#include <Arduino.h>

class GsrSensor {
 public:
  explicit GsrSensor(uint8_t analogPin);
  void begin();
  float readNormalized() const;

 private:
  uint8_t pin_;
};

