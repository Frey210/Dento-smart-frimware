#include "drivers/Ds18b20Sensor.h"

#include "config/AppConfig.h"
#include "utils/SerialLog.h"

namespace {
bool isValidTemperature(float celsius) {
  return celsius != DEVICE_DISCONNECTED_C && celsius > -55.0f && celsius < 125.0f;
}
}

Ds18b20Sensor::Ds18b20Sensor(uint8_t pin) : oneWire_(pin), sensorBus_(&oneWire_) {}

void Ds18b20Sensor::begin() {
  sensorBus_.begin();
  deviceCount_ = sensorBus_.getDeviceCount();
  LOGI("DS18B20 devices found=%u", static_cast<unsigned>(deviceCount_));
  sensorBus_.setWaitForConversion(false);
  sensorBus_.requestTemperatures();
  conversionPending_ = true;
  lastRequestMs_ = millis();
}

void Ds18b20Sensor::update(uint32_t nowMs) {
  if (conversionPending_ && (nowMs - lastRequestMs_) >= 800UL) {
    const float celsius = sensorBus_.getTempCByIndex(0);
    if (isValidTemperature(celsius)) {
      latestCelsius_ = celsius;
      hasValidReading_ = true;
    } else {
      hasValidReading_ = false;
      deviceCount_ = sensorBus_.getDeviceCount();
    }
    conversionPending_ = false;
    sensorBus_.requestTemperatures();
    conversionPending_ = true;
    lastRequestMs_ = nowMs;
    return;
  }

  if (!conversionPending_ || (nowMs - lastRequestMs_) >= AppConfig::DS18B20_PERIOD_MS) {
    sensorBus_.requestTemperatures();
    conversionPending_ = true;
    lastRequestMs_ = nowMs;
  }
}

float Ds18b20Sensor::latestCelsius() const { return latestCelsius_; }

bool Ds18b20Sensor::hasValidReading() const { return hasValidReading_; }

uint8_t Ds18b20Sensor::deviceCount() const { return deviceCount_; }
