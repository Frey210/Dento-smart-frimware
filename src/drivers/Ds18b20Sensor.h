#pragma once

#include <Arduino.h>
#include <DallasTemperature.h>
#include <OneWire.h>

class Ds18b20Sensor {
 public:
  explicit Ds18b20Sensor(uint8_t pin);
  void begin();
  void update(uint32_t nowMs);
  float latestCelsius() const;
  bool hasValidReading() const;
  uint8_t deviceCount() const;

 private:
  OneWire oneWire_;
  DallasTemperature sensorBus_;
  uint32_t lastRequestMs_ = 0;
  bool conversionPending_ = false;
  bool hasValidReading_ = false;
  uint8_t deviceCount_ = 0;
  float latestCelsius_ = 0.0f;
};
