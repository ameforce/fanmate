#include "app_persistence.h"

#include <Preferences.h>

namespace {
constexpr uint16_t PERSISTENCE_VERSION = 1;
constexpr char kNamespace[] = "fanmate";
constexpr char kVersionKey[] = "ver";
constexpr char kModeKey[] = "mode";
constexpr char kManualFanKey[] = "manual";
constexpr char kAutoFanKey[] = "auto";
constexpr char kServoSweepKey[] = "sweep";
constexpr char kServoAngleKey[] = "angle";

uint8_t clampPercent(int value) {
  if (value < 0) {
    return 0;
  }
  if (value > 100) {
    return 100;
  }
  return static_cast<uint8_t>(value);
}

int clampServoAngle(int angle) {
  if (angle < Config::SERVO_MIN_ANGLE) {
    return Config::SERVO_MIN_ANGLE;
  }
  if (angle > Config::SERVO_MAX_ANGLE) {
    return Config::SERVO_MAX_ANGLE;
  }
  return angle;
}

uint8_t modeToByte(AppMode mode) {
  switch (mode) {
    case AppMode::Off:
      return 0;
    case AppMode::Manual:
      return 1;
    case AppMode::Auto:
      return 2;
  }
  return 0;
}

AppMode modeFromByte(uint32_t value) {
  switch (value) {
    case 1:
      return AppMode::Manual;
    case 2:
      return AppMode::Auto;
    default:
      return AppMode::Off;
  }
}
}  // namespace

bool AppPersistence::begin() {
  available_ = preferences_.begin(kNamespace, false);
  return available_;
}

PersistedAppState AppPersistence::load() {
  PersistedAppState state;
  if (!available_) {
    return state;
  }

  uint32_t version = preferences_.getUInt(kVersionKey, 0);
  if (version != PERSISTENCE_VERSION) {
    return state;
  }

  state.valid = true;
  state.mode = modeFromByte(preferences_.getUInt(kModeKey, 0));
  state.manualFanPercent =
      clampPercent(static_cast<int>(preferences_.getUInt(kManualFanKey,
                                                         Config::BOOT_FAN_PERCENT)));
  state.autoFanPercent =
      clampPercent(static_cast<int>(preferences_.getUInt(kAutoFanKey,
                                                         Config::BOOT_FAN_PERCENT)));
  state.servoSweepEnabled = preferences_.getBool(kServoSweepKey, false);
  state.servoAngle =
      clampServoAngle(preferences_.getInt(kServoAngleKey, Config::SERVO_BOOT_ANGLE));
  return state;
}

bool AppPersistence::save(const PersistedAppState &state) {
  if (!available_) {
    return false;
  }

  bool ok = true;
  ok = preferences_.putUInt(kVersionKey, PERSISTENCE_VERSION) > 0 && ok;
  ok = preferences_.putUInt(kModeKey, modeToByte(state.mode)) > 0 && ok;
  ok = preferences_.putUInt(kManualFanKey,
                            clampPercent(state.manualFanPercent)) > 0 &&
       ok;
  ok = preferences_.putUInt(kAutoFanKey, clampPercent(state.autoFanPercent)) > 0 &&
       ok;
  ok = preferences_.putBool(kServoSweepKey, state.servoSweepEnabled) > 0 && ok;
  ok = preferences_.putInt(kServoAngleKey,
                           clampServoAngle(state.servoAngle)) > 0 &&
       ok;
  return ok;
}

bool AppPersistence::available() const {
  return available_;
}
