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

}  // namespace

void WiFiService::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.setHostname(AppConfig::DEVICE_UID);
  manager_.setConnectTimeout(30);
  manager_.setConfigPortalBlocking(false);
  portalActive_ = false;
  startupPortalAttempted_ = false;
  timeSyncConfigured_ = false;
  connecting_ = false;
  startupConnectStartedMs_ = millis();
  lastReconnectAttemptMs_ = 0;

  const bool hasSavedCredentials = manager_.getWiFiIsSaved();
  portalRequested_ = !hasSavedCredentials;
  if (hasSavedCredentials) {
    WiFi.begin();
    connecting_ = true;
    LOGI("WiFiService initialized, saved WiFi credentials found");
  } else {
    LOGI("WiFiService initialized, no saved credentials: setup portal scheduled");
  }
}

void WiFiService::maintain() {
  if (portalActive_) {
    portalRequested_ = false;
    const bool connectedFromPortal = manager_.process();
    portalActive_ = manager_.getConfigPortalActive();
    if (WiFi.status() == WL_CONNECTED) {
      LOGI("WiFi connected from setup portal ssid=%s ip=%s", WiFi.SSID().c_str(),
           WiFi.localIP().toString().c_str());
      stopPortal_();
      return;
    }
    if (!portalActive_) {
      connecting_ = true;
      startupConnectStartedMs_ = millis();
      LOGI("WiFi setup portal closed, waiting for STA connection status=%s",
           wifiStatusToString(WiFi.status()));
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
    connecting_ = false;
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
    connecting_ = false;
    manager_.resetSettings();
    WiFi.disconnect(true, true);
    LOGW("WiFi startup connect timed out, saved credentials cleared, opening setup portal");
    startPortal_();
    return;
  }

  const uint32_t nowMs = millis();
  if (!connecting_ && (nowMs - lastReconnectAttemptMs_) >= AppConfig::WIFI_RECONNECT_PERIOD_MS) {
    lastReconnectAttemptMs_ = nowMs;
    LOGW("WiFi disconnected, attempting reconnect status=%s", wifiStatusToString(WiFi.status()));
    WiFi.reconnect();
    connecting_ = true;
  } else if (connecting_ && (WiFi.status() == WL_CONNECT_FAILED ||
                             WiFi.status() == WL_NO_SSID_AVAIL ||
                             WiFi.status() == WL_DISCONNECTED)) {
    if ((nowMs - lastReconnectAttemptMs_) >= AppConfig::WIFI_RECONNECT_PERIOD_MS) {
      connecting_ = false;
    }
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
  manager_.resetSettings();
  WiFi.disconnect(true, true);
  startupPortalAttempted_ = true;
  connecting_ = false;
  portalRequested_ = true;
  LOGI("WiFi captive portal requested from UI, saved credentials cleared");
}

String WiFiService::ipAddress() const { return WiFi.localIP().toString(); }

String WiFiService::ssid() const { return WiFi.SSID(); }

String WiFiService::statusLabel() const {
  if (portalActive_) {
    return "SETUP";
  }
  if (connecting_) {
    return "CONNECTING";
  }
  if (WiFi.status() == WL_CONNECTED) {
    return "ONLINE";
  }
  return wifiStatusToString(WiFi.status());
}

bool WiFiService::startPortal_() {
  if (portalActive_) {
    return true;
  }

  LOGI("Starting WiFiManager config portal ssid=%s", AppConfig::WIFI_PORTAL_AP_NAME);
  LOGI("WiFi status before portal: %s", wifiStatusToString(WiFi.status()));

  WiFi.disconnect(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);
  connecting_ = false;
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
  connecting_ = (WiFi.status() != WL_CONNECTED);
  LOGI("WiFiManager portal stopped status=%s ip=%s", wifiStatusToString(WiFi.status()),
       WiFi.localIP().toString().c_str());
}
