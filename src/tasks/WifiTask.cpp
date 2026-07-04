#include "tasks/WifiTask.h"

#include "config/AppConfig.h"
#include "models/SystemEvents.h"
#include "network/WiFiService.h"
#include "utils/SerialLog.h"

void wifi_task(void* parameter) {
  auto* app = static_cast<AppContext*>(parameter);
  TickType_t lastWake = xTaskGetTickCount();
  bool wasConnected = false;
  uint32_t lastStatusLogMs = 0;

  for (;;) {
    app->services.wifi->maintain();
    const bool connected = app->services.wifi->isConnected();
    if (connected) {
      xEventGroupSetBits(app->sync.eventGroup, SystemEvents::WIFI_CONNECTED);
    } else {
      xEventGroupClearBits(app->sync.eventGroup, SystemEvents::WIFI_CONNECTED);
    }

    if (connected != wasConnected) {
      if (connected) {
        LOGI("WiFi connected ssid=%s ip=%s", app->services.wifi->ssid().c_str(),
             app->services.wifi->ipAddress().c_str());
      } else {
        LOGW("WiFi connection lost");
      }
      wasConnected = connected;
    }

    const uint32_t nowMs = millis();
    if (!connected && !app->services.wifi->isPortalActive() &&
        (nowMs - lastStatusLogMs) >= AppConfig::STATUS_PERIOD_MS) {
      LOGI("WiFi still connecting/reconnecting...");
      lastStatusLogMs = nowMs;
    }

    if (app->services.wifi->isPortalActive()) {
      vTaskDelay(pdMS_TO_TICKS(50));
      lastWake = xTaskGetTickCount();
    } else {
      vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(AppConfig::WIFI_HEALTHCHECK_PERIOD_MS));
    }
  }
}
