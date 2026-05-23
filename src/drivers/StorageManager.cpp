#include "drivers/StorageManager.h"

#include <LittleFS.h>

#include "config/AppConfig.h"

bool StorageManager::begin() { return LittleFS.begin(true); }

bool StorageManager::appendOfflinePayload(const String& payload) {
  File file = LittleFS.open(AppConfig::OFFLINE_LOG_FILE, FILE_APPEND);
  if (!file) {
    return false;
  }
  file.println(payload);
  file.close();
  return true;
}

bool StorageManager::popOldestOfflinePayload(String& payloadOut) {
  File readFile = LittleFS.open(AppConfig::OFFLINE_LOG_FILE, FILE_READ);
  if (!readFile) {
    return false;
  }

  String firstLine = readFile.readStringUntil('\n');
  String remainder;
  while (readFile.available()) {
    remainder += readFile.readStringUntil('\n');
    if (readFile.available()) {
      remainder += '\n';
    }
  }
  readFile.close();

  File writeFile = LittleFS.open(AppConfig::OFFLINE_LOG_FILE, FILE_WRITE);
  if (!writeFile) {
    return false;
  }
  writeFile.print(remainder);
  writeFile.close();

  firstLine.trim();
  payloadOut = firstLine;
  return !payloadOut.isEmpty();
}

bool StorageManager::hasOfflinePayload() const {
  File file = LittleFS.open(AppConfig::OFFLINE_LOG_FILE, FILE_READ);
  if (!file) {
    return false;
  }
  const bool hasData = file.available() > 0;
  file.close();
  return hasData;
}

