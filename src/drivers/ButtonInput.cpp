#include "drivers/ButtonInput.h"

ButtonInput::ButtonInput(uint8_t upPin, uint8_t downPin, uint8_t selectPin, uint8_t backPin) {
  buttons_[0] = ButtonState{upPin, ButtonId::UP};
  buttons_[1] = ButtonState{downPin, ButtonId::DOWN};
  buttons_[2] = ButtonState{selectPin, ButtonId::SELECT};
  buttons_[3] = ButtonState{backPin, ButtonId::BACK};
}

void ButtonInput::begin() {
  for (auto& button : buttons_) {
    pinMode(button.pin, INPUT_PULLUP);
    button.stableLevel = digitalRead(button.pin);
    button.lastRead = button.stableLevel;
    button.changedMs = millis();
  }
}

bool ButtonInput::poll(ButtonEvent& eventOut, uint32_t nowMs) {
  for (auto& button : buttons_) {
    if (processButton_(button, eventOut, nowMs)) {
      return true;
    }
  }
  return false;
}

bool ButtonInput::processButton_(ButtonState& button, ButtonEvent& eventOut, uint32_t nowMs) {
  const bool level = digitalRead(button.pin);
  if (level != button.lastRead) {
    button.lastRead = level;
    button.changedMs = nowMs;
  }

  if ((nowMs - button.changedMs) < kDebounceMs || level == button.stableLevel) {
    return false;
  }

  button.stableLevel = level;
  if (level == LOW) {
    button.pressedMs = nowMs;
    button.longPressSent = false;
    return false;
  }

  const uint32_t pressDuration = nowMs - button.pressedMs;
  eventOut.id = button.id;
  eventOut.eventMs = nowMs;
  if (pressDuration >= kLongPressMs) {
    eventOut.type = ButtonEventType::LONG_PRESS;
  } else {
    eventOut.type = ButtonEventType::SHORT_PRESS;
  }
  return true;
}

