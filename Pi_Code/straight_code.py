#!/usr/bin/env python3
"""straight_code — Raspberry Pi Zero W port of
Code/straight_code_2/straight_code_2.ino.

Drives straight down a corridor using side ToF sensors, stops when a
front wall is close. The algorithm is the same as the Arduino version:
invalid readings skipped, 4-reading moving averages, proportional
steering with a deadband, front-wall stop with hysteresis, and
comma-separated telemetry over BLE (Nordic UART Service).

Usage:
    python3 straight_code.py            # run for real
    python3 straight_code.py --no-motors  # bench test: sensors + BLE only
    python3 straight_code.py --hz 5     # slow the loop for debugging
"""
import argparse
import logging
import time

import pigpio

import config
from ble_uart import BleUart
from motors import MotorDriver
from tof_sensors import SensorArray


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--no-motors', action='store_true',
                        help='run sensors and telemetry with motors off')
    parser.add_argument('--hz', type=float, default=None,
                        help='cap the control loop at this rate')
    return parser.parse_args()


def main():
    logging.basicConfig(level=logging.INFO,
                        format='%(asctime)s %(levelname)s %(message)s')
    args = parse_args()

    pi = pigpio.pi()
    if not pi.connected:
        raise SystemExit('Could not connect to pigpiod — is it running? '
                         '(sudo systemctl start pigpiod)')

    motors = MotorDriver(pi)
    motors.setup()

    sensors = SensorArray(pi)
    sensors.setup()

    telemetry = BleUart(config.BLE_LOCAL_NAME, config.BLE_TX_INTERVAL)
    telemetry.start()

    motors_enabled = config.ENABLE_MOTORS and not args.no_motors

    # Hysteresis state: stays False until the front sensor sees clear
    # corridor past FRONT_RESUME_MM.
    driving = False
    min_loop_s = 1.0 / args.hz if args.hz else 0.0

    try:
        while True:
            loop_start = time.monotonic()

            sensors.read_all()

            averages = [sensors.average(i)
                       for i in range(len(config.XSHUT_PINS))]
            telemetry.send_line(','.join(
                '0.0' if avg is None else '%.1f' % avg
                for avg in averages))

            # No valid front reading yet -> stay still (matches the
            # Arduino code, which waited for the first front reading).
            front = averages[config.FRONT]
            if front is None:
                motors.stop()
                continue

            if front < config.FRONT_STOP_MM:
                driving = False
            elif front > config.FRONT_RESUME_MM:
                driving = True

            if not driving:
                motors.stop()
                continue

            # Positive error = right wall farther away than the left one,
            # i.e. drifting left, so speed up the left motor. If a side
            # sensor is dead, just drive straight on the base speed.
            left = averages[config.LEFT]
            right = averages[config.RIGHT]
            correction = 0
            if (sensors.ok[config.LEFT] and sensors.ok[config.RIGHT]
                    and left is not None and right is not None):
                error = right - left
                if abs(error) > config.STEERING_DEADBAND:
                    correction = int(config.STEERING_KP * error)

            if motors_enabled:
                motors.drive(config.BASE_SPEED + correction,
                             config.BASE_SPEED - correction)

            if min_loop_s:
                elapsed = time.monotonic() - loop_start
                if elapsed < min_loop_s:
                    time.sleep(min_loop_s - elapsed)
    except KeyboardInterrupt:
        pass
    finally:
        motors.stop()
        sensors.shutdown()
        pi.stop()


if __name__ == '__main__':
    main()
