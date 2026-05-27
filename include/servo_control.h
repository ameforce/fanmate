#pragma once

#include <Arduino.h>
#include "config.h"

class ServoController {
 public:
  bool begin();
  bool update(uint32_t nowMs);
  void writeAngle(int angle);
  void restoreState(int angle, bool sweepEnabled);
  void setSweepEnabled(bool enabled);
  void toggleSweep();
  bool sweepEnabled() const;
  bool attached() const;
  int currentAngle() const;

 private:
  bool attached_ = false;
  bool sweepEnabled_ = false;
  int currentAngle_ = Config::SERVO_BOOT_ANGLE;
  int direction_ = 1;
  uint32_t lastStepMs_ = 0;
};
