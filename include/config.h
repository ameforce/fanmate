#pragma once

#include <Arduino.h>

namespace Config {

// Pin constants: soldered hardware contract. Do not change without hardware review.
constexpr uint8_t I2C_SDA_PIN = 5;
constexpr uint8_t I2C_SCL_PIN = 6;
constexpr uint8_t DHT_PIN = 4;
constexpr uint8_t SERVO_PIN = 3;
constexpr uint8_t FAN_PWM_PIN = 10;
constexpr uint8_t BUTTON_MODE_PIN = 0;
constexpr uint8_t BUTTON_UP_PIN = 1;
constexpr uint8_t BUTTON_DOWN_PIN = 7;

// Fan PWM.
constexpr uint32_t FAN_PWM_FREQ_HZ = 25000;
constexpr uint8_t FAN_PWM_RESOLUTION_BITS = 8;
constexpr uint8_t FAN_PWM_CHANNEL = 5;  // Keep away from channels commonly used by servo libraries.
constexpr bool FAN_PWM_INVERTED = false;
constexpr uint8_t BOOT_FAN_PERCENT = 0;

// Button behavior. Run the button_test environment first and update this if needed.
constexpr bool BUTTON_ACTIVE_LOW = false;
constexpr bool BUTTON_USE_INTERNAL_PULLUP = false;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 35;
constexpr uint32_t BUTTON_LONG_PRESS_MS = 900;
constexpr uint32_t BUTTON_REPEAT_START_MS = 450;
constexpr uint32_t BUTTON_REPEAT_INTERVAL_MS = 140;

// I2C / display.
constexpr uint32_t I2C_CLOCK_HZ = 100000;
constexpr uint8_t OLED_ADDR_PRIMARY = 0x3C;
constexpr uint8_t OLED_ADDR_SECONDARY = 0x3D;
constexpr int OLED_RESET_PIN = -1;
constexpr int OLED_WIDTH = 128;
constexpr int OLED_HEIGHT = 64;
constexpr uint32_t DISPLAY_UPDATE_MS = 100;

// INA219 / battery.
constexpr uint8_t INA219_ADDR = 0x40;
constexpr uint32_t BATTERY_UPDATE_MS = 1000;
constexpr float BATTERY_VOLTAGE_OFFSET = 0.0f;
constexpr float BATTERY_VOLTAGE_SCALE = 1.0f;
constexpr bool BATTERY_INCLUDE_SHUNT_CORRECTION = false;
constexpr float BATTERY_LOW_WARNING_V = 6.6f;
constexpr float BATTERY_CRITICAL_V = 6.2f;

// DHT11.
constexpr uint32_t DHT_READ_INTERVAL_MS = 2200;
constexpr float DHT_SMOOTHING_ALPHA = 0.20f;

// AUTO fan curve.
constexpr float AUTO_TEMP_MIN_C = 22.0f;
constexpr float AUTO_TEMP_MAX_C = 34.0f;
constexpr uint8_t AUTO_FAN_DEADBAND_PERCENT = 1;

// Servo.
constexpr int SERVO_BOOT_ANGLE = 90;
constexpr int SERVO_MIN_ANGLE = 30;
constexpr int SERVO_MAX_ANGLE = 150;
constexpr int SERVO_STEP_DEGREES = 1;
constexpr uint32_t SERVO_STEP_INTERVAL_MS = 50;
constexpr int SERVO_MIN_PULSE_US = 500;
constexpr int SERVO_MAX_PULSE_US = 2400;

// Logging.
constexpr uint32_t SERIAL_BAUD = 115200;
constexpr bool DEBUG_LOGS = true;

}  // namespace Config
