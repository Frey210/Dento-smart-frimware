#include "drivers/GsrSensor.h"

GsrSensor::GsrSensor(uint8_t analogPin) : pin_(analogPin) {}

void GsrSensor::begin() { pinMode(pin_, INPUT); }

float GsrSensor::readNormalized() const {
  const int raw = analogRead(pin_);
  return static_cast<float>(raw) / 4095.0f;
}

