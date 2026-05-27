#pragma once

#include <Arduino.h>

struct ButtonEvent {
  bool pressed = false;
  bool released = false;
  bool shortPress = false;
  bool longPress = false;
  bool repeatPress = false;
  bool isDown = false;
  bool rawLevel = false;
};

class DebouncedButton {
 public:
  void begin(uint8_t pin, bool activeLow);
  ButtonEvent update(uint32_t nowMs);
  bool raw() const;
  bool down() const;

 private:
  bool activeFromRaw(bool rawLevel) const;

  uint8_t pin_ = 0;
  bool activeLow_ = false;
  bool rawLevel_ = false;
  bool stableDown_ = false;
  bool lastReadingDown_ = false;
  bool longReported_ = false;
  uint32_t lastChangeMs_ = 0;
  uint32_t pressStartMs_ = 0;
  uint32_t nextRepeatMs_ = 0;
};

struct ButtonEvents {
  ButtonEvent mode;
  ButtonEvent up;
  ButtonEvent down;
};

class ButtonManager {
 public:
  void begin();
  ButtonEvents update(uint32_t nowMs);

 private:
  DebouncedButton mode_;
  DebouncedButton up_;
  DebouncedButton down_;
};

void setupRawButtonPins();
