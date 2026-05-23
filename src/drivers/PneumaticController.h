#pragma once

#include <Arduino.h>

#include "drivers/Hx710bSensor.h"
#include "models/SensorData.h"

class PneumaticController {
 public:
  PneumaticController(uint8_t pumpPin, uint8_t valvePin, Hx710bSensor& pressureSensor);

  void begin();
  void requestMeasurement();
  void update(uint32_t nowMs);
  void stop();

  BpState state() const;
  uint8_t progress() const;
  BloodPressureReading reading() const;
  float currentPressure() const;

 private:
  struct OscillationSample {
    float pressureMmhg = 0.0f;
    float amplitude = 0.0f;
  };

  void setPump_(bool enabled);
  void setValve_(bool enabled);
  void resetReading_();
  void resetMeasurementData_();
  void updateOscillationModel_(uint32_t nowMs, float pressureMmhg);
  void appendOscillationSample_(float pressureMmhg, float amplitude, uint32_t nowMs);
  bool finalizeOscillationReading_();
  float interpolatePressureAtAmplitude_(float pressureA, float amplitudeA, float pressureB,
                                        float amplitudeB, float targetAmplitude) const;
  void setState_(BpState nextState, uint32_t nowMs);

  uint8_t pumpPin_;
  uint8_t valvePin_;
  Hx710bSensor& pressureSensor_;
  BpState state_ = BpState::IDLE;
  BloodPressureReading reading_;
  float currentPressure_ = 0.0f;
  uint8_t progress_ = 0;
  uint32_t stateStartedMs_ = 0;
  uint32_t measurementStartedMs_ = 0;
  uint32_t lastOscillationPeakMs_ = 0;
  float filteredPressure_ = 0.0f;
  float dcPressure_ = 0.0f;
  float pulseAbsPrev2_ = 0.0f;
  float pulseAbsPrev1_ = 0.0f;
  float pressurePrev2_ = 0.0f;
  float pressurePrev1_ = 0.0f;
  bool oscillationHistoryPrimed_ = false;
  size_t oscillationSampleCount_ = 0;
  static constexpr size_t kMaxOscillationSamples_ = 96;
  OscillationSample oscillationSamples_[kMaxOscillationSamples_];
};
