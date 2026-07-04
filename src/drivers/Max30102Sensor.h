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
  uint32_t latestIr() const;
  bool fingerDetected() const;

 private:
  static constexpr uint8_t RATE_BUFFER_SIZE = 4;

  MAX30105 sensor_;
  SemaphoreHandle_t busMutex_ = nullptr;
  bool initialized_ = false;
  uint32_t lastSampleMs_ = 0;
  uint32_t lastBeatMs_ = 0;
  uint32_t latestIr_ = 0;
  uint8_t rates_[RATE_BUFFER_SIZE] = {0};
  uint8_t rateSpot_ = 0;
  uint8_t validRateCount_ = 0;
  int latestHeartRate_ = 0;
  bool fingerDetected_ = false;
};
