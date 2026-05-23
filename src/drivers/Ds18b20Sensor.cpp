#include "drivers/Ds18b20Sensor.h"

#include "config/AppConfig.h"

Ds18b20Sensor::Ds18b20Sensor(uint8_t pin) : oneWire_(pin), sensorBus_(&oneWire_) {}

void Ds18b20Sensor::begin() {
  sensorBus_.begin();
  sensorBus_.setWaitForConversion(false);
  sensorBus_.requestTemperatures();
  conversionPending_ = true;
  lastRequestMs_ = millis();
}

void Ds18b20Sensor::update(uint32_t nowMs) {
  if (conversionPending_ && (nowMs - lastRequestMs_) >= 800UL) {
    latestCelsius_ = sensorBus_.getTempCByIndex(0);
    sensorBus_.requestTemperatures();
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

