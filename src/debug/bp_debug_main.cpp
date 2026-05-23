#include <Arduino.h>

#include "config/AppConfig.h"
#include "debug/DebugHelpers.h"
#include "drivers/Hx710bSensor.h"
#include "drivers/PneumaticController.h"

namespace {
Hx710bSensor pressureSensor(AppConfig::PIN_HX710B_SCK, AppConfig::PIN_HX710B_DOUT);
PneumaticController pneumaticController(AppConfig::PIN_PUMP, AppConfig::PIN_VALVE, pressureSensor);

bool autoMode = false;
bool pumpManual = false;
bool valveManual = false;
uint32_t lastPrintMs = 0;
uint32_t lastUpdateMs = 0;
uint32_t lastStatePrintMs = 0;
uint32_t lastManualPressureSampleMs = 0;
float lastManualPressure = 0.0f;
BpState lastState = BpState::IDLE;

void printHelp() {
  DebugHelpers::printDivider();
  Serial.println("BP debug commands:");
  Serial.println("  h : help");
  Serial.println("  a : start auto measurement state machine");
  Serial.println("  x : stop all outputs and exit auto mode");
  Serial.println("  p : toggle pump manual");
  Serial.println("  v : toggle valve manual");
  Serial.println("  s : print current status");
  DebugHelpers::printDivider();
}

void setPumpManual(bool enabled) {
  digitalWrite(AppConfig::PIN_PUMP, enabled ? HIGH : LOW);
  pumpManual = enabled;
}

void setValveManual(bool enabled) {
  digitalWrite(AppConfig::PIN_VALVE, enabled ? HIGH : LOW);
  valveManual = enabled;
}

void stopAllManual() {
  setPumpManual(false);
  setValveManual(false);
}

float samplePressure() {
  lastManualPressure = pressureSensor.readPressureMmhg();
  lastManualPressureSampleMs = millis();
  return lastManualPressure;
}

void printStatus() {
  const float pressure = autoMode ? pneumaticController.currentPressure() : samplePressure();
  const BloodPressureReading reading = pneumaticController.reading();
  Serial.printf("mode=%s state=%s pressure=%.2f mmHg pump=%d valve=%d progress=%u valid=%d sys=%d dia=%d\r\n",
                autoMode ? "AUTO" : "MANUAL", toString(pneumaticController.state()), pressure,
                pumpManual, valveManual, pneumaticController.progress(), reading.valid,
                reading.systolic, reading.diastolic);
}

void handleCommand(char command) {
  switch (command) {
    case 'h':
    case '?':
      printHelp();
      break;
    case 'a':
      stopAllManual();
      autoMode = true;
      pneumaticController.requestMeasurement();
      Serial.println("Auto measurement started");
      break;
    case 'x':
      autoMode = false;
      pneumaticController.stop();
      stopAllManual();
      Serial.println("All outputs stopped");
      break;
    case 'p':
      if (autoMode) {
        Serial.println("Disable auto mode first with 'x'");
        break;
      }
      setPumpManual(!pumpManual);
      Serial.printf("Pump manual=%d\r\n", pumpManual);
      break;
    case 'v':
      if (autoMode) {
        Serial.println("Disable auto mode first with 'x'");
        break;
      }
      setValveManual(!valveManual);
      Serial.printf("Valve manual=%d\r\n", valveManual);
      break;
    case 's':
      printStatus();
      break;
    default:
      if (command != '\r' && command != '\n') {
        Serial.printf("Unknown command: %c\r\n", command);
      }
      break;
  }
}
}  // namespace

void setup() {
  DebugHelpers::waitForSerial();
  pressureSensor.begin();
  pneumaticController.begin();
  stopAllManual();

  Serial.println("Blood pressure debug mode");
  Serial.printf("Pump  L9110 A1-A -> GPIO %u, A1-B -> GND\r\n", AppConfig::PIN_PUMP);
  Serial.printf("Valve L9110 B1-A -> GPIO %u, B1-B -> GND\r\n", AppConfig::PIN_VALVE);
  Serial.printf("Pressure sensor HX710B SCK=%u DOUT=%u\r\n", AppConfig::PIN_HX710B_SCK,
                AppConfig::PIN_HX710B_DOUT);
  Serial.println("Flow: pump -> cuff -> T joint(pressure sensor) -> valve -> exhaust");
  printHelp();
}

void loop() {
  while (Serial.available() > 0) {
    handleCommand(static_cast<char>(Serial.read()));
  }

  const uint32_t nowMs = millis();
  if (autoMode && (nowMs - lastUpdateMs) >= AppConfig::BP_LOOP_PERIOD_MS) {
    pneumaticController.update(nowMs);
    lastUpdateMs = nowMs;

    const BpState currentState = pneumaticController.state();
    if (currentState != lastState) {
      const BloodPressureReading reading = pneumaticController.reading();
      Serial.printf("State %s -> %s pressure=%.2f progress=%u valid=%d sys=%d dia=%d\r\n",
                    toString(lastState), toString(currentState), pneumaticController.currentPressure(),
                    pneumaticController.progress(), reading.valid, reading.systolic,
                    reading.diastolic);
      lastState = currentState;
    }
  }

  if ((nowMs - lastPrintMs) >= 500UL) {
    const float pressure = autoMode ? pneumaticController.currentPressure() : samplePressure();
    if (!autoMode || (nowMs - lastStatePrintMs) >= 1000UL) {
      Serial.printf("[%10lu ms] mode=%s pressure=%.2f mmHg pump=%d valve=%d state=%s progress=%u\r\n",
                    nowMs, autoMode ? "AUTO" : "MANUAL", pressure, pumpManual, valveManual,
                    toString(pneumaticController.state()), pneumaticController.progress());
      lastStatePrintMs = nowMs;
    }
    lastPrintMs = nowMs;
  }

  if (!autoMode && !pumpManual && !valveManual && (nowMs - lastManualPressureSampleMs) > 2000UL) {
    samplePressure();
  }

  delay(10);
}
