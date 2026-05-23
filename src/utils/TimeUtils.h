#pragma once

#include <Arduino.h>

namespace TimeUtils {
String iso8601FromMillis(uint32_t millisSinceBoot);
}

