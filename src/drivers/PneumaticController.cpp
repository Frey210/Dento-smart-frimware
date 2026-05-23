#include "drivers/PneumaticController.h"

#include "config/AppConfig.h"

namespace {
constexpr float kOscillationDcAlpha = 0.08f;
constexpr float kPressureFilterAlpha = 0.25f;
constexpr float kMinOscillationAmplitudeMmhg = 0.8f;
constexpr float kMinPeakEnvelopeMmhg = 1.5f;
constexpr uint32_t kMinBeatIntervalMs = 250UL;
constexpr float kMeasurementStopPressureMmhg = 40.0f;
constexpr float kSystolicRatio = 0.55f;
constexpr float kDiastolicRatio = 0.82f;
constexpr uint32_t kValvePulsePeriodMs = 160UL;

uint32_t measurementValveOpenWindowMs(float pressureMmhg) {
  if (pressureMmhg > 120.0f) {
    return 20UL;
  }
  if (pressureMmhg > 90.0f) {
    return 28UL;
  }
  if (pressureMmhg > 70.0f) {
    return 36UL;
  }
  return 48UL;
}
}  // namespace

PneumaticController::PneumaticController(uint8_t pumpPin, uint8_t valvePin,
                                         Hx710bSensor& pressureSensor)
    : pumpPin_(pumpPin), valvePin_(valvePin), pressureSensor_(pressureSensor) {}

void PneumaticController::begin() {
  pinMode(pumpPin_, OUTPUT);
  pinMode(valvePin_, OUTPUT);
  setPump_(false);
  setValve_(false);
}

void PneumaticController::requestMeasurement() {
  if (state_ == BpState::IDLE || state_ == BpState::COMPLETE || state_ == BpState::ERROR) {
    resetReading_();
    resetMeasurementData_();
    setState_(BpState::INFLATING, millis());
    measurementStartedMs_ = stateStartedMs_;
    progress_ = 0;
  }
}

void PneumaticController::update(uint32_t nowMs) {
  currentPressure_ = pressureSensor_.readPressureMmhg();

  if (state_ != BpState::IDLE &&
      (nowMs - measurementStartedMs_) > AppConfig::BP_MEASUREMENT_TIMEOUT_MS) {
    stop();
    state_ = BpState::ERROR;
    progress_ = 0;
    return;
  }

  switch (state_) {
    case BpState::IDLE:
      setPump_(false);
      setValve_(false);
      progress_ = 0;
      break;
    case BpState::INFLATING:
      setPump_(true);
      setValve_(false);
      progress_ = static_cast<uint8_t>(min(30.0f, currentPressure_ /
                                                     AppConfig::BP_TARGET_PRESSURE_MMHG * 30.0f));
      if (currentPressure_ >= AppConfig::BP_TARGET_PRESSURE_MMHG) {
        setPump_(false);
        setState_(BpState::HOLD, nowMs);
      }
      break;
    case BpState::HOLD:
      setPump_(false);
      setValve_(false);
      progress_ = 40;
      if ((nowMs - stateStartedMs_) >= AppConfig::BP_HOLD_DURATION_MS) {
        setState_(BpState::MEASURING, nowMs);
      }
      break;
    case BpState::MEASURING:
      setPump_(false);
      setValve_(((nowMs - stateStartedMs_) % kValvePulsePeriodMs) <
                measurementValveOpenWindowMs(currentPressure_));
      updateOscillationModel_(nowMs, currentPressure_);
      progress_ = static_cast<uint8_t>(
          40U + min(50.0f, (AppConfig::BP_TARGET_PRESSURE_MMHG - currentPressure_) /
                               (AppConfig::BP_TARGET_PRESSURE_MMHG - kMeasurementStopPressureMmhg) *
                               50.0f));
      if (currentPressure_ <= kMeasurementStopPressureMmhg) {
        const bool valid = finalizeOscillationReading_();
        setState_(valid ? BpState::DEFLATING : BpState::ERROR, nowMs);
      }
      break;
    case BpState::DEFLATING:
      setPump_(false);
      setValve_(true);
      progress_ = 90;
      if (currentPressure_ <= 5.0f) {
        setState_(BpState::COMPLETE, nowMs);
        setValve_(false);
      }
      break;
    case BpState::COMPLETE:
      setPump_(false);
      setValve_(false);
      progress_ = 100;
      break;
    case BpState::ERROR:
      setPump_(false);
      setValve_(true);
      progress_ = 0;
      break;
  }
}

void PneumaticController::stop() {
  setPump_(false);
  setValve_(false);
}

BpState PneumaticController::state() const { return state_; }

uint8_t PneumaticController::progress() const { return progress_; }

BloodPressureReading PneumaticController::reading() const { return reading_; }

float PneumaticController::currentPressure() const { return currentPressure_; }

void PneumaticController::setPump_(bool enabled) { digitalWrite(pumpPin_, enabled ? HIGH : LOW); }

void PneumaticController::setValve_(bool enabled) {
  digitalWrite(valvePin_, enabled ? HIGH : LOW);
}

void PneumaticController::resetReading_() { reading_ = BloodPressureReading{}; }

void PneumaticController::resetMeasurementData_() {
  filteredPressure_ = 0.0f;
  dcPressure_ = 0.0f;
  pulseAbsPrev2_ = 0.0f;
  pulseAbsPrev1_ = 0.0f;
  pressurePrev2_ = 0.0f;
  pressurePrev1_ = 0.0f;
  oscillationHistoryPrimed_ = false;
  oscillationSampleCount_ = 0;
  lastOscillationPeakMs_ = 0;
}

void PneumaticController::updateOscillationModel_(uint32_t nowMs, float pressureMmhg) {
  if (!oscillationHistoryPrimed_) {
    filteredPressure_ = pressureMmhg;
    dcPressure_ = pressureMmhg;
    pressurePrev2_ = pressureMmhg;
    pressurePrev1_ = pressureMmhg;
    pulseAbsPrev2_ = 0.0f;
    pulseAbsPrev1_ = 0.0f;
    oscillationHistoryPrimed_ = true;
    return;
  }

  filteredPressure_ =
      ((1.0f - kPressureFilterAlpha) * filteredPressure_) + (kPressureFilterAlpha * pressureMmhg);
  dcPressure_ = ((1.0f - kOscillationDcAlpha) * dcPressure_) + (kOscillationDcAlpha * filteredPressure_);
  const float pulseAbs = fabsf(filteredPressure_ - dcPressure_);

  if (pulseAbsPrev1_ > pulseAbsPrev2_ && pulseAbsPrev1_ >= pulseAbs &&
      pulseAbsPrev1_ >= kMinOscillationAmplitudeMmhg &&
      (nowMs - lastOscillationPeakMs_) >= kMinBeatIntervalMs) {
    appendOscillationSample_(pressurePrev1_, pulseAbsPrev1_, nowMs);
  }

  pulseAbsPrev2_ = pulseAbsPrev1_;
  pulseAbsPrev1_ = pulseAbs;
  pressurePrev2_ = pressurePrev1_;
  pressurePrev1_ = filteredPressure_;
}

void PneumaticController::appendOscillationSample_(float pressureMmhg, float amplitude,
                                                   uint32_t nowMs) {
  if (oscillationSampleCount_ >= kMaxOscillationSamples_) {
    return;
  }
  oscillationSamples_[oscillationSampleCount_++] = OscillationSample{pressureMmhg, amplitude};
  lastOscillationPeakMs_ = nowMs;
}

bool PneumaticController::finalizeOscillationReading_() {
  if (oscillationSampleCount_ < 6) {
    resetReading_();
    return false;
  }

  float smoothedAmplitudes[kMaxOscillationSamples_];
  for (size_t i = 0; i < oscillationSampleCount_; ++i) {
    const float left = (i == 0) ? oscillationSamples_[i].amplitude : oscillationSamples_[i - 1].amplitude;
    const float center = oscillationSamples_[i].amplitude;
    const float right =
        (i + 1 >= oscillationSampleCount_) ? oscillationSamples_[i].amplitude : oscillationSamples_[i + 1].amplitude;
    smoothedAmplitudes[i] = (left + center + right) / 3.0f;
  }

  size_t mapIndex = 0;
  for (size_t i = 1; i < oscillationSampleCount_; ++i) {
    if (smoothedAmplitudes[i] > smoothedAmplitudes[mapIndex]) {
      mapIndex = i;
    }
  }

  const float mapAmplitude = smoothedAmplitudes[mapIndex];
  if (mapAmplitude < kMinPeakEnvelopeMmhg) {
    resetReading_();
    return false;
  }

  const float systolicThreshold = mapAmplitude * kSystolicRatio;
  const float diastolicThreshold = mapAmplitude * kDiastolicRatio;

  bool systolicFound = false;
  bool diastolicFound = false;
  float systolicPressure = 0.0f;
  float diastolicPressure = 0.0f;

  for (size_t i = 1; i <= mapIndex; ++i) {
    if (smoothedAmplitudes[i - 1] < systolicThreshold && smoothedAmplitudes[i] >= systolicThreshold) {
      systolicPressure = interpolatePressureAtAmplitude_(
          oscillationSamples_[i - 1].pressureMmhg, smoothedAmplitudes[i - 1],
          oscillationSamples_[i].pressureMmhg, smoothedAmplitudes[i], systolicThreshold);
      systolicFound = true;
      break;
    }
  }

  for (size_t i = mapIndex + 1; i < oscillationSampleCount_; ++i) {
    if (smoothedAmplitudes[i - 1] >= diastolicThreshold && smoothedAmplitudes[i] < diastolicThreshold) {
      diastolicPressure = interpolatePressureAtAmplitude_(
          oscillationSamples_[i - 1].pressureMmhg, smoothedAmplitudes[i - 1],
          oscillationSamples_[i].pressureMmhg, smoothedAmplitudes[i], diastolicThreshold);
      diastolicFound = true;
      break;
    }
  }

  if (!systolicFound || !diastolicFound || systolicPressure <= diastolicPressure ||
      systolicPressure < 70.0f || systolicPressure > 220.0f || diastolicPressure < 40.0f ||
      diastolicPressure > 140.0f) {
    resetReading_();
    return false;
  }

  reading_.valid = true;
  reading_.systolic = static_cast<int>(lroundf(systolicPressure));
  reading_.diastolic = static_cast<int>(lroundf(diastolicPressure));
  return true;
}

float PneumaticController::interpolatePressureAtAmplitude_(float pressureA, float amplitudeA,
                                                           float pressureB, float amplitudeB,
                                                           float targetAmplitude) const {
  const float amplitudeDelta = amplitudeB - amplitudeA;
  if (fabsf(amplitudeDelta) < 0.001f) {
    return pressureB;
  }
  const float ratio = (targetAmplitude - amplitudeA) / amplitudeDelta;
  return pressureA + ((pressureB - pressureA) * ratio);
}

void PneumaticController::setState_(BpState nextState, uint32_t nowMs) {
  state_ = nextState;
  stateStartedMs_ = nowMs;
}
