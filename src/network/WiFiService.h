#pragma once

#include <Arduino.h>
#include <WiFiManager.h>

class WiFiService {
 public:
  void begin();
  void maintain();
  bool isConnected() const;
  bool isPortalActive() const;
  void requestPortal();
  String ipAddress() const;
  String ssid() const;
  String statusLabel() const;

 private:
  bool startPortal_();
  void stopPortal_();

  bool portalRequested_ = false;
  bool portalActive_ = false;
  bool startupPortalAttempted_ = false;
  bool timeSyncConfigured_ = false;
  bool connecting_ = false;
  uint32_t startupConnectStartedMs_ = 0;
  uint32_t portalStartedMs_ = 0;
  uint32_t lastReconnectAttemptMs_ = 0;
  WiFiManager manager_;
};
