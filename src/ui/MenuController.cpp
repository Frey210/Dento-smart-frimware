#include "ui/MenuController.h"

#include "drivers/DisplayManager.h"
#include "models/SystemEvents.h"

MenuController::MenuController(AppContext& app) : app_(app) {}

void MenuController::handleButton(const ButtonEvent& event) {
  if (event.type != ButtonEventType::SHORT_PRESS) {
    return;
  }

  SharedRuntimeState& state = app_.state;
  if (state.uiScreen == UiScreen::MENU) {
    if (event.id == ButtonId::UP) {
      state.menuIndex = (state.menuIndex == 0) ? (kMenuItemCount - 1) : (state.menuIndex - 1);
    } else if (event.id == ButtonId::DOWN) {
      state.menuIndex = (state.menuIndex + 1) % kMenuItemCount;
    } else if (event.id == ButtonId::SELECT) {
      switch (state.menuIndex) {
        case 0:
          state.uiScreen = UiScreen::BP_PROGRESS;
          xEventGroupSetBits(app_.sync.eventGroup, SystemEvents::BP_REQUEST);
          break;
        case 1:
          state.uiScreen = UiScreen::LIVE_MONITOR;
          break;
        case 2:
          state.uiScreen = UiScreen::WIFI_SETUP;
          state.wifiPortalRequested = true;
          break;
        case 3:
          state.uiScreen = UiScreen::DEVICE_INFO;
          break;
        case 4:
          state.uiScreen = UiScreen::CALIBRATION;
          break;
      }
    }
    return;
  }

  if (event.id == ButtonId::BACK) {
    state.uiScreen = UiScreen::MENU;
  }
}

void MenuController::render() {
  SharedRuntimeState stateCopy;
  if (xSemaphoreTake(app_.sync.stateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    stateCopy = app_.state;
    xSemaphoreGive(app_.sync.stateMutex);
  } else {
    return;
  }

  if (xSemaphoreTake(app_.sync.i2cMutex, pdMS_TO_TICKS(20)) != pdTRUE) {
    return;
  }

  switch (stateCopy.uiScreen) {
    case UiScreen::MENU:
      app_.services.display->renderMenu(stateCopy.menuIndex);
      break;
    case UiScreen::LIVE_MONITOR:
      app_.services.display->renderLiveMonitor(stateCopy.latestSensor, stateCopy.anxietyScore);
      break;
    case UiScreen::WIFI_SETUP:
      app_.services.display->renderMessage("WiFi Setup", "Open captive portal");
      break;
    case UiScreen::DEVICE_INFO:
      app_.services.display->renderMessage("Device Info", "UID: ESP32-C3-001");
      break;
    case UiScreen::CALIBRATION:
      app_.services.display->renderMessage("Calibration", "TODO: sensor calibration");
      break;
    case UiScreen::BP_PROGRESS:
      app_.services.display->renderBpProgress(stateCopy.bpState, stateCopy.bpProgress,
                                              stateCopy.latestSensor.cuffPressure,
                                              stateCopy.latestSensor.bp);
      break;
  }

  xSemaphoreGive(app_.sync.i2cMutex);
}
