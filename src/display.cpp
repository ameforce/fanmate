#include "display.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <math.h>
#include "config.h"
#include "i2c_devices.h"

namespace {
Adafruit_SSD1306 display(Config::OLED_WIDTH,
                         Config::OLED_HEIGHT,
                         &Wire,
                         Config::OLED_RESET_PIN);

bool beginDisplayAt(uint8_t address) {
  if (!display.begin(SSD1306_SWITCHCAPVCC, address)) {
    return false;
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("Fanmate"));
  display.println(F("OLED ready"));
  display.display();
  return true;
}

void printTemperature(float temperatureC, bool valid) {
  if (valid && !isnan(temperatureC)) {
    display.print(temperatureC, 1);
    display.print(F(" C"));
  } else {
    display.print(F("--.- C"));
  }
}

void printHumidity(float humidity, bool valid) {
  if (valid && !isnan(humidity)) {
    display.print(humidity, 0);
    display.print(F("%"));
  } else {
    display.print(F("--%"));
  }
}
}  // namespace

bool DisplayController::begin() {
  const uint8_t candidates[] = {Config::OLED_ADDR_PRIMARY,
                                Config::OLED_ADDR_SECONDARY};
  for (uint8_t candidate : candidates) {
    if (!i2cAddressPresent(candidate)) {
      continue;
    }
    if (beginDisplayAt(candidate)) {
      available_ = true;
      address_ = candidate;
      return true;
    }
  }

  for (uint8_t candidate : candidates) {
    if (beginDisplayAt(candidate)) {
      available_ = true;
      address_ = candidate;
      return true;
    }
  }

  available_ = false;
  address_ = 0;
  return false;
}

bool DisplayController::available() const {
  return available_;
}

void DisplayController::showBoot(const char *message) {
  if (!available_) {
    return;
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Fanmate"));
  display.println(message);
  display.print(F("OLED 0x"));
  display.println(address_, HEX);
  display.display();
}

void DisplayController::update(uint32_t nowMs,
                               AppMode mode,
                               const BatteryReading &battery,
                               bool tempValid,
                               float temperatureC,
                               float humidity,
                               uint8_t fanPercent,
                               bool servoSweep) {
  if (!available_ || (nowMs - lastUpdateMs_) < Config::DISPLAY_UPDATE_MS) {
    return;
  }
  lastUpdateMs_ = nowMs;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(modeName(mode));
  display.setCursor(58, 0);
  display.print(F("Batt "));
  if (battery.available && !isnan(battery.calibratedVoltage)) {
    display.print(battery.percent);
    display.print(F("%"));
  } else {
    display.print(F("--%"));
  }

  display.setTextSize(2);
  display.setCursor(0, 12);
  display.print(F("Fan "));
  display.print(fanPercent);
  display.print(F("%"));

  display.setTextSize(1);
  display.setCursor(0, 34);
  display.print(F("T "));
  printTemperature(temperatureC, tempValid);
  display.print(F("  H "));
  printHumidity(humidity, tempValid);

  display.setCursor(0, 45);
  display.print(F("V "));
  if (battery.available && !isnan(battery.calibratedVoltage)) {
    display.print(battery.calibratedVoltage, 2);
    display.print(F("V"));
    if (battery.critical) {
      display.print(F(" CRIT"));
    } else if (battery.lowWarning) {
      display.print(F(" LOW"));
    }
  } else {
    display.print(F("--.--V"));
  }

  display.setCursor(0, 56);
  display.print(F("Servo: "));
  display.println(servoSweep ? F("SWEEP") : F("OFF"));

  display.display();
}
