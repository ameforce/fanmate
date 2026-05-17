#pragma once

#include <Arduino.h>

void setupI2cBus();
uint8_t scanI2c(Print &out, uint8_t *addresses = nullptr, size_t maxAddresses = 0);
bool i2cAddressPresent(uint8_t address);
