#pragma once

#include <Arduino.h>

struct DhtReading {
  bool valid = false;
  float rawTemperatureC = NAN;
  float temperatureC = NAN;
  float humidity = NAN;
  float smoothedTemperatureC = NAN;
  uint32_t lastValidMs = 0;
};

class DhtSensor {
 public:
  void begin();
  bool update(uint32_t nowMs, bool force = false);
  const DhtReading &reading() const;

 private:
  DhtReading reading_;
  uint32_t lastReadAttemptMs_ = 0;
};
