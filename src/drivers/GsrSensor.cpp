#include "drivers/GsrSensor.h"

namespace {
constexpr uint8_t GSR_SAMPLE_COUNT = 8;
constexpr uint16_t ADC_MAX_VALUE = 4095;
}

GsrSensor::GsrSensor(uint8_t analogPin) : pin_(analogPin) {}

void GsrSensor::begin() {
  pinMode(pin_, INPUT);
#if defined(ARDUINO_ARCH_ESP32)
  analogSetPinAttenuation(pin_, ADC_11db);
#endif
}

int GsrSensor::readRaw() const {
  uint32_t total = 0;
  for (uint8_t i = 0; i < GSR_SAMPLE_COUNT; ++i) {
    total += analogRead(pin_);
    delayMicroseconds(250);
  }
  return static_cast<int>(total / GSR_SAMPLE_COUNT);
}

float GsrSensor::readNormalized() const {
  const int raw = readRaw();
  return static_cast<float>(raw) / static_cast<float>(ADC_MAX_VALUE);
}
