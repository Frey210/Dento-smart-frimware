#include "drivers/Max30102Sensor.h"

#include "config/AppConfig.h"
#include "utils/SerialLog.h"

bool Max30102Sensor::begin(TwoWire& wire) {
  if (busMutex_ != nullptr && xSemaphoreTake(busMutex_, pdMS_TO_TICKS(100)) != pdTRUE) {
    LOGE("MAX30102 begin failed: I2C mutex timeout");
    initialized_ = false;
    return false;
  }

  const bool ok = sensor_.begin(wire, I2C_SPEED_FAST);
  if (busMutex_ != nullptr) {
    xSemaphoreGive(busMutex_);
  }

  if (!ok) {
    LOGW("MAX30102 not detected");
    initialized_ = false;
    return false;
  }

  sensor_.setup();
  sensor_.setPulseAmplitudeRed(0x2F);
  sensor_.setPulseAmplitudeGreen(0x00);
  initialized_ = true;
  LOGI("MAX30102 initialized");
  return true;
}

void Max30102Sensor::setBusMutex(SemaphoreHandle_t mutex) { busMutex_ = mutex; }

void Max30102Sensor::update(uint32_t nowMs) {
  if (!initialized_ || (nowMs - lastSampleMs_) < AppConfig::HEART_RATE_PERIOD_MS) {
    return;
  }

  lastSampleMs_ = nowMs;
  if (busMutex_ != nullptr && xSemaphoreTake(busMutex_, pdMS_TO_TICKS(20)) != pdTRUE) {
    LOGW("MAX30102 read skipped: I2C busy");
    return;
  }

  const uint32_t irValue = sensor_.getIR();
  if (busMutex_ != nullptr) {
    xSemaphoreGive(busMutex_);
  }

  if (detectBeat_(irValue) && lastBeatMs_ > 0) {
    const uint32_t interval = nowMs - lastBeatMs_;
    if (interval > 250UL && interval < 2000UL) {
      latestHeartRate_ = static_cast<int>(60000UL / interval);
    }
    lastBeatMs_ = nowMs;
    return;
  }

  if (detectBeat_(irValue) && lastBeatMs_ == 0) {
    lastBeatMs_ = nowMs;
  }
}

int Max30102Sensor::latestHeartRate() const { return latestHeartRate_; }

bool Max30102Sensor::detectBeat_(uint32_t irValue) {
  const bool rising = irValue > previousIr_;
  const bool threshold = irValue > 50000UL;
  const bool beat = threshold && !rising && previousIr_ > 50000UL;
  previousIr_ = irValue;
  return beat;
}
