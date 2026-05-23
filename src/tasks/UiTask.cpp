#include "tasks/UiTask.h"

#include "config/AppConfig.h"
#include "drivers/ButtonInput.h"
#include "network/WiFiService.h"
#include "ui/MenuController.h"
#include "utils/SerialLog.h"

void ui_task(void* parameter) {
  auto* app = static_cast<AppContext*>(parameter);
  TickType_t lastWake = xTaskGetTickCount();
  TickType_t lastRenderWake = lastWake;

  for (;;) {
    ButtonEvent buttonEvent;
    const uint32_t nowMs = millis();
    if (app->services.buttons->poll(buttonEvent, nowMs)) {
      LOGI("Button event id=%s type=%s", toString(buttonEvent.id), toString(buttonEvent.type));
      if (xSemaphoreTake(app->sync.stateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        app->services.menu->handleButton(buttonEvent);
        xSemaphoreGive(app->sync.stateMutex);
      }
    }

    if (xSemaphoreTake(app->sync.stateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
      if (app->state.wifiPortalRequested) {
        app->services.wifi->requestPortal();
        app->state.wifiPortalRequested = false;
      }

      if (app->state.bpState == BpState::COMPLETE && app->state.uiScreen == UiScreen::BP_PROGRESS) {
        app->state.uiScreen = UiScreen::LIVE_MONITOR;
      }
      xSemaphoreGive(app->sync.stateMutex);
    }

    if ((xTaskGetTickCount() - lastRenderWake) >= pdMS_TO_TICKS(AppConfig::UI_REFRESH_PERIOD_MS)) {
      app->services.menu->render();
      lastRenderWake = xTaskGetTickCount();
    }

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(AppConfig::BUTTON_POLL_PERIOD_MS));
  }
}
