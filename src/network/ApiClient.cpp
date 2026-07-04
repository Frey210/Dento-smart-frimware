#include "network/ApiClient.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "config/AppConfig.h"
#include "utils/SerialLog.h"
#include "utils/TimeUtils.h"

namespace {
const char* httpCodeLabel(int responseCode) {
  switch (responseCode) {
    case 200:
      return "OK";
    case 201:
      return "CREATED";
    case 202:
      return "ACCEPTED";
    case 400:
      return "BAD_REQUEST";
    case 401:
      return "UNAUTHORIZED";
    case 403:
      return "FORBIDDEN";
    case 404:
      return "NOT_FOUND";
    case 409:
      return "CONFLICT";
    case 415:
      return "UNSUPPORTED_MEDIA_TYPE";
    case 422:
      return "UNPROCESSABLE_ENTITY";
    case 429:
      return "RATE_LIMITED";
    case 500:
      return "INTERNAL_SERVER_ERROR";
    case 502:
      return "BAD_GATEWAY";
    case 503:
      return "SERVICE_UNAVAILABLE";
    case 504:
      return "GATEWAY_TIMEOUT";
    default:
      return "HTTP_STATUS_OTHER";
  }
}
}  // namespace

ApiClient::ApiClient(const String& endpoint) : endpoint_(endpoint) {}

String ApiClient::buildPayload(const SensorSnapshot& snapshot) const {
  JsonDocument doc;
  doc["device_uid"] = AppConfig::DEVICE_UID;
  doc["timestamp"] = TimeUtils::iso8601FromMillis(snapshot.sampleMs);
  doc["gsr"] = snapshot.gsr;
  doc["heart_rate"] = snapshot.heartRate;
  doc["temperature"] = snapshot.temperature;
  doc["blood_pressure_sys"] = snapshot.bp.valid ? snapshot.bp.systolic : 0;
  doc["blood_pressure_dia"] = snapshot.bp.valid ? snapshot.bp.diastolic : 0;
  doc["battery_level"] = snapshot.batteryLevel;

  String payload;
  serializeJson(doc, payload);
  return payload;
}

bool ApiClient::postPayload(const String& payload) const {
  if (WiFi.status() != WL_CONNECTED) {
    LOGW("Upload skipped: WiFi disconnected");
    return false;
  }

  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();
  http.setTimeout(AppConfig::HTTP_TIMEOUT_MS);
  if (!http.begin(client, endpoint_)) {
    LOGE("HTTP begin failed for endpoint: %s", endpoint_.c_str());
    return false;
  }

  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-DEVICE-KEY", AppConfig::DEVICE_KEY);
  LOGI("Posting payload: %s", payload.c_str());
  const int responseCode = http.POST(payload);
  const String responseBody = http.getString();
  LOGI("HTTP POST %s -> %d %s", endpoint_.c_str(), responseCode, httpCodeLabel(responseCode));
  if (responseCode >= 200 && responseCode < 300) {
    LOGI("Backend accepted payload");
  } else if (responseCode == 401 || responseCode == 403) {
    LOGE("Backend rejected device authentication. Check X-DEVICE-KEY and device registration.");
  } else if (responseCode == 422 || responseCode == 400) {
    LOGE("Backend rejected payload format/content. Check JSON fields and values.");
  } else if (responseCode >= 500) {
    LOGE("Backend server error. Request reached server but processing failed.");
  } else if (responseCode < 0) {
    LOGE("HTTP transport error: %d", responseCode);
  }
  LOGI("Backend response body: %s", responseBody.c_str());
  http.end();
  return responseCode >= 200 && responseCode < 300;
}

const String& ApiClient::endpoint() const { return endpoint_; }
