# Micromouse — PolyEd

Robot mouse for the APEC MicroMouse contest. Arduino (ATmega32U4) code in
`Code/`, Raspberry Pi Zero W port in `Pi_Code/` (the project is
migrating to the Pi — see TODO.md).

## Layout

- `Pi_Code/` — current direction: Python port of the straight-driving
  firmware to the Pi Zero W. All pins/tuning in `Pi_Code/config.py`.
- `Code/` — Arduino sketches. `straight_code_2/` is the improved
  version of `straight_code/`; treat `straight_code_2` as the reference
  algorithm when porting.
- Folders literally named `(OLD) ...` are archives — don't edit, but
  they hold datasheets and sensor info that may still be useful.
- Root README.md and the two hyphenated READMEs (I2C, PLOTTER) hold
  project-level context; read before working with the Arduino code.

## Hardware quirks

- 5 VL53L0X ToF sensors, roles fixed across all code versions:
  **sensor 3 = front, sensor 2 = left, sensor 5 = right** (indexes
  2/1/4 in 0-based code).
- On the Arduino build, XSHUT_3 and the Bluefruit IRQ are **both pin 7**
  (inherited wiring conflict, commented in straight_code_2.ino). The Pi
  port gives every sensor its own XSHUT pin, so this doesn't apply there.
- Pi motor pinout is **NOT finalized** — proposed defaults live in
  `Pi_Code/config.py` and must be updated to match actual wiring.
- Pi GPIOs are 3.3 V only; the Arduino may have been driving the motor
  driver at 5 V. Check the driver accepts 3.3 V logic before connecting.
- Telemetry is comma-separated sensor averages, sent over BLE Nordic
  UART Service (same service on Bluefruit and the Pi port), consumed by
  a bleak receiver script / serial plotter workflow.

## Conventions

- New code gets its own folder; don't modify other sketches in place
  (that's how straight_code_2 and Pi_Code came to exist).
- Arduino sketches target `arduino:avr:micro` (Pro Micro / ATmega32U4).
- Pi code is plain Python 3, no framework; each concern is one small
  module (config / sensors / motors / ble) so pieces can be swapped.

## Verifying changes

- Arduino: `arduino-cli compile --fqbn arduino:avr:micro <sketch-dir>`
  (libs already installed: Adafruit BluefruitLE nRF51, VL53L0X).
- Pi code (on a non-Pi machine): `python3 -m py_compile <files>`, or
  smoke-test `straight_code.py` with stub `pigpio`/`VL53L0X`/`bluezero`
  modules on PYTHONPATH and a SIGINT after a few seconds — motor
  commands can be logged from the stub to verify control logic without
  hardware.
- On the Pi: `i2cdetect -y 1` should show 0x2A–0x2E once sensors init;
  run `python3 straight_code.py --no-motors` before any real run.
