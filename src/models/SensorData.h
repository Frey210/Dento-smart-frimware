#pragma once

#include <Arduino.h>

enum class BpState : uint8_t {
  IDLE = 0,
  INFLATING,
  HOLD,
  MEASURING,
  DEFLATING,
  COMPLETE,
  ERROR
};

enum class UiScreen : uint8_t {
  MENU = 0,
  LIVE_MONITOR,
  WIFI_SETUP,
  DEVICE_INFO,
  CALIBRATION,
  BP_PROGRESS
};

struct BloodPressureReading {
  bool valid = false;
  int systolic = 0;
  int diastolic = 0;
};

struct SensorSnapshot {
  uint32_t sampleMs = 0;
  float gsr = 0.0f;
  int heartRate = 0;
  float temperature = 0.0f;
  float cuffPressure = 0.0f;
  uint8_t batteryLevel = 0;
  BloodPressureReading bp;
};

struct SharedRuntimeState {
  SensorSnapshot latestSensor;
  BpState bpState = BpState::IDLE;
  UiScreen uiScreen = UiScreen::MENU;
  uint8_t menuIndex = 0;
  uint8_t bpProgress = 0;
  bool wifiPortalRequested = false;
  float anxietyScore = 0.0f;
};

struct UploadPayload {
  char json[256] = {0};
};

enum class ButtonEventType : uint8_t {
  SHORT_PRESS = 0,
  LONG_PRESS
};

enum class ButtonId : uint8_t {
  UP = 0,
  DOWN,
  SELECT,
  BACK
};

struct ButtonEvent {
  ButtonId id = ButtonId::UP;
  ButtonEventType type = ButtonEventType::SHORT_PRESS;
  uint32_t eventMs = 0;
};

inline const char* toString(BpState state) {
  switch (state) {
    case BpState::IDLE:
      return "IDLE";
    case BpState::INFLATING:
      return "INFLATING";
    case BpState::HOLD:
      return "HOLD";
    case BpState::MEASURING:
      return "MEASURING";
    case BpState::DEFLATING:
      return "DEFLATING";
    case BpState::COMPLETE:
      return "COMPLETE";
    case BpState::ERROR:
      return "ERROR";
  }
  return "UNKNOWN";
}

inline const char* toString(UiScreen screen) {
  switch (screen) {
    case UiScreen::MENU:
      return "MENU";
    case UiScreen::LIVE_MONITOR:
      return "LIVE_MONITOR";
    case UiScreen::WIFI_SETUP:
      return "WIFI_SETUP";
    case UiScreen::DEVICE_INFO:
      return "DEVICE_INFO";
    case UiScreen::CALIBRATION:
      return "CALIBRATION";
    case UiScreen::BP_PROGRESS:
      return "BP_PROGRESS";
  }
  return "UNKNOWN";
}

inline const char* toString(ButtonId id) {
  switch (id) {
    case ButtonId::UP:
      return "UP";
    case ButtonId::DOWN:
      return "DOWN";
    case ButtonId::SELECT:
      return "SELECT";
    case ButtonId::BACK:
      return "BACK";
  }
  return "UNKNOWN";
}

inline const char* toString(ButtonEventType type) {
  switch (type) {
    case ButtonEventType::SHORT_PRESS:
      return "SHORT";
    case ButtonEventType::LONG_PRESS:
      return "LONG";
  }
  return "UNKNOWN";
}
