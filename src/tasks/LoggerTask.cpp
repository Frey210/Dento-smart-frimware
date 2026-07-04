#include "tasks/LoggerTask.h"

#include "config/AppConfig.h"
#include "drivers/StorageManager.h"
#include "models/SystemEvents.h"
#include "network/ApiClient.h"
#include "utils/SerialLog.h"

void logger_task(void* parameter) {
  auto* app = static_cast<AppContext*>(parameter);
  TickType_t lastWake = xTaskGetTickCount();

  for (;;) {
    UploadPayload queuedPayload;
    uint8_t storedCount = 0;
    while (xQueueReceive(app->queues.loggerQueue, &queuedPayload, 0) == pdTRUE) {
      const bool stored = app->services.storage->appendOfflinePayload(String(queuedPayload.json));
      if (stored) {
        ++storedCount;
      }
    }
    if (storedCount > 0) {
      LOGI("Offline payloads persisted=%u", static_cast<unsigned>(storedCount));
    }

    if ((xEventGroupGetBits(app->sync.eventGroup) & SystemEvents::WIFI_CONNECTED) != 0 &&
        app->services.storage->hasOfflinePayload()) {
      String offlinePayload;
      if (app->services.storage->popOldestOfflinePayload(offlinePayload)) {
        LOGI("Replaying offline payload: %s", offlinePayload.c_str());
        if (!app->services.api->postPayload(offlinePayload)) {
          app->services.storage->appendOfflinePayload(offlinePayload);
          LOGW("Offline replay failed, payload returned to storage");
        } else {
          LOGI("Offline replay success");
        }
      }
    }

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(AppConfig::LOGGER_PERIOD_MS));
  }
}
