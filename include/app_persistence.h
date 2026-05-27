#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "app_state.h"
#include "config.h"

struct PersistedAppState {
  bool valid = false;
  AppMode mode = AppMode::Off;
  uint8_t manualFanPercent = Config::BOOT_FAN_PERCENT;
  uint8_t autoFanPercent = Config::BOOT_FAN_PERCENT;
  bool servoSweepEnabled = false;
  int servoAngle = Config::SERVO_BOOT_ANGLE;
};

class AppPersistence {
 public:
  bool begin();
  PersistedAppState load();
  bool save(const PersistedAppState &state);
  bool available() const;

 private:
  Preferences preferences_;
  bool available_ = false;
};
