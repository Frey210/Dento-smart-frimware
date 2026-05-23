#pragma once

#include <Arduino.h>

#include "models/AppContext.h"

class MenuController {
 public:
  explicit MenuController(AppContext& app);
  void handleButton(const ButtonEvent& event);
  void render();

 private:
  static constexpr uint8_t kMenuItemCount = 5;

  AppContext& app_;
};

