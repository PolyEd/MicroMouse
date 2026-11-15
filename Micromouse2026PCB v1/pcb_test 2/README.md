# pcb_test — board self-test for Micromouse2026PCB

Flash this onto the Arduino Micro of an assembled board to check every system
from the OLED, using only the rotary encoder.

## Controls

| Action | Does |
|---|---|
| Turn | Move the cursor / change a value |
| Click | Select / start or stop editing a value |
| Hold (0.6 s) | Back to the menu. On the motor page: **stop motors** and go back |

## Pages

| Page | Shows |
|---|---|
| Status (opens at boot) | OLED, IMU and ToF 1–5: I2C address, `OK` / `NO ACK` / `NOINIT`, live distance, VIN, seconds since boot (`UP`), lowest free RAM in bytes (`RAM`) |
| ToF map | Top view, mouse facing up: a dotted beam from each sensor's place on the board to what it sees, ending in a solid bar. The five distances in mm are underneath (left to right). No bar = out of range; no beam = sensor not working |
| IMU | Yaw / pitch / roll, report rate, calibration status (0–3) |
| Encoders | Raw count, revolutions and RPM of both motors. Click zeroes the counts |
| Motor test | Pick M1 / M2 / both, PWM or RPM mode, then dial the value. Shows PWM, RPM, count and a verdict |
| Re-init sensors | Runs sensor start-up again, e.g. after re-seating a part |

What the words mean:

- `NO ACK` — nothing answers at that address: part missing, not soldered, no power, or SDA/SCL open.
- `NOINIT` — the part answers but its start-up failed.
- `!I2C BUS ERR` on the status title — an I2C transfer timed out (SDA or SCL stuck or shorted).
- `UP` going back to a small number by itself — the Micro reset (usually a power dip), it did not freeze.
- `RAM` — the fewest bytes that have ever been free since boot. Anything in the hundreds is healthy.
- Screen blank, shifted or scrambled but the knob still works — hold the knob: every page change re-sends the display set-up.
- Motor verdict `ok` — encoder counts the same way the motor is driven.
- Motor verdict `REV` — encoder counts backwards. Flip `M?_ENC_REVERSED` in the sketch (or swap A/B). **Do not use RPM mode until this says `ok`**, the closed loop would run away.
- Motor verdict `STUCK` — driven hard but no counts: no battery, motor unplugged, dead driver, or dead encoder.
- `NO BATTERY!` — VIN is under 5.5 V. The motor drivers are powered from the
  battery rail, so the page will not drive the motors until the battery is on
  (`MOTOR_MIN_VIN` in the sketch; 0 switches the check off).

A ToF 1–4 showing `NO ACK` can also mean its XSHUT trace is open: that sensor
then never resets and ends up sharing an address with ToF 5.

## Pin map (read from `Micromouse2026PCB.kicad_pcb`)

| Signal | Micro pin |
|---|---|
| I2C SDA / SCL | D2 / D3 |
| ToF XSHUT U1, U2, U3, U4 | A0, A1, A2, A3 (U5 has none) |
| M1 driver IA / IB | D5 / D6 |
| M1 encoder A / B | D1 / D12 |
| M2 driver IA / IB | D9 / D10 |
| M2 encoder A / B | D0 / D11 |
| Rotary encoder A / B / switch | D7 / D8 / D4 |
| VIN sense (10k/10k divider) | A5 |

I2C addresses after start-up: ToF U1–U5 = 0x2A–0x2E, OLED = 0x3C, BNO085 = 0x4A.

## Libraries (Library Manager)

- **VL53L0X** by Pololu (1.3.1)
- **SparkFun BNO080 Cortex Based IMU** (1.1.12) — works with the Adafruit BNO085 board
- **SSD1306Ascii** by Bill Greiman (1.3.5)
- **Encoder** by Paul Stoffregen (1.4.4)

Board: Arduino Micro. Compile check: `arduino-cli compile --fqbn arduino:avr:micro Code/pcb_test`

## Things to know

- **Flash is nearly full** (28,356 of 28,672 bytes). If the IDE says "Sketch too big",
  set `TOF_PERIOD_MS` to 0, which frees about 190 bytes. RAM is fine (~61 %
  static, roughly 900 bytes free at run time).
- `TOF_PERIOD_MS` (default 100) is how often each ToF ranges. The laser is on
  for about a third of that time, which keeps the sensors cooler than ranging
  non-stop.
- `EncoderMotor.h` here is the file from `Encoder Motor/EncoderMotor_SimpleDemo`
  plus one `#ifndef ENCODERMOTOR_NO_POSITION_MODE` around the position-mode
  line in `update()`. The sketch defines it because the unused `moveTo` code
  costs 2.2 KB and the sketch does not fit otherwise. PWM and RPM modes are
  unchanged.
- IMU angles are computed from the raw quaternion with integer maths (the
  SparkFun float helpers cost ~4 KB). Accuracy is about ±0.25°.
- `ENCODER_PPR` and `GEAR_RATIO` at the top of the sketch are still the demo's
  values. They only affect "rev", "rpm" and RPM mode; raw counts are always
  right. To measure: zero the counts, turn a wheel one full turn by hand, read
  `cnt`.
- The ToF map takes each sensor's position and direction from the PCB file
  (`TOF_GEOM` in the sketch), assuming U1 is the left side and U5 the right.
  If the picture is mirrored, swap the signs in the "right" and "dx" columns.
  `MAP_MM_PER_PX` sets the zoom (4 = about 200 mm ahead, 250 mm to each side).
