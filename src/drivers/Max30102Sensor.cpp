#include "drivers/Max30102Sensor.h"

#include "config/AppConfig.h"
#include "utils/SerialLog.h"
#include "heartRate.h"

namespace {
constexpr uint32_t FINGER_IR_THRESHOLD = 50000UL;
constexpr uint32_t FINGER_LOST_TIMEOUT_MS = 3000UL;
constexpr uint32_t I2C_BUSY_WARNING_PERIOD_MS = 2000UL;
constexpr uint32_t I2C_READ_WAIT_MS = 100UL;
}

bool Max30102Sensor::begin(TwoWire& wire) {
  if (busMutex_ != nullptr && xSemaphoreTake(busMutex_, pdMS_TO_TICKS(100)) != pdTRUE) {
    LOGE("MAX30102 begin failed: I2C mutex timeout");
    initialized_ = false;
    return false;
  }

  const bool ok = sensor_.begin(wire, AppConfig::I2C_FREQUENCY);
  if (!ok) {
    if (busMutex_ != nullptr) {
      xSemaphoreGive(busMutex_);
    }
    LOGW("MAX30102 not detected");
    initialized_ = false;
    return false;
  }

  sensor_.setup(0x2F, 4, 2, 400, 411, 4096);
  sensor_.setPulseAmplitudeRed(0x0A);
  sensor_.setPulseAmplitudeIR(0x2F);
  sensor_.setPulseAmplitudeGreen(0x00);
  sensor_.clearFIFO();
  if (busMutex_ != nullptr) {
    xSemaphoreGive(busMutex_);
  }

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
  if (busMutex_ != nullptr && xSemaphoreTake(busMutex_, pdMS_TO_TICKS(I2C_READ_WAIT_MS)) != pdTRUE) {
    if ((nowMs - lastI2cBusyWarningMs_) >= I2C_BUSY_WARNING_PERIOD_MS) {
      LOGW("MAX30102 read skipped: I2C busy");
      lastI2cBusyWarningMs_ = nowMs;
    }
    return;
  }

  sensor_.check();
  if (sensor_.available() == 0) {
    if (busMutex_ != nullptr) {
      xSemaphoreGive(busMutex_);
    }
    return;
  }

  latestIr_ = sensor_.getFIFOIR();
  sensor_.nextSample();
  if (busMutex_ != nullptr) {
    xSemaphoreGive(busMutex_);
  }

  fingerDetected_ = latestIr_ >= FINGER_IR_THRESHOLD;
  if (!fingerDetected_) {
    if (lastBeatMs_ == 0 || (nowMs - lastBeatMs_) >= FINGER_LOST_TIMEOUT_MS) {
      latestHeartRate_ = 0;
      validRateCount_ = 0;
      rateSpot_ = 0;
    }
    return;
  }

  if (!checkForBeat(static_cast<int32_t>(latestIr_))) {
    return;
  }

  if (lastBeatMs_ == 0) {
    lastBeatMs_ = nowMs;
    return;
  }

  const uint32_t interval = nowMs - lastBeatMs_;
  lastBeatMs_ = nowMs;
  if (interval < 250UL || interval > 2000UL) {
    return;
  }

  const uint8_t beatsPerMinute = static_cast<uint8_t>(60000UL / interval);
  if (beatsPerMinute < 20U || beatsPerMinute > 240U) {
    return;
  }

  rates_[rateSpot_] = beatsPerMinute;
  rateSpot_ = (rateSpot_ + 1U) % RATE_BUFFER_SIZE;
  if (validRateCount_ < RATE_BUFFER_SIZE) {
    ++validRateCount_;
  }

  uint16_t total = 0;
  for (uint8_t i = 0; i < validRateCount_; ++i) {
    total += rates_[i];
  }
  latestHeartRate_ = total / validRateCount_;
}

int Max30102Sensor::latestHeartRate() const { return latestHeartRate_; }

uint32_t Max30102Sensor::latestIr() const { return latestIr_; }

bool Max30102Sensor::fingerDetected() const { return fingerDetected_; }
