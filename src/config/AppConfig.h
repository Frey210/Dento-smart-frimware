#pragma once

#include <Arduino.h>

namespace AppConfig {
constexpr char DEVICE_UID[] = "ESP32-C3-020";
constexpr char DEVICE_KEY[] = "KEY-ESP32-C3-020";
constexpr char API_ENDPOINT[] =
    "https://dento-backend.inkubasistartupunhas.id/api/sensor-data";
constexpr char WIFI_AP_NAME[] = "Aerasea";
constexpr char WIFI_AP_PASSWORD[] = "Mtc28082023";
constexpr char WIFI_PORTAL_AP_NAME[] = "DentoSmart-Setup";

constexpr uint8_t PIN_PUMP = 5;
constexpr uint8_t PIN_VALVE = 6;

constexpr uint8_t PIN_BUTTON_UP = 7;
constexpr uint8_t PIN_BUTTON_DOWN = 10;
constexpr uint8_t PIN_BUTTON_SELECT = 20;
constexpr uint8_t PIN_BUTTON_BACK = 21;

constexpr uint8_t PIN_I2C_SDA = 8;
constexpr uint8_t PIN_I2C_SCL = 9;

constexpr uint8_t PIN_HX710B_SCK = 4;
constexpr uint8_t PIN_HX710B_DOUT = 2;
constexpr uint8_t PIN_GSR = 3;
constexpr uint8_t PIN_DS18B20 = 1;

constexpr uint8_t PIN_RESERVED = 0;

constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint32_t I2C_FREQUENCY = 400000;

constexpr uint32_t SENSOR_SAMPLE_PERIOD_MS = 200;
constexpr uint32_t UI_REFRESH_PERIOD_MS = 100;
constexpr uint32_t WIFI_HEALTHCHECK_PERIOD_MS = 1000;
constexpr uint32_t WIFI_STARTUP_CONNECT_TIMEOUT_MS = 15000;
constexpr uint32_t WIFI_PORTAL_TIMEOUT_MS = 300000;
constexpr uint32_t WIFI_RECONNECT_PERIOD_MS = 5000;
constexpr uint32_t UPLOAD_PERIOD_MS = 1000;
constexpr uint32_t OFFLINE_QUEUE_PERIOD_MS = 15000;
constexpr uint32_t LOGGER_PERIOD_MS = 5000;
constexpr uint32_t STATUS_PERIOD_MS = 5000;
constexpr uint32_t SENSOR_LOG_PERIOD_MS = 15000;
constexpr uint32_t BUTTON_POLL_PERIOD_MS = 20;
constexpr uint32_t BP_LOOP_PERIOD_MS = 50;
constexpr uint32_t DS18B20_PERIOD_MS = 1000;
constexpr uint32_t HEART_RATE_PERIOD_MS = 40;

constexpr uint32_t HTTP_TIMEOUT_MS = 10000;
constexpr uint8_t HTTP_RETRY_COUNT = 3;

constexpr size_t SENSOR_QUEUE_LENGTH = 8;
constexpr size_t LOGGER_QUEUE_LENGTH = 12;
constexpr size_t BUTTON_QUEUE_LENGTH = 12;

constexpr uint16_t TASK_STACK_SENSOR = 4096;
constexpr uint16_t TASK_STACK_UI = 6144;
constexpr uint16_t TASK_STACK_BP = 4096;
constexpr uint16_t TASK_STACK_WIFI = 6144;
constexpr uint16_t TASK_STACK_UPLOAD = 6144;
constexpr uint16_t TASK_STACK_LOGGER = 4096;
constexpr uint16_t TASK_STACK_STATUS = 4096;

constexpr UBaseType_t TASK_PRIORITY_SENSOR = 3;
constexpr UBaseType_t TASK_PRIORITY_UI = 2;
constexpr UBaseType_t TASK_PRIORITY_BP = 3;
constexpr UBaseType_t TASK_PRIORITY_WIFI = 2;
constexpr UBaseType_t TASK_PRIORITY_UPLOAD = 2;
constexpr UBaseType_t TASK_PRIORITY_LOGGER = 1;
constexpr UBaseType_t TASK_PRIORITY_STATUS = 1;

constexpr uint16_t BP_TARGET_PRESSURE_MMHG = 160;
constexpr uint16_t BP_HOLD_DURATION_MS = 1500;
constexpr uint16_t BP_MEASUREMENT_TIMEOUT_MS = 45000;

constexpr uint8_t DEFAULT_BATTERY_LEVEL = 87;
constexpr char OFFLINE_LOG_FILE[] = "/offline_queue.log";
}  // namespace AppConfig
