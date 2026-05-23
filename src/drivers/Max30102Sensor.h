#pragma once

#include <Arduino.h>
#include <MAX30105.h>
#include <Wire.h>
#include <freertos/semphr.h>

class Max30102Sensor {
 public:
  bool begin(TwoWire& wire);
  void setBusMutex(SemaphoreHandle_t mutex);
  void update(uint32_t nowMs);
  int latestHeartRate() const;

 private:
  bool detectBeat_(uint32_t irValue);

  MAX30105 sensor_;
  SemaphoreHandle_t busMutex_ = nullptr;
  bool initialized_ = false;
  uint32_t lastSampleMs_ = 0;
  uint32_t lastBeatMs_ = 0;
  uint32_t previousIr_ = 0;
  int latestHeartRate_ = 0;
};
