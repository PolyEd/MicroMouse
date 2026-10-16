# Pi Code

The Raspberry Pi Zero W port of `Code/straight_code_2/straight_code_2.ino`.
Same simple algorithm, new hardware: the Pi's onboard I2C talks to the five
VL53L0X sensors, onboard Bluetooth replaces the Bluefruit SPI board, and
pigpio drives the motors.

## Files

| File | What it is |
|---|---|
| `straight_code.py` | Main control loop (run this) |
| `config.py` | **All** pin numbers and tuning constants |
| `tof_sensors.py` | Sensor array: XSHUT init, I2C addresses, moving averages |
| `motors.py` | Motor driver (pigpio PWM, forward/backward pin per motor) |
| `ble_uart.py` | BLE telemetry (Nordic UART Service via bluezero) |

## Setup (once, on the Pi)

```bash
sudo raspi-config            # Interfacing Options -> I2C -> enable
sudo apt install build-essential python3-dev python3-pip i2c-tools
sudo systemctl enable --now pigpiod

cd Pi_Code
pip install -r requirements.txt
# If bluezero complains about D-Bus bindings:
#   sudo apt install python3-dbus python3-gi
```

## Wiring

BCM pin numbering. **I2C**: SDA = GPIO2, SCL = GPIO3 (the Pi's dedicated
I2C pins).

| Signal | BCM pin | Notes |
|---|---|---|
| Sensor 1 XSHUT | 4 | |
| Sensor 2 XSHUT | 5 | |
| Sensor 3 XSHUT | 6 | |
| Sensor 4 XSHUT | 17 | |
| Sensor 5 XSHUT | 27 | every sensor now gets its own XSHUT |
| M1 (left) forward | 12 | **NOT FINALIZED — proposed defaults** |
| M1 (left) backward | 13 | **NOT FINALIZED — proposed defaults** |
| M2 (right) forward | 18 | **NOT FINALIZED — proposed defaults** |
| M2 (right) backward | 19 | **NOT FINALIZED — proposed defaults** |

All pins live in `config.py` — change them there, nothing else needs
editing. Sensor roles are unchanged from the Arduino build: sensor 3 =
front, sensor 2 = left, sensor 5 = right.

After the sensors initialize, `i2cdetect -y 1` should show addresses
0x2A–0x2E (42–46 decimal).

## Running

```bash
# 1. Bench test first — sensors + telemetry, motors guaranteed off
python3 straight_code.py --no-motors

# 2. For real (slightly reduced loop rate is normal; see note below)
python3 straight_code.py
```

`--hz N` caps the loop rate for debugging. Ctrl-C always stops the motors.

## Getting the telemetry

The Pi advertises the **Nordic UART Service** (`6e400001-...`), the same
one the Bluefruit used, as `Micromouse`. On your laptop
(`pip install bleak`):

```python
# receiver.py
import asyncio
from bleak import BleakScanner, BleakClient

UART_TX = '6e400003-b5a3-f393-e0a9-e50e24dcca9e'

async def main():
    device = await BleakScanner.find_device_by_name('Micromouse')
    if device is None:
        raise SystemExit('Micromouse not found — is it running?')
    async with BleakClient(device) as client:
        await client.start_notify(UART_TX,
                                  lambda _, data: print(data.decode()))
        await asyncio.Event().wait()  # Ctrl-C to quit

asyncio.run(main())
```

Prints one comma-separated line per update: the five moving averages in
mm (sensor 1..5), the same shape as the Arduino telemetry.

## Hardware notes

- **3.3 V logic**: the Pi's GPIOs are 3.3 V only. The VL53L0X boards are
  fine at 3.3 V, but check that your motor driver inputs recognize 3.3 V
  as a high (the Arduino may have been driving them at 5 V).
- **Power**: motor current spikes on a shared rail will brown out the Pi
  and corrupt I2C. Give the Pi a clean 5 V feed; add bulk decoupling
  across the motor supply.
- **I2C pull-ups**: the Adafruit/Pololu VL53L0X breakouts each carry
  their own pull-ups; five boards is about the practical limit before
  the bus gets too strong.
- **Loop rate**: the sensors run in continuous ranging mode at a 20 ms
  timing budget, so expect a ~20–25 ms control loop (~40–50 Hz), roughly
  matching the Arduino version. Bluetooth is not the bottleneck; the
  sensor timing budget is.

## Tuning

Everything is in `config.py`: `BASE_SPEED`, `STEERING_KP`,
`STEERING_DEADBAND`, `FRONT_STOP_MM` / `FRONT_RESUME_MM`, and
`NUM_READINGS`. Same meaning as the `#define`s in `straight_code_2.ino`.
