#pragma once

#include <Arduino.h>
#include <WiFiManager.h>

class WiFiService {
 public:
  void begin();
  void maintain();
  bool isConnected() const;
  void requestPortal();
  String ipAddress() const;
  String ssid() const;

 private:
  bool portalRequested_ = false;
  bool fallbackAttempted_ = false;
  WiFiManager manager_;
};
