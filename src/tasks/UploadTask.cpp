#include "tasks/UploadTask.h"

#include "config/AppConfig.h"
#include "drivers/PneumaticController.h"
#include "models/SystemEvents.h"
#include "network/ApiClient.h"
#include "utils/SerialLog.h"

void upload_task(void* parameter) {
  auto* app = static_cast<AppContext*>(parameter);
  SensorSnapshot snapshot;
  uint32_t lastUploadMs = 0;

  for (;;) {
    if (xQueueReceive(app->queues.sensorQueue, &snapshot, pdMS_TO_TICKS(250)) != pdTRUE) {
      continue;
    }

    const uint32_t nowMs = millis();
    if ((nowMs - lastUploadMs) < AppConfig::UPLOAD_PERIOD_MS) {
      continue;
    }
    lastUploadMs = nowMs;

    snapshot.bp = app->services.pneumatic->reading();
    const String payload = app->services.api->buildPayload(snapshot);
    bool success = false;
    LOGI("Upload cycle started, payload=%s", payload.c_str());

    if ((xEventGroupGetBits(app->sync.eventGroup) & SystemEvents::WIFI_CONNECTED) != 0) {
      for (uint8_t attempt = 0; attempt < AppConfig::HTTP_RETRY_COUNT; ++attempt) {
        LOGI("Upload attempt %u/%u", static_cast<unsigned>(attempt + 1),
             static_cast<unsigned>(AppConfig::HTTP_RETRY_COUNT));
        success = app->services.api->postPayload(payload);
        if (success) {
          LOGI("Upload success");
          break;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
      }
    } else {
      LOGW("Upload deferred: WiFi not connected");
    }

    if (!success) {
      UploadPayload queued;
      payload.toCharArray(queued.json, sizeof(queued.json));
      xQueueSend(app->queues.loggerQueue, &queued, pdMS_TO_TICKS(20));
      LOGW("Payload queued to offline logger");
    }
  }
}
