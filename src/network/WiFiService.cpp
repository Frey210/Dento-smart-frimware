#include "network/WiFiService.h"

#include <WiFi.h>

#include "config/AppConfig.h"
#include "utils/SerialLog.h"

namespace {
const char* wifiStatusToString(wl_status_t status) {
  switch (status) {
    case WL_IDLE_STATUS:
      return "IDLE";
    case WL_NO_SSID_AVAIL:
      return "NO_SSID";
    case WL_SCAN_COMPLETED:
      return "SCAN_DONE";
    case WL_CONNECTED:
      return "CONNECTED";
    case WL_CONNECT_FAILED:
      return "CONNECT_FAILED";
    case WL_CONNECTION_LOST:
      return "CONNECTION_LOST";
    case WL_DISCONNECTED:
      return "DISCONNECTED";
    default:
      return "UNKNOWN";
  }
}

#if defined(DEBUG_WIFI_FALLBACK_SSID) && defined(DEBUG_WIFI_FALLBACK_PASSWORD)
bool tryDebugFallbackConnect() {
  LOGI("Trying debug fallback WiFi ssid=%s", DEBUG_WIFI_FALLBACK_SSID);
  WiFi.begin(DEBUG_WIFI_FALLBACK_SSID, DEBUG_WIFI_FALLBACK_PASSWORD);

  const uint32_t startedMs = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startedMs) < 15000UL) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();

  const bool connected = WiFi.status() == WL_CONNECTED;
  LOGI("Debug fallback WiFi connected=%d status=%s ip=%s", connected,
       wifiStatusToString(WiFi.status()), WiFi.localIP().toString().c_str());
  return connected;
}
#endif
}  // namespace

void WiFiService::begin() {
  WiFi.mode(WIFI_STA);
  manager_.setConnectTimeout(15);
  manager_.setConfigPortalTimeout(180);
  portalRequested_ = true;
  fallbackAttempted_ = false;
  LOGI("WiFiService initialized, portal scheduled");
}

void WiFiService::maintain() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  if (portalRequested_) {
#if defined(DEBUG_WIFI_FALLBACK_SSID) && defined(DEBUG_WIFI_FALLBACK_PASSWORD)
    if (!fallbackAttempted_) {
      fallbackAttempted_ = true;
      if (tryDebugFallbackConnect()) {
        portalRequested_ = false;
        return;
      }
    }
#endif
    LOGI("Starting WiFiManager portal ssid=%s", AppConfig::WIFI_AP_NAME);
    LOGI("WiFi status before portal: %s", wifiStatusToString(WiFi.status()));
    const bool connected = manager_.autoConnect(AppConfig::WIFI_AP_NAME, AppConfig::WIFI_AP_PASSWORD);
    portalRequested_ = false;
    LOGI("WiFiManager portal finished connected=%d status=%s ip=%s", connected,
         wifiStatusToString(WiFi.status()), WiFi.localIP().toString().c_str());
    return;
  }

  LOGW("WiFi disconnected, attempting reconnect status=%s", wifiStatusToString(WiFi.status()));
  WiFi.reconnect();
}

bool WiFiService::isConnected() const { return WiFi.status() == WL_CONNECTED; }

void WiFiService::requestPortal() {
  portalRequested_ = true;
  LOGI("WiFi captive portal requested from UI");
}

String WiFiService::ipAddress() const { return WiFi.localIP().toString(); }

String WiFiService::ssid() const { return WiFi.SSID(); }
