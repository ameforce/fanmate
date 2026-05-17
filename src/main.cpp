#include <Arduino.h>
#include <math.h>
#include "app_state.h"
#include "battery.h"
#include "buttons.h"
#include "config.h"
#include "dht_sensor.h"
#include "display.h"
#include "fan_pwm.h"
#include "i2c_devices.h"
#include "servo_control.h"

#if !defined(APP_MODE_BUTTON_TEST) && !defined(APP_MODE_I2C_SCAN) && \
    !defined(APP_MODE_OLED_TEST) && !defined(APP_MODE_INA219_TEST) && \
    !defined(APP_MODE_DHT_TEST) && !defined(APP_MODE_FAN_PWM_TEST) && \
    !defined(APP_MODE_SERVO_TEST) && !defined(APP_MODE_INTEGRATED)
#define APP_MODE_INTEGRATED
#endif

namespace {

BatteryMonitor battery;
ButtonManager buttons;
DisplayController displayController;
DhtSensor dhtSensor;
ServoController servoController;

AppMode appMode = AppMode::Off;
uint8_t manualFanPercent = Config::BOOT_FAN_PERCENT;
uint8_t autoFanPercent = Config::BOOT_FAN_PERCENT;
uint32_t lastStatusLogMs = 0;
uint32_t lastButtonPrintMs = 0;
uint32_t lastI2cScanMs = 0;
uint32_t lastFanPromptMs = 0;
uint32_t lastServoPromptMs = 0;

void startSerial(const __FlashStringHelper *banner) {
  Serial.begin(Config::SERIAL_BAUD);
  Serial.println();
  Serial.println(banner);
  Serial.println(F("Fan safety: GPIO10 forced LOW before Serial init"));
}

void logFanState(const __FlashStringHelper *prefix) {
  Serial.print(prefix);
  Serial.print(F(" fan="));
  Serial.print(getFanPercent());
  Serial.print(F("% duty="));
  Serial.print(getFanDuty());
  Serial.print(F(" pwmReady="));
  Serial.println(isFanPwmReady() ? F("yes") : F("no"));
}

void printBatteryReading(const BatteryReading &reading) {
  if (!reading.available) {
    Serial.println(F("INA219 unavailable"));
    return;
  }
  Serial.print(F("INA219 bus="));
  Serial.print(reading.busVoltage, 3);
  Serial.print(F("V shunt="));
  Serial.print(reading.shuntVoltageMv, 3);
  Serial.print(F("mV candidate="));
  Serial.print(reading.candidateLoadVoltage, 3);
  Serial.print(F("V selected="));
  Serial.print(reading.calibratedVoltage, 3);
  Serial.print(F("V percent="));
  Serial.print(reading.percent);
  Serial.print(F("%"));
  if (reading.critical) {
    Serial.print(F(" CRITICAL"));
  } else if (reading.lowWarning) {
    Serial.print(F(" LOW"));
  }
  Serial.println();
}

uint8_t clampPercent(int value) {
  if (value < 0) {
    return 0;
  }
  if (value > 100) {
    return 100;
  }
  return static_cast<uint8_t>(value);
}

uint8_t fanPercentFromTemperature(float temperatureC) {
  if (isnan(temperatureC) || temperatureC <= Config::AUTO_TEMP_MIN_C) {
    return 0;
  }
  if (temperatureC >= Config::AUTO_TEMP_MAX_C) {
    return 100;
  }
  float ratio = (temperatureC - Config::AUTO_TEMP_MIN_C) /
                (Config::AUTO_TEMP_MAX_C - Config::AUTO_TEMP_MIN_C);
  return clampPercent(static_cast<int>(ratio * 100.0f + 0.5f));
}

void applyModeFanOutput() {
  uint8_t target = 0;
  if (appMode == AppMode::Manual) {
    target = manualFanPercent;
  } else if (appMode == AppMode::Auto) {
    target = autoFanPercent;
  }

  if (target != getFanPercent()) {
    writeFanPercent(target);
    logFanState(F("Fan update"));
  }
}

void printButtonRawLine() {
  Serial.print(F("MODE="));
  Serial.print(digitalRead(Config::BUTTON_MODE_PIN));
  Serial.print(F(" UP="));
  Serial.print(digitalRead(Config::BUTTON_UP_PIN));
  Serial.print(F(" DOWN="));
  Serial.println(digitalRead(Config::BUTTON_DOWN_PIN));
}

void printFanCommandHelp() {
  Serial.println(F("Fan PWM commands: 0/1/2/3/4 => 0/25/50/75/100%, +=+1%, -=-1%, number 0-100 sets percent"));
}

bool parsePercentCommand(const String &command, int &value) {
  if (command.length() == 0) {
    return false;
  }
  for (size_t i = 0; i < command.length(); ++i) {
    if (!isDigit(command.charAt(i))) {
      return false;
    }
  }
  value = command.toInt();
  return value >= 0 && value <= 100;
}

void handleFanSerial() {
  if (!Serial.available()) {
    return;
  }
  String command = Serial.readStringUntil('\n');
  command.trim();
  if (command.length() == 0) {
    return;
  }

  int nextPercent = getFanPercent();
  if (command == "+") {
    nextPercent += 1;
  } else if (command == "-") {
    nextPercent -= 1;
  } else if (command == "0") {
    nextPercent = 0;
  } else if (command == "1") {
    nextPercent = 25;
  } else if (command == "2") {
    nextPercent = 50;
  } else if (command == "3") {
    nextPercent = 75;
  } else if (command == "4") {
    nextPercent = 100;
  } else if (!parsePercentCommand(command, nextPercent)) {
    Serial.print(F("Unknown fan command: "));
    Serial.println(command);
    printFanCommandHelp();
    return;
  }

  writeFanPercent(clampPercent(nextPercent));
  logFanState(F("Fan command"));
}

void printServoCommandHelp() {
  Serial.println(F("Servo commands: s=toggle sweep, c=center, status"));
}

void handleServoSerial() {
  if (!Serial.available()) {
    return;
  }
  String command = Serial.readStringUntil('\n');
  command.trim();
  if (command == "s") {
    servoController.toggleSweep();
  } else if (command == "c") {
    servoController.setSweepEnabled(false);
    servoController.begin();
  }
  Serial.print(F("Servo attached="));
  Serial.print(servoController.attached() ? F("yes") : F("no"));
  Serial.print(F(" sweep="));
  Serial.print(servoController.sweepEnabled() ? F("on") : F("off"));
  Serial.print(F(" angle="));
  Serial.println(servoController.currentAngle());
}

void logIntegratedStatus(uint32_t now) {
  if ((now - lastStatusLogMs) < 3000) {
    return;
  }
  lastStatusLogMs = now;

  Serial.print(F("Status mode="));
  Serial.print(modeName(appMode));
  Serial.print(F(" fan="));
  Serial.print(getFanPercent());
  Serial.print(F("% manual="));
  Serial.print(manualFanPercent);
  Serial.print(F("% auto="));
  Serial.print(autoFanPercent);
  Serial.print(F("% temp="));
  const DhtReading &dht = dhtSensor.reading();
  if (dht.valid) {
    Serial.print(dht.smoothedTemperatureC, 1);
    Serial.print(F("C"));
  } else {
    Serial.print(F("--.-C"));
  }
  Serial.print(F(" servoSweep="));
  Serial.println(servoController.sweepEnabled() ? F("on") : F("off"));
}

void setupSharedSafetyOnly(const __FlashStringHelper *banner) {
  bootFanSafeInit();
  startSerial(banner);
}

#if defined(APP_MODE_BUTTON_TEST)
void setupMode() {
  setupSharedSafetyOnly(F("Fanmate button_test"));
  setupRawButtonPins();
  Serial.print(F("BUTTON_ACTIVE_LOW currently configured as "));
  Serial.println(Config::BUTTON_ACTIVE_LOW ? F("true") : F("false"));
}

void loopMode(uint32_t now) {
  if ((now - lastButtonPrintMs) >= 250) {
    lastButtonPrintMs = now;
    printButtonRawLine();
  }
}
#elif defined(APP_MODE_I2C_SCAN)
void setupMode() {
  setupSharedSafetyOnly(F("Fanmate i2c_scan"));
  setupI2cBus();
  scanI2c(Serial);
}

void loopMode(uint32_t now) {
  if ((now - lastI2cScanMs) >= 5000) {
    lastI2cScanMs = now;
    scanI2c(Serial);
  }
}
#elif defined(APP_MODE_OLED_TEST)
void setupMode() {
  setupSharedSafetyOnly(F("Fanmate oled_test"));
  setupI2cBus();
  scanI2c(Serial);
  bool ok = displayController.begin();
  Serial.print(F("OLED init="));
  Serial.println(ok ? F("ok") : F("failed"));
  displayController.showBoot(ok ? "OLED test ready" : "OLED failed");
}

void loopMode(uint32_t now) {
  BatteryReading emptyBattery;
  displayController.update(now, AppMode::Off, emptyBattery, true, 25.0f, 0, false);
}
#elif defined(APP_MODE_INA219_TEST)
void setupMode() {
  setupSharedSafetyOnly(F("Fanmate ina219_test"));
  setupI2cBus();
  scanI2c(Serial);
  bool ok = battery.begin();
  Serial.print(F("INA219 init="));
  Serial.println(ok ? F("ok") : F("failed"));
}

void loopMode(uint32_t now) {
  if (battery.update(now, false)) {
    printBatteryReading(battery.reading());
  }
}
#elif defined(APP_MODE_DHT_TEST)
void setupMode() {
  setupSharedSafetyOnly(F("Fanmate dht_test"));
  dhtSensor.begin();
  Serial.println(F("DHT11 initialized; reading every >=2.2s"));
}

void loopMode(uint32_t now) {
  if (dhtSensor.update(now, false)) {
    const DhtReading &reading = dhtSensor.reading();
    if (reading.valid) {
      Serial.print(F("DHT temp="));
      Serial.print(reading.temperatureC, 1);
      Serial.print(F("C smooth="));
      Serial.print(reading.smoothedTemperatureC, 1);
      Serial.print(F("C humidity="));
      Serial.print(reading.humidity, 1);
      Serial.println(F("%"));
    } else {
      Serial.println(F("DHT read failed; keeping last valid value if any"));
    }
  }
}
#elif defined(APP_MODE_FAN_PWM_TEST)
void setupMode() {
  setupSharedSafetyOnly(F("Fanmate fan_pwm_test"));
  bool ok = setupFanPwm();
  Serial.print(F("Fan PWM setup="));
  Serial.println(ok ? F("ok") : F("failed"));
  logFanState(F("Boot"));
  printFanCommandHelp();
}

void loopMode(uint32_t now) {
  handleFanSerial();
  if ((now - lastFanPromptMs) >= 5000) {
    lastFanPromptMs = now;
    logFanState(F("Fan idle"));
    printFanCommandHelp();
  }
}
#elif defined(APP_MODE_SERVO_TEST)
void setupMode() {
  setupSharedSafetyOnly(F("Fanmate servo_test"));
  bool ok = servoController.begin();
  Serial.print(F("Servo init="));
  Serial.println(ok ? F("ok") : F("failed"));
  printServoCommandHelp();
}

void loopMode(uint32_t now) {
  handleServoSerial();
  servoController.update(now);
  if ((now - lastServoPromptMs) >= 5000) {
    lastServoPromptMs = now;
    Serial.print(F("Servo sweep="));
    Serial.print(servoController.sweepEnabled() ? F("on") : F("off"));
    Serial.print(F(" angle="));
    Serial.println(servoController.currentAngle());
    printServoCommandHelp();
  }
}
#else
void setupMode() {
  setupSharedSafetyOnly(F("Fanmate integrated"));
  bool fanOk = setupFanPwm();
  Serial.print(F("Fan PWM setup="));
  Serial.println(fanOk ? F("ok") : F("failed"));

  buttons.begin();
  setupI2cBus();
  scanI2c(Serial);

  bool displayOk = displayController.begin();
  Serial.print(F("OLED init="));
  Serial.println(displayOk ? F("ok") : F("failed"));

  bool batteryOk = battery.begin();
  Serial.print(F("INA219 init="));
  Serial.println(batteryOk ? F("ok") : F("failed"));

  dhtSensor.begin();
  Serial.println(F("DHT11 initialized"));

  bool servoOk = servoController.begin();
  Serial.print(F("Servo init="));
  Serial.println(servoOk ? F("ok") : F("failed"));

  writeFanPercent(Config::BOOT_FAN_PERCENT);
  displayController.showBoot("Ready");
  logFanState(F("Integrated boot"));
}

void handleIntegratedButtons(const ButtonEvents &events) {
  if (events.mode.longPress) {
    servoController.toggleSweep();
    Serial.print(F("MODE long: servo sweep "));
    Serial.println(servoController.sweepEnabled() ? F("on") : F("off"));
  }

  if (events.mode.shortPress) {
    if (appMode == AppMode::Off) {
      appMode = AppMode::Manual;
    } else if (appMode == AppMode::Manual) {
      appMode = AppMode::Auto;
    } else {
      appMode = AppMode::Off;
    }
    Serial.print(F("MODE short: mode="));
    Serial.println(modeName(appMode));
    applyModeFanOutput();
  }

  if (appMode == AppMode::Manual && events.up.shortPress) {
    manualFanPercent = clampPercent(static_cast<int>(manualFanPercent) + 1);
    Serial.print(F("UP: manual fan="));
    Serial.print(manualFanPercent);
    Serial.println(F("%"));
    applyModeFanOutput();
  }

  if (appMode == AppMode::Manual && events.down.shortPress) {
    manualFanPercent = clampPercent(static_cast<int>(manualFanPercent) - 1);
    Serial.print(F("DOWN: manual fan="));
    Serial.print(manualFanPercent);
    Serial.println(F("%"));
    applyModeFanOutput();
  }
}

void updateAutoFanFromTemperature() {
  const DhtReading &dht = dhtSensor.reading();
  if (!dht.valid || isnan(dht.smoothedTemperatureC)) {
    autoFanPercent = 0;
    return;
  }

  uint8_t nextAuto = fanPercentFromTemperature(dht.smoothedTemperatureC);
  int delta = static_cast<int>(nextAuto) - static_cast<int>(autoFanPercent);
  if (abs(delta) >= Config::AUTO_FAN_DEADBAND_PERCENT) {
    autoFanPercent = nextAuto;
  }
}

void loopMode(uint32_t now) {
  ButtonEvents events = buttons.update(now);
  handleIntegratedButtons(events);

  if (dhtSensor.update(now, false)) {
    const DhtReading &dht = dhtSensor.reading();
    if (dht.valid) {
      Serial.print(F("DHT temp="));
      Serial.print(dht.temperatureC, 1);
      Serial.print(F("C smooth="));
      Serial.print(dht.smoothedTemperatureC, 1);
      Serial.print(F("C humidity="));
      Serial.print(dht.humidity, 1);
      Serial.println(F("%"));
    } else {
      Serial.println(F("DHT read failed"));
    }
    updateAutoFanFromTemperature();
    if (appMode == AppMode::Auto) {
      applyModeFanOutput();
    }
  }

  if (battery.update(now, false)) {
    printBatteryReading(battery.reading());
  }

  servoController.update(now);
  applyModeFanOutput();

  const DhtReading &dht = dhtSensor.reading();
  displayController.update(now,
                           appMode,
                           battery.reading(),
                           dht.valid,
                           dht.valid ? dht.smoothedTemperatureC : NAN,
                           getFanPercent(),
                           servoController.sweepEnabled());
  logIntegratedStatus(now);
}
#endif

}  // namespace

void setup() {
  setupMode();
}

void loop() {
  loopMode(millis());
}
