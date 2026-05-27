#include "i2c_devices.h"

#include <Wire.h>
#include "config.h"

void setupI2cBus() {
  Wire.begin(Config::I2C_SDA_PIN, Config::I2C_SCL_PIN);
  Wire.setClock(Config::I2C_CLOCK_HZ);
}

uint8_t scanI2c(Print &out, uint8_t *addresses, size_t maxAddresses) {
  uint8_t found = 0;
  out.println(F("I2C scan start"));
  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();
    if (error == 0) {
      out.print(F("I2C device found at 0x"));
      if (address < 16) {
        out.print('0');
      }
      out.println(address, HEX);
      if (addresses != nullptr && found < maxAddresses) {
        addresses[found] = address;
      }
      ++found;
    }
  }
  out.print(F("I2C scan done, devices="));
  out.println(found);
  return found;
}

bool i2cAddressPresent(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}
