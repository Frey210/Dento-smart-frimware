#include <Arduino.h>
#include <Wire.h>

#include "config/AppConfig.h"
#include "drivers/ButtonInput.h"
#include "drivers/DisplayManager.h"
#include "drivers/Ds18b20Sensor.h"
#include "drivers/GsrSensor.h"
#include "drivers/Hx710bSensor.h"
#include "drivers/PneumaticController.h"
#include "drivers/StorageManager.h"
#include "drivers/Max30102Sensor.h"
#include "models/AppContext.h"
#include "network/ApiClient.h"
#include "network/WiFiService.h"
#include "tasks/BpTask.h"
#include "tasks/LoggerTask.h"
#include "tasks/SensorTask.h"
#include "tasks/StatusTask.h"
#include "tasks/UiTask.h"
#include "tasks/UploadTask.h"
#include "tasks/WifiTask.h"
#include "ui/MenuController.h"
#include "utils/SerialLog.h"

namespace {
AppContext app;

GsrSensor gsrSensor(AppConfig::PIN_GSR);
Max30102Sensor max30102Sensor;
Ds18b20Sensor ds18b20Sensor(AppConfig::PIN_DS18B20);
Hx710bSensor pressureSensor(AppConfig::PIN_HX710B_SCK, AppConfig::PIN_HX710B_DOUT);
PneumaticController pneumaticController(AppConfig::PIN_PUMP, AppConfig::PIN_VALVE, pressureSensor);
ButtonInput buttonInput(AppConfig::PIN_BUTTON_UP, AppConfig::PIN_BUTTON_DOWN,
                        AppConfig::PIN_BUTTON_SELECT, AppConfig::PIN_BUTTON_BACK);
DisplayManager displayManager;
WiFiService wifiService;
ApiClient apiClient(AppConfig::API_ENDPOINT);
StorageManager storageManager;
MenuController* menuController = nullptr;

bool initializeRtObjects(AppContext& ctx) {
  ctx.queues.sensorQueue = xQueueCreate(1, sizeof(SensorSnapshot));
  ctx.queues.loggerQueue = xQueueCreate(AppConfig::LOGGER_QUEUE_LENGTH, sizeof(UploadPayload));
  ctx.queues.buttonQueue = xQueueCreate(AppConfig::BUTTON_QUEUE_LENGTH, sizeof(ButtonEvent));
  ctx.sync.i2cMutex = xSemaphoreCreateMutex();
  ctx.sync.stateMutex = xSemaphoreCreateMutex();
  ctx.sync.eventGroup = xEventGroupCreate();

  return ctx.queues.sensorQueue != nullptr && ctx.queues.loggerQueue != nullptr &&
         ctx.queues.buttonQueue != nullptr && ctx.sync.i2cMutex != nullptr &&
         ctx.sync.stateMutex != nullptr && ctx.sync.eventGroup != nullptr;
}

void createTasks(AppContext& ctx) {
  xTaskCreate(sensor_task, "sensor_task", AppConfig::TASK_STACK_SENSOR, &ctx,
              AppConfig::TASK_PRIORITY_SENSOR, nullptr);
  xTaskCreate(ui_task, "ui_task", AppConfig::TASK_STACK_UI, &ctx, AppConfig::TASK_PRIORITY_UI,
              nullptr);
  xTaskCreate(bp_task, "bp_task", AppConfig::TASK_STACK_BP, &ctx, AppConfig::TASK_PRIORITY_BP,
              nullptr);
  xTaskCreate(wifi_task, "wifi_task", AppConfig::TASK_STACK_WIFI, &ctx,
              AppConfig::TASK_PRIORITY_WIFI, nullptr);
  xTaskCreate(upload_task, "upload_task", AppConfig::TASK_STACK_UPLOAD, &ctx,
              AppConfig::TASK_PRIORITY_UPLOAD, nullptr);
  xTaskCreate(logger_task, "logger_task", AppConfig::TASK_STACK_LOGGER, &ctx,
              AppConfig::TASK_PRIORITY_LOGGER, nullptr);
  xTaskCreate(status_task, "status_task", AppConfig::TASK_STACK_STATUS, &ctx,
              AppConfig::TASK_PRIORITY_STATUS, nullptr);
}
}  // namespace

void setup() {
  Serial.begin(AppConfig::SERIAL_BAUD);
  const uint32_t serialWaitStarted = millis();
  while (!Serial && (millis() - serialWaitStarted) < 3000UL) {
    delay(10);
  }
  delay(200);
  Wire.begin(AppConfig::PIN_I2C_SDA, AppConfig::PIN_I2C_SCL, AppConfig::I2C_FREQUENCY);
  analogReadResolution(12);

  LOGI("Booting %s", AppConfig::DEVICE_UID);
  LOGI("API endpoint: %s", AppConfig::API_ENDPOINT);

  if (!initializeRtObjects(app)) {
    LOGE("Failed to create RTOS objects");
    for (;;) {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  gsrSensor.begin();
  ds18b20Sensor.begin();
  pressureSensor.begin();
  pneumaticController.begin();
  buttonInput.begin();
  if (xSemaphoreTake(app.sync.i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    displayManager.begin();
    xSemaphoreGive(app.sync.i2cMutex);
  }
  max30102Sensor.setBusMutex(app.sync.i2cMutex);
  max30102Sensor.begin(Wire);
  storageManager.begin();
  wifiService.begin();

  app.services.gsr = &gsrSensor;
  app.services.max30102 = &max30102Sensor;
  app.services.ds18b20 = &ds18b20Sensor;
  app.services.hx710b = &pressureSensor;
  app.services.pneumatic = &pneumaticController;
  app.services.buttons = &buttonInput;
  app.services.display = &displayManager;
  app.services.wifi = &wifiService;
  app.services.api = &apiClient;
  app.services.storage = &storageManager;

  menuController = new MenuController(app);
  app.services.menu = menuController;

  app.state.latestSensor.batteryLevel = AppConfig::DEFAULT_BATTERY_LEVEL;
  createTasks(app);
  LOGI("Dento Smart scaffold initialized");
}

void loop() { vTaskDelay(pdMS_TO_TICKS(1000)); }
