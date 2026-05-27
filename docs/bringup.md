# Fanmate ESP32-C3 Bring-Up Guide

This project uses PlatformIO with the Arduino framework and the `esp32-c3-devkitm-1` board target.

## Safety Rules
- Do not change the soldered pin map without a hardware review.
- Do not power the fan from the 5V rail.
- Do not power button VCC from 5V.
- Keep the fan yellow TACH/SENSE wire unused and insulated for the first firmware.
- Avoid holding the MODE button on GPIO0 during reset/upload until board behavior is confirmed.
- If the fan spins before an explicit test action, stop and fix fan PWM safety before continuing.

## Setup
If PlatformIO is available globally:

```powershell
pio run -e integrated
```

If `pio` is not on PATH, create a local virtual environment:

```powershell
python -m venv .venv
.\.venv\Scripts\python -m pip install --upgrade pip platformio
.\.venv\Scripts\pio.exe run -e integrated
```

Serial monitor:

```powershell
pio device monitor --port COM4 --baud 115200
```

or with the local virtual environment:

```powershell
.\.venv\Scripts\pio.exe device monitor --port COM4 --baud 115200
```

## Build All Environments

```powershell
pio run -e button_test
pio run -e i2c_scan
pio run -e oled_test
pio run -e ina219_test
pio run -e dht_test
pio run -e fan_pwm_test
pio run -e servo_test
pio run -e integrated
```

## Upload Order
Upload only one stage at a time and verify the observation before continuing.

```powershell
pio run -e button_test -t upload
pio device monitor -b 115200
```

Then repeat for:

1. `button_test`
2. `i2c_scan`
3. `oled_test`
4. `ina219_test`
5. `dht_test`
6. `fan_pwm_test`
7. `servo_test`
8. `integrated`

## Stage Notes

### button_test
Expected output:

```text
MODE=0 UP=0 DOWN=0
```

Verified hardware reads released as `0` and pressed as `1` for MODE, UP, and DOWN, so `BUTTON_ACTIVE_LOW` is `false`.

### i2c_scan
Expected devices:
- INA219: verified `0x40`
- OLED: verified `0x3C`

### oled_test
The firmware tries `0x3C` and then `0x3D`. OLED failure is logged and does not halt.

### ina219_test
Expected output shape:

```text
INA219 bus=7.820V shunt=0.300mV candidate=7.820V raw=7.820V filtered=7.810V rawPercent=76% percent=75%
```

Compare `raw` with a multimeter across the 2S pack. Adjust `BATTERY_VOLTAGE_OFFSET`, `BATTERY_VOLTAGE_SCALE`, or `BATTERY_INCLUDE_SHUNT_CORRECTION` in `include/config.h`.

For the verified wiring, VIN- is on BMS P+ and VIN+ is on the load-side positive line. The raw voltage currently uses bus voltage without shunt correction. OLED battery percent uses a filtered voltage so fan load spikes do not immediately swing the displayed percentage.

### dht_test
DHT11 is read every 2.2 seconds or slower. Failures are logged without blocking.
If the module consistently differs from a trusted room thermometer, set `DHT_TEMP_OFFSET_C` in `include/config.h`.
Current calibration applies `DHT_TEMP_OFFSET_C = -2.6f` from one trusted comparison:
fanmate displayed `28.6C` while the room thermometer read `26.0C`. Recheck this
offset after changing the sensor location or enclosure airflow.

### fan_pwm_test
The fan starts at 0%. It changes speed only through explicit Serial commands:
- `0`, `1`, `2`, `3`, `4` -> `0`, `25`, `50`, `75`, `100%`
- `+` / `-` -> one percent step
- `0` to `100` -> direct percent

If the fan runs faster at lower logical values, set `FAN_PWM_INVERTED` in `include/config.h`.

Verified hardware behavior: boot 0% keeps the fan stopped, 25% starts the fan, and 0% stops it again. `FAN_PWM_INVERTED` is `false`.

### servo_test
The servo initializes to 70 degrees. Send `s` to toggle the 10-170 degree sweep. Sweep is off at boot. Send a number from `10` to `170`, `+`, `-`, or `c` to calibrate the center angle.

### integrated
- MODE short press: `OFF -> MANUAL -> AUTO -> OFF`
- MODE long press: servo sweep on/off
- UP/DOWN short press: switch to MANUAL and move fan speed through
  `0 -> 15 -> 30 -> 45 -> 60 -> 75 -> 90 -> 100` boundaries.
  Non-boundary values snap in the pressed direction, for example `7 -> 15`
  on UP and `7 -> 0` on DOWN. From `100`, DOWN goes to `90`.
- UP/DOWN hold: switch to MANUAL immediately and repeat +1/-1% continuously
  after a short delay without applying the short-press boundary jump.
- AUTO: corrected, smoothed DHT11 temperature maps 22C to 0%, 26C to 13%,
  34C to 50%, and 44C to 100%, with a 1% update deadband
- Restore: after safe fan PWM and servo initialization, the firmware restores
  the last app mode, MANUAL fan percent, last AUTO fan percent, servo sweep
  state, and servo angle from ESP32 Preferences storage
- AUTO restore: if the restored mode is AUTO, the last AUTO fan percent is
  applied immediately until the first valid DHT sample arrives. Failed DHT
  reads before that first valid sample do not clear the provisional fan output.
- Persistence writes: MANUAL fan changes are debounced for 2 seconds, AUTO fan
  output is saved at most every 30 seconds, and servo sweep angle changes are
  saved at most every 10 seconds; mode and sweep toggles are saved immediately.
- OLED: larger fan percent, battery, temperature, humidity, estimated remaining time, and servo state

Remaining time is an estimate from `BATTERY_PACK_CAPACITY_MAH`, `BATTERY_PACK_NOMINAL_V`, `SYSTEM_IDLE_POWER_W`, `FAN_FULL_POWER_W`, and `SERVO_SWEEP_POWER_W` in `include/config.h`. Calibrate those constants after measuring the actual pack capacity and fan power draw.

Verified integrated behavior:
- Boot initializes OLED, INA219, DHT11, servo, and fan PWM without halting.
- OFF keeps fan at 0%.
- MANUAL UP/DOWN short press moves fan speed through the 0/15/30/45/60/75/90/100 boundaries.
- Holding UP/DOWN repeats one-percent fan adjustments without the short-press boundary jump.
- AUTO uses the calibrated piecewise smoothed temperature curve.
- MODE long press toggles servo sweep; fresh/default storage keeps sweep off,
  while a saved sweep state is restored on the next boot.

## Pin Map
| Function | Pin |
| --- | --- |
| I2C SDA | GPIO5 |
| I2C SCL | GPIO6 |
| DHT11 DATA | GPIO4 |
| Servo signal | GPIO3 |
| Fan PWM blue | GPIO10 |
| MODE button OUT | GPIO0 |
| UP button OUT | GPIO1 |
| DOWN button OUT | GPIO7 |
