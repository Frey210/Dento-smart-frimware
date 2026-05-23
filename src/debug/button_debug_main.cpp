#include <Arduino.h>

#include "config/AppConfig.h"
#include "drivers/ButtonInput.h"

namespace {
ButtonInput buttonInput(AppConfig::PIN_BUTTON_UP, AppConfig::PIN_BUTTON_DOWN,
                        AppConfig::PIN_BUTTON_SELECT, AppConfig::PIN_BUTTON_BACK);

void printButtonMap() {
  Serial.println();
  Serial.println("Button debug mode");
  Serial.printf("UP     -> GPIO %u\r\n", AppConfig::PIN_BUTTON_UP);
  Serial.printf("DOWN   -> GPIO %u\r\n", AppConfig::PIN_BUTTON_DOWN);
  Serial.printf("SELECT -> GPIO %u\r\n", AppConfig::PIN_BUTTON_SELECT);
  Serial.printf("BACK   -> GPIO %u\r\n", AppConfig::PIN_BUTTON_BACK);
  Serial.println("Idle state uses INPUT_PULLUP, press should pull pin to GND.");
  Serial.println("Waiting for button events...");
}
}  // namespace

void setup() {
  Serial.begin(AppConfig::SERIAL_BAUD);
  const uint32_t serialWaitStarted = millis();
  while (!Serial && (millis() - serialWaitStarted) < 3000UL) {
    delay(10);
  }
  delay(200);

  buttonInput.begin();
  printButtonMap();
}

void loop() {
  ButtonEvent event;
  if (buttonInput.poll(event, millis())) {
    Serial.printf("[%10lu ms] %-6s %s\r\n", event.eventMs, toString(event.id),
                  toString(event.type));
  }
  delay(5);
}
