#include "fan_pwm.h"

#include "config.h"

#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif

#ifndef ESP_ARDUINO_VERSION_MAJOR
#define ESP_ARDUINO_VERSION_MAJOR 2
#endif

namespace {
bool pwmReady = false;
uint8_t currentPercent = Config::BOOT_FAN_PERCENT;
uint32_t currentDuty = 0;

constexpr uint32_t maxDuty() {
  return (1UL << Config::FAN_PWM_RESOLUTION_BITS) - 1UL;
}

uint32_t dutyFromPercent(uint8_t percent) {
  if (percent > 100) {
    percent = 100;
  }
  uint32_t duty = (static_cast<uint32_t>(percent) * maxDuty() + 50UL) / 100UL;
  if (Config::FAN_PWM_INVERTED) {
    duty = maxDuty() - duty;
  }
  return duty;
}

bool writeRawDuty(uint32_t duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  return ledcWrite(Config::FAN_PWM_PIN, duty);
#else
  ledcWrite(Config::FAN_PWM_CHANNEL, duty);
  return true;
#endif
}
}  // namespace

void bootFanSafeInit() {
  currentPercent = Config::BOOT_FAN_PERCENT;
  currentDuty = dutyFromPercent(currentPercent);
  pinMode(Config::FAN_PWM_PIN, OUTPUT);
  digitalWrite(Config::FAN_PWM_PIN, LOW);
}

bool setupFanPwm() {
  bootFanSafeInit();
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  pwmReady = ledcAttach(Config::FAN_PWM_PIN,
                        Config::FAN_PWM_FREQ_HZ,
                        Config::FAN_PWM_RESOLUTION_BITS);
#else
  double actualFreq = ledcSetup(Config::FAN_PWM_CHANNEL,
                                Config::FAN_PWM_FREQ_HZ,
                                Config::FAN_PWM_RESOLUTION_BITS);
  pwmReady = actualFreq > 0.0;
  if (pwmReady) {
    ledcAttachPin(Config::FAN_PWM_PIN, Config::FAN_PWM_CHANNEL);
  }
#endif

  bool wroteZero = writeFanPercent(Config::BOOT_FAN_PERCENT);
  if (!wroteZero) {
    digitalWrite(Config::FAN_PWM_PIN, LOW);
  }
  return pwmReady && wroteZero;
}

bool writeFanPercent(uint8_t percent) {
  if (percent > 100) {
    percent = 100;
  }
  currentPercent = percent;
  currentDuty = dutyFromPercent(percent);

  if (!pwmReady) {
    if (percent == 0 && !Config::FAN_PWM_INVERTED) {
      digitalWrite(Config::FAN_PWM_PIN, LOW);
      return true;
    }
    return false;
  }

  return writeRawDuty(currentDuty);
}

uint8_t getFanPercent() {
  return currentPercent;
}

uint32_t getFanDuty() {
  return currentDuty;
}

bool isFanPwmReady() {
  return pwmReady;
}
