#include "network/WiFiService.h"

#include <time.h>

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
  manager_.setConfigPortalBlocking(false);
  fallbackAttempted_ = false;
  portalActive_ = false;
  startupPortalAttempted_ = false;
  timeSyncConfigured_ = false;
  startupConnectStartedMs_ = millis();
  lastReconnectAttemptMs_ = 0;

  const bool hasSavedCredentials = manager_.getWiFiIsSaved();
  portalRequested_ = !hasSavedCredentials;
  if (hasSavedCredentials) {
    WiFi.begin();
    LOGI("WiFiService initialized, saved WiFi credentials found");
  } else {
    LOGI("WiFiService initialized, no saved credentials: setup portal scheduled");
  }
}

void WiFiService::maintain() {
  if (portalActive_) {
    portalRequested_ = false;
    const bool connectedFromPortal = manager_.process();
    if (WiFi.status() == WL_CONNECTED) {
      LOGI("WiFi connected from setup portal ssid=%s ip=%s", WiFi.SSID().c_str(),
           WiFi.localIP().toString().c_str());
      stopPortal_();
      return;
    }
    if (connectedFromPortal) {
      LOGI("WiFiManager reports portal connection complete");
    }
    if ((millis() - portalStartedMs_) >= AppConfig::WIFI_PORTAL_TIMEOUT_MS) {
      LOGW("WiFi setup portal timed out");
      stopPortal_();
      return;
    }
    return;
  }

  if (portalRequested_) {
    portalRequested_ = false;
    startupPortalAttempted_ = true;
    startPortal_();
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (!timeSyncConfigured_) {
      configTime(0, 0, "pool.ntp.org", "time.nist.gov");
      timeSyncConfigured_ = true;
      LOGI("UTC time sync configured");
    }
    return;
  }

  if (!startupPortalAttempted_ &&
      (millis() - startupConnectStartedMs_) >= AppConfig::WIFI_STARTUP_CONNECT_TIMEOUT_MS) {
    startupPortalAttempted_ = true;
    LOGW("WiFi startup connect timed out, opening setup portal");
    startPortal_();
    return;
  }

#if defined(DEBUG_WIFI_FALLBACK_SSID) && defined(DEBUG_WIFI_FALLBACK_PASSWORD)
  if (!fallbackAttempted_) {
    fallbackAttempted_ = true;
    if (tryDebugFallbackConnect()) {
      return;
    }
  }
#endif

  const uint32_t nowMs = millis();
  if ((nowMs - lastReconnectAttemptMs_) >= AppConfig::WIFI_RECONNECT_PERIOD_MS) {
    lastReconnectAttemptMs_ = nowMs;
    LOGW("WiFi disconnected, attempting reconnect status=%s", wifiStatusToString(WiFi.status()));
    WiFi.reconnect();
  }
}

bool WiFiService::isConnected() const { return WiFi.status() == WL_CONNECTED; }

bool WiFiService::isPortalActive() const { return portalActive_; }

void WiFiService::requestPortal() {
  if (portalActive_) {
    LOGI("WiFi setup portal already active ssid=%s ap_ip=%s", AppConfig::WIFI_PORTAL_AP_NAME,
         WiFi.softAPIP().toString().c_str());
    return;
  }
  portalRequested_ = true;
  LOGI("WiFi captive portal requested from UI");
}

String WiFiService::ipAddress() const { return WiFi.localIP().toString(); }

String WiFiService::ssid() const { return WiFi.SSID(); }

bool WiFiService::startPortal_() {
  if (portalActive_) {
    return true;
  }

  LOGI("Starting WiFiManager config portal ssid=%s", AppConfig::WIFI_PORTAL_AP_NAME);
  LOGI("WiFi status before portal: %s", wifiStatusToString(WiFi.status()));

  WiFi.disconnect(false);
  WiFi.mode(WIFI_AP);
  delay(100);

  const bool started = manager_.startConfigPortal(AppConfig::WIFI_PORTAL_AP_NAME);
  portalActive_ = manager_.getConfigPortalActive();
  portalStartedMs_ = millis();

  LOGI("WiFiManager portal started=%d active=%d ap_ip=%s mac=%s", started, portalActive_,
       WiFi.softAPIP().toString().c_str(), WiFi.softAPmacAddress().c_str());
  return portalActive_;
}

void WiFiService::stopPortal_() {
  if (manager_.getConfigPortalActive()) {
    manager_.stopConfigPortal();
  }
  portalActive_ = false;
  WiFi.mode(WIFI_STA);
  LOGI("WiFiManager portal stopped status=%s ip=%s", wifiStatusToString(WiFi.status()),
       WiFi.localIP().toString().c_str());
}
