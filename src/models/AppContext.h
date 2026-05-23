#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

#include "models/SensorData.h"

class GsrSensor;
class Max30102Sensor;
class Ds18b20Sensor;
class Hx710bSensor;
class PneumaticController;
class ButtonInput;
class DisplayManager;
class MenuController;
class WiFiService;
class ApiClient;
class StorageManager;

struct AppQueues {
  QueueHandle_t sensorQueue = nullptr;
  QueueHandle_t loggerQueue = nullptr;
  QueueHandle_t buttonQueue = nullptr;
};

struct AppSync {
  SemaphoreHandle_t i2cMutex = nullptr;
  SemaphoreHandle_t stateMutex = nullptr;
  EventGroupHandle_t eventGroup = nullptr;
};

struct AppServices {
  GsrSensor* gsr = nullptr;
  Max30102Sensor* max30102 = nullptr;
  Ds18b20Sensor* ds18b20 = nullptr;
  Hx710bSensor* hx710b = nullptr;
  PneumaticController* pneumatic = nullptr;
  ButtonInput* buttons = nullptr;
  DisplayManager* display = nullptr;
  MenuController* menu = nullptr;
  WiFiService* wifi = nullptr;
  ApiClient* api = nullptr;
  StorageManager* storage = nullptr;
};

struct AppContext {
  AppQueues queues;
  AppSync sync;
  AppServices services;
  SharedRuntimeState state;
};
