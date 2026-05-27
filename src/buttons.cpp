#include "buttons.h"

#include "config.h"

namespace {
void setupButtonPin(uint8_t pin) {
  pinMode(pin, Config::BUTTON_USE_INTERNAL_PULLUP ? INPUT_PULLUP : INPUT);
}
}  // namespace

void DebouncedButton::begin(uint8_t pin, bool activeLow) {
  pin_ = pin;
  activeLow_ = activeLow;
  setupButtonPin(pin_);
  rawLevel_ = digitalRead(pin_) == HIGH;
  stableDown_ = activeFromRaw(rawLevel_);
  lastReadingDown_ = stableDown_;
  lastChangeMs_ = millis();
  pressStartMs_ = stableDown_ ? lastChangeMs_ : 0;
  nextRepeatMs_ = stableDown_ ? lastChangeMs_ + Config::BUTTON_REPEAT_START_MS : 0;
  longReported_ = false;
}

ButtonEvent DebouncedButton::update(uint32_t nowMs) {
  ButtonEvent event;
  rawLevel_ = digitalRead(pin_) == HIGH;
  bool readingDown = activeFromRaw(rawLevel_);

  if (readingDown != lastReadingDown_) {
    lastReadingDown_ = readingDown;
    lastChangeMs_ = nowMs;
  }

  if ((nowMs - lastChangeMs_) >= Config::BUTTON_DEBOUNCE_MS &&
      readingDown != stableDown_) {
    stableDown_ = readingDown;
    if (stableDown_) {
      event.pressed = true;
      pressStartMs_ = nowMs;
      nextRepeatMs_ = nowMs + Config::BUTTON_REPEAT_START_MS;
      longReported_ = false;
    } else {
      event.released = true;
      nextRepeatMs_ = 0;
      if (!longReported_) {
        event.shortPress = true;
      }
    }
  }

  if (stableDown_ && nextRepeatMs_ != 0 && nowMs >= nextRepeatMs_) {
    event.repeatPress = true;
    nextRepeatMs_ = nowMs + Config::BUTTON_REPEAT_INTERVAL_MS;
  }

  if (stableDown_ && !longReported_ &&
      (nowMs - pressStartMs_) >= Config::BUTTON_LONG_PRESS_MS) {
    event.longPress = true;
    longReported_ = true;
  }

  event.isDown = stableDown_;
  event.rawLevel = rawLevel_;
  return event;
}

bool DebouncedButton::raw() const {
  return rawLevel_;
}

bool DebouncedButton::down() const {
  return stableDown_;
}

bool DebouncedButton::activeFromRaw(bool rawLevel) const {
  return activeLow_ ? !rawLevel : rawLevel;
}

void ButtonManager::begin() {
  mode_.begin(Config::BUTTON_MODE_PIN, Config::BUTTON_ACTIVE_LOW);
  up_.begin(Config::BUTTON_UP_PIN, Config::BUTTON_ACTIVE_LOW);
  down_.begin(Config::BUTTON_DOWN_PIN, Config::BUTTON_ACTIVE_LOW);
}

ButtonEvents ButtonManager::update(uint32_t nowMs) {
  ButtonEvents events;
  events.mode = mode_.update(nowMs);
  events.up = up_.update(nowMs);
  events.down = down_.update(nowMs);
  return events;
}

void setupRawButtonPins() {
  setupButtonPin(Config::BUTTON_MODE_PIN);
  setupButtonPin(Config::BUTTON_UP_PIN);
  setupButtonPin(Config::BUTTON_DOWN_PIN);
}
