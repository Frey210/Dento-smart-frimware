#include "drivers/Hx710bSensor.h"

Hx710bSensor::Hx710bSensor(uint8_t clockPin, uint8_t dataPin)
    : clockPin_(clockPin), dataPin_(dataPin) {}

void Hx710bSensor::begin() {
  pinMode(clockPin_, OUTPUT);
  pinMode(dataPin_, INPUT);
  digitalWrite(clockPin_, LOW);
  if (isReady()) {
    offset_ = readRaw();
  }
}

bool Hx710bSensor::isReady() const { return digitalRead(dataPin_) == LOW; }

int32_t Hx710bSensor::readRaw() {
  while (!isReady()) {
    taskYIELD();
  }

  int32_t value = 0;
  for (uint8_t bit = 0; bit < 24; ++bit) {
    digitalWrite(clockPin_, HIGH);
    ets_delay_us(1);
    value = (value << 1) | digitalRead(dataPin_);
    digitalWrite(clockPin_, LOW);
    ets_delay_us(1);
  }

  digitalWrite(clockPin_, HIGH);
  ets_delay_us(1);
  digitalWrite(clockPin_, LOW);

  if (value & 0x800000L) {
    value |= ~0xFFFFFFL;
  }
  return value;
}

float Hx710bSensor::readPressureMmhg() {
  const int32_t raw = readRaw() - offset_;
  return static_cast<float>(raw) / scale_;
}

