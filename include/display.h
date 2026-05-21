#pragma once

#include <Arduino.h>
#include "app_state.h"
#include "battery.h"

class DisplayController {
 public:
  bool begin();
  bool available() const;
  void showBoot(const char *message);
  void update(uint32_t nowMs,
              AppMode mode,
              const BatteryReading &battery,
              bool tempValid,
              float temperatureC,
              float humidity,
              uint8_t fanPercent,
              float remainingMinutes,
              bool servoSweep);

 private:
  bool available_ = false;
  uint8_t address_ = 0;
  uint32_t lastUpdateMs_ = 0;
};
