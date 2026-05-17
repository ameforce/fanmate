#pragma once

#include <Arduino.h>

void bootFanSafeInit();
bool setupFanPwm();
bool writeFanPercent(uint8_t percent);
uint8_t getFanPercent();
uint32_t getFanDuty();
bool isFanPwmReady();
