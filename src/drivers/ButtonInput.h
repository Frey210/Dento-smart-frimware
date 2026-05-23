#pragma once

#include <Arduino.h>

#include "models/SensorData.h"

class ButtonInput {
 public:
  ButtonInput(uint8_t upPin, uint8_t downPin, uint8_t selectPin, uint8_t backPin);
  void begin();
  bool poll(ButtonEvent& eventOut, uint32_t nowMs);

 private:
  struct ButtonState {
    uint8_t pin = 0;
    ButtonId id = ButtonId::UP;
    bool stableLevel = HIGH;
    bool lastRead = HIGH;
    uint32_t changedMs = 0;
    uint32_t pressedMs = 0;
    bool longPressSent = false;
  };

  bool processButton_(ButtonState& button, ButtonEvent& eventOut, uint32_t nowMs);

  ButtonState buttons_[4];
  static constexpr uint32_t kDebounceMs = 30;
  static constexpr uint32_t kLongPressMs = 800;
};

