#include "dht_sensor.h"

#include <DHT.h>
#include <math.h>
#include "config.h"

namespace {
DHT dht(Config::DHT_PIN, DHT11);
}

void DhtSensor::begin() {
  dht.begin();
}

bool DhtSensor::update(uint32_t nowMs, bool force) {
  if (!force && (nowMs - lastReadAttemptMs_) < Config::DHT_READ_INTERVAL_MS) {
    return false;
  }
  lastReadAttemptMs_ = nowMs;

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  if (isnan(humidity) || isnan(temperature)) {
    return true;
  }

  reading_.valid = true;
  reading_.humidity = humidity;
  reading_.temperatureC = temperature;
  reading_.lastValidMs = nowMs;
  if (isnan(reading_.smoothedTemperatureC)) {
    reading_.smoothedTemperatureC = temperature;
  } else {
    reading_.smoothedTemperatureC =
        Config::DHT_SMOOTHING_ALPHA * temperature +
        (1.0f - Config::DHT_SMOOTHING_ALPHA) *
            reading_.smoothedTemperatureC;
  }
  return true;
}

const DhtReading &DhtSensor::reading() const {
  return reading_;
}
