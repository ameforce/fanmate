#pragma once

#include <Arduino.h>

struct BatteryReading {
  bool available = false;
  float busVoltage = NAN;
  float shuntVoltageMv = NAN;
  float candidateLoadVoltage = NAN;
  float selectedVoltage = NAN;
  float rawCalibratedVoltage = NAN;
  float filteredVoltage = NAN;
  float calibratedVoltage = NAN;
  uint8_t rawPercent = 0;
  uint8_t percent = 0;
  bool lowWarning = false;
  bool critical = false;
};

class BatteryMonitor {
 public:
  bool begin();
  bool update(uint32_t nowMs, bool force = false);
  const BatteryReading &reading() const;
  bool available() const;

 private:
  uint8_t percentFromVoltage(float voltage) const;
  BatteryReading reading_;
  uint32_t lastReadMs_ = 0;
};
