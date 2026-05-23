#include "tasks/BpTask.h"

#include "config/AppConfig.h"
#include "drivers/PneumaticController.h"
#include "models/SystemEvents.h"
#include "utils/SerialLog.h"

void bp_task(void* parameter) {
  auto* app = static_cast<AppContext*>(parameter);
  TickType_t lastWake = xTaskGetTickCount();
  BpState previousState = BpState::IDLE;

  for (;;) {
    const EventBits_t bits =
        xEventGroupGetBits(app->sync.eventGroup);
    if ((bits & SystemEvents::BP_REQUEST) != 0) {
      LOGI("Blood pressure measurement requested");
      app->services.pneumatic->requestMeasurement();
      xEventGroupClearBits(app->sync.eventGroup, SystemEvents::BP_REQUEST);
      xEventGroupSetBits(app->sync.eventGroup, SystemEvents::BP_RUNNING);
    }

    app->services.pneumatic->update(millis());
    const BpState currentState = app->services.pneumatic->state();
    if (currentState != previousState) {
      const BloodPressureReading reading = app->services.pneumatic->reading();
      LOGI("BP state changed: %s -> %s pressure=%.2f progress=%u sys=%d dia=%d valid=%d",
           toString(previousState), toString(currentState), app->services.pneumatic->currentPressure(),
           app->services.pneumatic->progress(), reading.systolic, reading.diastolic, reading.valid);
    }

    if (xSemaphoreTake(app->sync.stateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
      app->state.bpState = currentState;
      app->state.bpProgress = app->services.pneumatic->progress();
      app->state.latestSensor.cuffPressure = app->services.pneumatic->currentPressure();
      app->state.latestSensor.bp = app->services.pneumatic->reading();
      xSemaphoreGive(app->sync.stateMutex);
    }

    if (currentState == BpState::COMPLETE && previousState != BpState::COMPLETE) {
      LOGI("Blood pressure measurement complete");
      xEventGroupClearBits(app->sync.eventGroup, SystemEvents::BP_RUNNING);
      xEventGroupSetBits(app->sync.eventGroup, SystemEvents::BP_COMPLETE);
    } else if (currentState == BpState::IDLE || currentState == BpState::ERROR) {
      if (currentState == BpState::ERROR && previousState != BpState::ERROR) {
        LOGE("Blood pressure state entered ERROR");
      }
      xEventGroupClearBits(app->sync.eventGroup,
                           SystemEvents::BP_RUNNING | SystemEvents::BP_COMPLETE);
    }

    previousState = currentState;
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(AppConfig::BP_LOOP_PERIOD_MS));
  }
}
