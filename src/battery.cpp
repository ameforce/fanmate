#include "battery.h"

#include <Adafruit_INA219.h>
#include <math.h>
#include "config.h"

namespace {
Adafruit_INA219 ina219(Config::INA219_ADDR);

struct BatteryPoint {
  float voltage;
  uint8_t percent;
};

constexpr BatteryPoint BATTERY_TABLE[] = {
    {6.00f, 0},  {6.20f, 5},  {6.40f, 10}, {6.60f, 15},
    {6.80f, 25}, {7.00f, 35}, {7.20f, 45}, {7.40f, 55},
    {7.60f, 65}, {7.80f, 75}, {8.00f, 85}, {8.20f, 95},
    {8.40f, 100},
};
}  // namespace

bool BatteryMonitor::begin() {
  reading_.available = ina219.begin(&Wire);
  if (reading_.available) {
    ina219.setCalibration_32V_2A();
  }
  return reading_.available;
}

bool BatteryMonitor::update(uint32_t nowMs, bool force) {
  if (!reading_.available) {
    return false;
  }
  if (!force && (nowMs - lastReadMs_) < Config::BATTERY_UPDATE_MS) {
    return false;
  }
  lastReadMs_ = nowMs;

  reading_.busVoltage = ina219.getBusVoltage_V();
  reading_.shuntVoltageMv = ina219.getShuntVoltage_mV();
  reading_.candidateLoadVoltage =
      reading_.busVoltage + (reading_.shuntVoltageMv / 1000.0f);
  reading_.selectedVoltage = Config::BATTERY_INCLUDE_SHUNT_CORRECTION
                                 ? reading_.candidateLoadVoltage
                                 : reading_.busVoltage;
  reading_.calibratedVoltage =
      (reading_.selectedVoltage * Config::BATTERY_VOLTAGE_SCALE) +
      Config::BATTERY_VOLTAGE_OFFSET;
  reading_.percent = percentFromVoltage(reading_.calibratedVoltage);
  reading_.lowWarning =
      reading_.calibratedVoltage <= Config::BATTERY_LOW_WARNING_V;
  reading_.critical = reading_.calibratedVoltage <= Config::BATTERY_CRITICAL_V;
  return true;
}

const BatteryReading &BatteryMonitor::reading() const {
  return reading_;
}

bool BatteryMonitor::available() const {
  return reading_.available;
}

uint8_t BatteryMonitor::percentFromVoltage(float voltage) const {
  if (isnan(voltage) || voltage <= BATTERY_TABLE[0].voltage) {
    return 0;
  }
  size_t last = (sizeof(BATTERY_TABLE) / sizeof(BATTERY_TABLE[0])) - 1;
  if (voltage >= BATTERY_TABLE[last].voltage) {
    return 100;
  }

  for (size_t i = 1; i <= last; ++i) {
    if (voltage <= BATTERY_TABLE[i].voltage) {
      const BatteryPoint &lower = BATTERY_TABLE[i - 1];
      const BatteryPoint &upper = BATTERY_TABLE[i];
      float span = upper.voltage - lower.voltage;
      float ratio = span <= 0.0f ? 0.0f : (voltage - lower.voltage) / span;
      float percent =
          lower.percent + ratio * (upper.percent - lower.percent);
      if (percent < 0.0f) {
        return 0;
      }
      if (percent > 100.0f) {
        return 100;
      }
      return static_cast<uint8_t>(percent + 0.5f);
    }
  }
  return 0;
}
