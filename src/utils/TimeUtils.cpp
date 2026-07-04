#include "utils/TimeUtils.h"

#include <time.h>

namespace TimeUtils {
String iso8601FromMillis(uint32_t millisSinceBoot) {
  const time_t now = time(nullptr);
  if (now >= 1609459200) {
    struct tm utc;
    gmtime_r(&now, &utc);

    char timeBuffer[32];
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%dT%H:%M:%SZ", &utc);
    return String(timeBuffer);
  }

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
