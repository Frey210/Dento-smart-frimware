#include "utils/TimeUtils.h"

namespace TimeUtils {
String iso8601FromMillis(uint32_t millisSinceBoot) {
  const uint32_t totalSeconds = millisSinceBoot / 1000UL;
  const uint32_t seconds = totalSeconds % 60UL;
  const uint32_t minutes = (totalSeconds / 60UL) % 60UL;
  const uint32_t hours = (totalSeconds / 3600UL) % 24UL;
  const uint32_t days = totalSeconds / 86400UL;

  char buffer[32];
  snprintf(buffer, sizeof(buffer), "1970-01-%02luT%02lu:%02lu:%02luZ",
           static_cast<unsigned long>((days % 28UL) + 1UL),
           static_cast<unsigned long>(hours),
           static_cast<unsigned long>(minutes),
           static_cast<unsigned long>(seconds));
  return String(buffer);
}
}  // namespace TimeUtils

