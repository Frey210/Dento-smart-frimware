#include "drivers/StorageManager.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

#include "config/AppConfig.h"

namespace {
bool normalizeJsonPayload(const String& payload, String& normalizedOut) {
  String candidate = payload;
  candidate.trim();
  if (candidate.isEmpty()) {
    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, candidate);
  if (error) {
    return false;
  }

  normalizedOut = "";
  serializeJson(doc, normalizedOut);
  return !normalizedOut.isEmpty();
}

int findJsonObjectEnd(const String& content) {
  int depth = 0;
  bool inString = false;
  bool escaped = false;
  bool started = false;

  for (int i = 0; i < static_cast<int>(content.length()); ++i) {
    const char c = content[i];
    if (!started) {
      if (c == '{') {
        started = true;
        depth = 1;
      }
      continue;
    }

    if (escaped) {
      escaped = false;
      continue;
    }
    if (c == '\\') {
      escaped = inString;
      continue;
    }
    if (c == '"') {
      inString = !inString;
      continue;
    }
    if (inString) {
      continue;
    }
    if (c == '{') {
      ++depth;
    } else if (c == '}') {
      --depth;
      if (depth == 0) {
        return i + 1;
      }
    }
  }

  return -1;
}
}  // namespace

bool StorageManager::begin() { return LittleFS.begin(true); }

bool StorageManager::appendOfflinePayload(const String& payload) {
  String normalized;
  if (!normalizeJsonPayload(payload, normalized)) {
    return false;
  }

  File file = LittleFS.open(AppConfig::OFFLINE_LOG_FILE, FILE_APPEND);
  if (!file) {
    return false;
  }
  file.println(normalized);
  file.close();
  return true;
}

bool StorageManager::popOldestOfflinePayload(String& payloadOut) {
  File readFile = LittleFS.open(AppConfig::OFFLINE_LOG_FILE, FILE_READ);
  if (!readFile) {
    return false;
  }

  const String content = readFile.readString();
  readFile.close();

  const int objectEnd = findJsonObjectEnd(content);
  if (objectEnd <= 0) {
    LittleFS.remove(AppConfig::OFFLINE_LOG_FILE);
    return false;
  }

  String firstPayload = content.substring(0, objectEnd);
  String normalized;
  if (!normalizeJsonPayload(firstPayload, normalized)) {
    LittleFS.remove(AppConfig::OFFLINE_LOG_FILE);
    return false;
  }

  String remainder = content.substring(objectEnd);
  remainder.trim();

  File writeFile = LittleFS.open(AppConfig::OFFLINE_LOG_FILE, FILE_WRITE);
  if (!writeFile) {
    return false;
  }
  if (!remainder.isEmpty()) {
    writeFile.println(remainder);
  }
  writeFile.close();

  payloadOut = normalized;
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
