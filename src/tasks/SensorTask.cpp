#include "tasks/SensorTask.h"

#include "config/AppConfig.h"
#include "drivers/Ds18b20Sensor.h"
#include "drivers/GsrSensor.h"
#include "drivers/Max30102Sensor.h"
#include "drivers/PneumaticController.h"
#include "utils/SerialLog.h"

void sensor_task(void* parameter) {
  auto* app = static_cast<AppContext*>(parameter);
  TickType_t lastWake = xTaskGetTickCount();
  uint32_t lastLogMs = 0;
  uint32_t lastWarningMs = 0;

  for (;;) {
    const uint32_t nowMs = millis();
    app->services.ds18b20->update(nowMs);
    app->services.max30102->update(nowMs);

    SensorSnapshot snapshot;
    snapshot.sampleMs = nowMs;
    snapshot.gsr = app->services.gsr->readNormalized();
    snapshot.heartRate = app->services.max30102->latestHeartRate();
    const bool temperatureValid = app->services.ds18b20->hasValidReading();
    snapshot.temperature = temperatureValid ? app->services.ds18b20->latestCelsius() : 0.0f;
    snapshot.cuffPressure = app->services.pneumatic->currentPressure();
    snapshot.batteryLevel = AppConfig::DEFAULT_BATTERY_LEVEL;
    snapshot.bp = app->services.pneumatic->reading();

    if (xSemaphoreTake(app->sync.stateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
      app->state.latestSensor = snapshot;
      // TODO: Replace placeholder anxiety score with a validated multimodal model.
      const float temperatureScore = temperatureValid ? (snapshot.temperature / 40.0f) * 0.2f : 0.0f;
      app->state.anxietyScore =
          min(1.0f, (snapshot.gsr * 0.5f) + (snapshot.heartRate / 180.0f) * 0.3f +
                        temperatureScore);
      xSemaphoreGive(app->sync.stateMutex);
    }

    xQueueOverwrite(app->queues.sensorQueue, &snapshot);
    if (!temperatureValid && (nowMs - lastWarningMs) >= AppConfig::STATUS_PERIOD_MS) {
      LOGW("DS18B20 temperature invalid. devices=%u value=%.2f C. Check wiring/sensor presence.",
           app->services.ds18b20->deviceCount(), app->services.ds18b20->latestCelsius());
      lastWarningMs = nowMs;
    }

    if ((nowMs - lastLogMs) >= AppConfig::SENSOR_LOG_PERIOD_MS) {
      LOGI("Sensor snapshot gsr=%.3f hr=%d ir=%lu finger=%d temp=%.2f temp_valid=%d cuff=%.2f batt=%u bp=%s",
           snapshot.gsr, snapshot.heartRate,
           static_cast<unsigned long>(app->services.max30102->latestIr()),
           app->services.max30102->fingerDetected(), snapshot.temperature,
           temperatureValid, snapshot.cuffPressure, snapshot.batteryLevel,
           snapshot.bp.valid ? "valid" : "null");
      lastLogMs = nowMs;
    }
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(AppConfig::SENSOR_SAMPLE_PERIOD_MS));
  }
}
