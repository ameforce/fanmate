#pragma once

enum class AppMode {
  Off,
  Manual,
  Auto,
};

inline const char *modeName(AppMode mode) {
  switch (mode) {
    case AppMode::Off:
      return "OFF";
    case AppMode::Manual:
      return "MANUAL";
    case AppMode::Auto:
      return "AUTO";
  }
  return "?";
}
