#pragma once

#include <Arduino.h>

namespace SystemEvents {
constexpr EventBits_t WIFI_CONNECTED = BIT0;
constexpr EventBits_t BP_REQUEST = BIT1;
constexpr EventBits_t BP_RUNNING = BIT2;
constexpr EventBits_t BP_COMPLETE = BIT3;
}  // namespace SystemEvents

