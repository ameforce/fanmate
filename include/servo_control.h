#pragma once

#include <Arduino.h>

class ServoController {
 public:
  bool begin();
  void update(uint32_t nowMs);
  void setSweepEnabled(bool enabled);
  void toggleSweep();
  bool sweepEnabled() const;
  bool attached() const;
  int currentAngle() const;

 private:
  bool attached_ = false;
  bool sweepEnabled_ = false;
  int currentAngle_ = 90;
  int direction_ = 1;
  uint32_t lastStepMs_ = 0;
};
