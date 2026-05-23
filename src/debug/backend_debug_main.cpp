#include <Arduino.h>

#include "config/AppConfig.h"
#include "debug/DebugHelpers.h"
#include "models/SensorData.h"
#include "network/ApiClient.h"
#include "network/WiFiService.h"

namespace {
WiFiService wifiService;
ApiClient apiClient(AppConfig::API_ENDPOINT);
uint32_t lastPostAttemptMs = 0;
uint32_t sampleCounter = 0;

SensorSnapshot buildDebugSnapshot(uint32_t nowMs) {
  SensorSnapshot snapshot;
  snapshot.sampleMs = nowMs;
  snapshot.gsr = 0.25f + (sampleCounter % 10U) * 0.03f;
  snapshot.heartRate = 72 + static_cast<int>(sampleCounter % 7U);
  snapshot.temperature = 36.4f + static_cast<float>(sampleCounter % 5U) * 0.1f;
  snapshot.cuffPressure = 0.0f;
  snapshot.batteryLevel = AppConfig::DEFAULT_BATTERY_LEVEL;
  snapshot.bp.valid = true;
  snapshot.bp.systolic = 118 + static_cast<int>(sampleCounter % 4U);
  snapshot.bp.diastolic = 76 + static_cast<int>(sampleCounter % 3U);
  ++sampleCounter;
  return snapshot;
}
}  // namespace

void setup() {
  DebugHelpers::waitForSerial();
  wifiService.begin();

  Serial.println("Backend debug mode");
  Serial.printf("Endpoint: %s\r\n", AppConfig::API_ENDPOINT);
#if defined(DEBUG_WIFI_FALLBACK_SSID)
  Serial.printf("Fallback WiFi: %s\r\n", DEBUG_WIFI_FALLBACK_SSID);
#endif
  DebugHelpers::printDivider();
}

void loop() {
  const uint32_t nowMs = millis();
  wifiService.maintain();

  if ((nowMs - lastPostAttemptMs) < 10000UL) {
    delay(100);
    return;
  }

  lastPostAttemptMs = nowMs;
  if (!wifiService.isConnected()) {
    Serial.printf("[%10lu ms] WiFi belum terhubung, upload ditunda\r\n", nowMs);
    return;
  }

  const SensorSnapshot snapshot = buildDebugSnapshot(nowMs);
  const String payload = apiClient.buildPayload(snapshot);
  const bool ok = apiClient.postPayload(payload);
  Serial.printf("[%10lu ms] upload=%s ssid=%s ip=%s\r\n", nowMs, ok ? "OK" : "FAIL",
                wifiService.ssid().c_str(), wifiService.ipAddress().c_str());
}
