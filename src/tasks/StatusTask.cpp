#include "tasks/StatusTask.h"

#include <WiFi.h>

#include "config/AppConfig.h"
#include "models/SystemEvents.h"
#include "utils/SerialLog.h"

void status_task(void* parameter) {
  auto* app = static_cast<AppContext*>(parameter);
  TickType_t lastWake = xTaskGetTickCount();

  for (;;) {
    SharedRuntimeState stateCopy;
    if (xSemaphoreTake(app->sync.stateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
      stateCopy = app->state;
      xSemaphoreGive(app->sync.stateMutex);
    }

    const EventBits_t bits = xEventGroupGetBits(app->sync.eventGroup);
    LOGI(
        "STATUS heap=%u min_heap=%u wifi=%s ip=%s rssi=%d ui=%s bp=%s progress=%u sensor_q=%u "
        "logger_q=%u bits=0x%02x gsr=%.3f hr=%d temp=%.2f cuff=%.2f anx=%.2f",
        static_cast<unsigned>(ESP.getFreeHeap()), static_cast<unsigned>(ESP.getMinFreeHeap()),
        (bits & SystemEvents::WIFI_CONNECTED) ? "connected" : "down",
        WiFi.localIP().toString().c_str(), WiFi.RSSI(), toString(stateCopy.uiScreen),
        toString(stateCopy.bpState), stateCopy.bpProgress,
        static_cast<unsigned>(uxQueueMessagesWaiting(app->queues.sensorQueue)),
        static_cast<unsigned>(uxQueueMessagesWaiting(app->queues.loggerQueue)),
        static_cast<unsigned>(bits), stateCopy.latestSensor.gsr, stateCopy.latestSensor.heartRate,
        stateCopy.latestSensor.temperature, stateCopy.latestSensor.cuffPressure,
        stateCopy.anxietyScore);

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(AppConfig::STATUS_PERIOD_MS));
  }
}
