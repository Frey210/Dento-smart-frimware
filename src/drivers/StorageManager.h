#pragma once

#include <Arduino.h>

class StorageManager {
 public:
  bool begin();
  bool appendOfflinePayload(const String& payload);
  bool popOldestOfflinePayload(String& payloadOut);
  bool hasOfflinePayload() const;
};

