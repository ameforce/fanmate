#pragma once

#include <stdint.h>

constexpr uint8_t kFanButtonStepBoundaries[] = {
    0, 15, 30, 45, 60, 75, 90, 100};
constexpr int kFanButtonStepBoundaryCount =
    sizeof(kFanButtonStepBoundaries) / sizeof(kFanButtonStepBoundaries[0]);

inline uint8_t fanButtonClampPercent(int value) {
  if (value < 0) {
    return 0;
  }
  if (value > 100) {
    return 100;
  }
  return static_cast<uint8_t>(value);
}

inline uint8_t fanButtonShortUp(uint8_t percent) {
  for (uint8_t boundary : kFanButtonStepBoundaries) {
    if (percent < boundary) {
      return boundary;
    }
  }
  return 100;
}

inline uint8_t fanButtonShortDown(uint8_t percent) {
  for (int i = kFanButtonStepBoundaryCount - 1; i >= 0; --i) {
    uint8_t boundary = kFanButtonStepBoundaries[i];
    if (percent > boundary) {
      return boundary;
    }
  }
  return 0;
}

inline uint8_t fanButtonRepeatUp(uint8_t percent) {
  return fanButtonClampPercent(static_cast<int>(percent) + 1);
}

inline uint8_t fanButtonRepeatDown(uint8_t percent) {
  return fanButtonClampPercent(static_cast<int>(percent) - 1);
}

class FanButtonStepTracker {
 public:
  void pressed() {
    repeated_ = false;
  }

  uint8_t repeatUp(uint8_t percent) {
    repeated_ = true;
    return fanButtonRepeatUp(percent);
  }

  uint8_t repeatDown(uint8_t percent) {
    repeated_ = true;
    return fanButtonRepeatDown(percent);
  }

  uint8_t shortUp(uint8_t percent) const {
    return repeated_ ? percent : fanButtonShortUp(percent);
  }

  uint8_t shortDown(uint8_t percent) const {
    return repeated_ ? percent : fanButtonShortDown(percent);
  }

  void released() {
    repeated_ = false;
  }

 private:
  bool repeated_ = false;
};
