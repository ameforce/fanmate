#include "servo_control.h"

#include <ESP32Servo.h>
#include "config.h"

namespace {
Servo servo;
}

bool ServoController::begin() {
  servo.setPeriodHertz(50);
  int attachResult = servo.attach(Config::SERVO_PIN,
                                  Config::SERVO_MIN_PULSE_US,
                                  Config::SERVO_MAX_PULSE_US);
  attached_ = attachResult >= 0;
  currentAngle_ = Config::SERVO_BOOT_ANGLE;
  if (attached_) {
    servo.write(currentAngle_);
  }
  sweepEnabled_ = false;
  direction_ = 1;
  lastStepMs_ = millis();
  return attached_;
}

bool ServoController::update(uint32_t nowMs) {
  if (!attached_ || !sweepEnabled_ ||
      (nowMs - lastStepMs_) < Config::SERVO_STEP_INTERVAL_MS) {
    return false;
  }
  lastStepMs_ = nowMs;

  currentAngle_ += direction_ * Config::SERVO_STEP_DEGREES;
  if (currentAngle_ >= Config::SERVO_MAX_ANGLE) {
    currentAngle_ = Config::SERVO_MAX_ANGLE;
    direction_ = -1;
  } else if (currentAngle_ <= Config::SERVO_MIN_ANGLE) {
    currentAngle_ = Config::SERVO_MIN_ANGLE;
    direction_ = 1;
  }
  servo.write(currentAngle_);
  return true;
}

void ServoController::writeAngle(int angle) {
  if (angle < Config::SERVO_MIN_ANGLE) {
    angle = Config::SERVO_MIN_ANGLE;
  } else if (angle > Config::SERVO_MAX_ANGLE) {
    angle = Config::SERVO_MAX_ANGLE;
  }
  currentAngle_ = angle;
  sweepEnabled_ = false;
  direction_ = 1;
  lastStepMs_ = millis();
  if (attached_) {
    servo.write(currentAngle_);
  }
}

void ServoController::restoreState(int angle, bool sweepEnabled) {
  if (angle < Config::SERVO_MIN_ANGLE) {
    angle = Config::SERVO_MIN_ANGLE;
  } else if (angle > Config::SERVO_MAX_ANGLE) {
    angle = Config::SERVO_MAX_ANGLE;
  }
  currentAngle_ = angle;
  sweepEnabled_ = sweepEnabled;
  direction_ = currentAngle_ >= Config::SERVO_MAX_ANGLE ? -1 : 1;
  lastStepMs_ = millis();
  if (attached_) {
    servo.write(currentAngle_);
  }
}

void ServoController::setSweepEnabled(bool enabled) {
  sweepEnabled_ = enabled;
}

void ServoController::toggleSweep() {
  sweepEnabled_ = !sweepEnabled_;
}

bool ServoController::sweepEnabled() const {
  return sweepEnabled_;
}

bool ServoController::attached() const {
  return attached_;
}

int ServoController::currentAngle() const {
  return currentAngle_;
}
