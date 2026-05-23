#include "drivers/DisplayManager.h"

#include "config/AppConfig.h"

DisplayManager::DisplayManager() : display_(U8G2_R0, U8X8_PIN_NONE) {}

void DisplayManager::begin() {
  display_.begin();
  display_.setBusClock(AppConfig::I2C_FREQUENCY);
  display_.setFont(u8g2_font_5x8_tr);
}

void DisplayManager::renderMenu(uint8_t selectedIndex) {
  static const char* kItems[] = {"Start BP", "Live Data", "WiFi Setup", "Device Info", "Calibrate"};
  constexpr uint8_t kVisibleItems = 4;
  const uint8_t startIndex =
      (selectedIndex < kVisibleItems) ? 0 : (selectedIndex - (kVisibleItems - 1));

  display_.clearBuffer();
  drawHeader_("Dento Smart");
  display_.setFont(u8g2_font_5x8_tr);

  for (uint8_t row = 0; row < kVisibleItems; ++row) {
    const uint8_t itemIndex = startIndex + row;
    if (itemIndex >= 5) {
      break;
    }

    const int y = 19 + (row * 11);
    if (itemIndex == selectedIndex) {
      display_.drawRBox(0, y - 7, 128, 10, 2);
      display_.setDrawColor(0);
      display_.drawStr(3, y, ">");
      display_.drawStr(12, y, kItems[itemIndex]);
      display_.setDrawColor(1);
    } else {
      display_.drawStr(3, y, " ");
      display_.drawStr(12, y, kItems[itemIndex]);
    }
  }

  if (startIndex > 0) {
    display_.drawStr(118, 17, "^");
  }
  if ((startIndex + kVisibleItems) < 5) {
    display_.drawStr(118, 60, "v");
  }

  drawFooterHint_("UP/DN SEL");
  display_.sendBuffer();
}

void DisplayManager::renderLiveMonitor(const SensorSnapshot& snapshot, float anxietyScore) {
  char line[24];
  display_.clearBuffer();
  drawHeader_("Live Data");

  display_.setFont(u8g2_font_5x8_tf);
  display_.drawFrame(0, 14, 62, 20);
  display_.drawFrame(66, 14, 62, 20);
  display_.drawFrame(0, 38, 62, 20);
  display_.drawFrame(66, 38, 62, 20);

  display_.drawStr(4, 22, "GSR");
  snprintf(line, sizeof(line), "%.2f", snapshot.gsr);
  display_.setFont(u8g2_font_6x12_tf);
  display_.drawStr(4, 31, line);

  display_.setFont(u8g2_font_5x8_tf);
  display_.drawStr(70, 22, "HR bpm");
  snprintf(line, sizeof(line), "%d", snapshot.heartRate);
  display_.setFont(u8g2_font_6x12_tf);
  display_.drawStr(70, 31, line);

  display_.setFont(u8g2_font_5x8_tf);
  display_.drawStr(4, 46, "Temp C");
  snprintf(line, sizeof(line), "%.1f", snapshot.temperature);
  display_.setFont(u8g2_font_6x12_tf);
  display_.drawStr(4, 55, line);

  display_.setFont(u8g2_font_5x8_tf);
  display_.drawStr(70, 46, "Anxiety");
  snprintf(line, sizeof(line), "%.2f", anxietyScore);
  display_.setFont(u8g2_font_6x12_tf);
  display_.drawStr(70, 55, line);

  drawFooterHint_("BACK menu");
  display_.sendBuffer();
}

void DisplayManager::renderBpProgress(BpState state, uint8_t progress, float pressureMmhg,
                                      const BloodPressureReading& reading) {
  char line[24];
  display_.clearBuffer();
  drawHeader_("Blood Pressure");

  display_.setFont(u8g2_font_5x8_tr);
  snprintf(line, sizeof(line), "State: %s", bpStateLabel_(state));
  display_.drawStr(0, 20, line);
  snprintf(line, sizeof(line), "Cuff : %.0f mmHg", pressureMmhg);
  display_.drawStr(0, 30, line);
  snprintf(line, sizeof(line), "Prog : %u%%", progress);
  display_.drawStr(84, 30, line);

  display_.drawFrame(0, 36, 128, 10);
  const uint8_t fillWidth = min<uint8_t>(124, static_cast<uint8_t>((progress * 124U) / 100U));
  display_.drawBox(2, 38, fillWidth, 6);

  if (reading.valid) {
    display_.setFont(u8g2_font_6x12_tf);
    snprintf(line, sizeof(line), "%d/%d", reading.systolic, reading.diastolic);
    display_.drawStr(0, 60, line);
    display_.setFont(u8g2_font_5x8_tr);
    display_.drawStr(52, 60, "mmHg");
  } else {
    display_.drawStr(0, 58, "Result: pending");
  }

  drawFooterHint_("BACK cancel");
  display_.sendBuffer();
}

void DisplayManager::renderMessage(const String& title, const String& body) {
  display_.clearBuffer();
  drawHeader_(title.c_str());
  display_.setFont(u8g2_font_5x8_tr);
  drawWrappedText_(0, 22, 21, body);
  drawFooterHint_("BACK menu");
  display_.sendBuffer();
}

void DisplayManager::drawHeader_(const char* title) {
  display_.setFont(u8g2_font_6x12_tf);
  display_.drawStr(0, 10, title);
  display_.drawHLine(0, 12, 128);
}

void DisplayManager::drawFooterHint_(const char* hint) {
  display_.setFont(u8g2_font_4x6_tr);
  display_.drawHLine(0, 63, 128);
  display_.drawStr(0, 62, hint);
}

void DisplayManager::drawWrappedText_(int16_t x, int16_t y, uint8_t maxCharsPerLine,
                                      const String& text) {
  String remaining = text;
  int16_t lineY = y;
  while (!remaining.isEmpty() && lineY <= 54) {
    String line = remaining.substring(0, min<uint8_t>(maxCharsPerLine, remaining.length()));
    if (remaining.length() > maxCharsPerLine) {
      int splitPos = line.lastIndexOf(' ');
      if (splitPos > 0) {
        line = line.substring(0, splitPos);
      }
    }

    line.trim();
    display_.drawUTF8(x, lineY, line.c_str());
    remaining.remove(0, line.length());
    remaining.trim();
    lineY += 10;
  }
}

const char* DisplayManager::bpStateLabel_(BpState state) const {
  switch (state) {
    case BpState::IDLE:
      return "IDLE";
    case BpState::INFLATING:
      return "INFLATE";
    case BpState::HOLD:
      return "HOLD";
    case BpState::MEASURING:
      return "MEASURE";
    case BpState::DEFLATING:
      return "DEFLATE";
    case BpState::COMPLETE:
      return "DONE";
    case BpState::ERROR:
      return "ERROR";
  }
  return "UNK";
}
