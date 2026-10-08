"""SensorArray: 5x VL53L0X on I2C, one XSHUT GPIO each, with moving averages.

Wraps the Pimoroni VL53L0X ctypes library so the control loop never
touches it directly. Invalid readings (error -1, 0, or out-of-range
~8190+) are skipped instead of being averaged in, same as
straight_code_2.ino.
"""
import logging
import time
from collections import deque

import pigpio
import VL53L0X

import config

# 20 ms timing budget: the fastest mode the library offers. Sensors run in
# continuous ranging mode, so get_distance() returns as soon as a fresh
# sample is pending — reading all 5 sensors takes roughly one timing
# budget per loop, not five.
RANGING_MODE = VL53L0X.Vl53l0xAccuracyMode.HIGH_SPEED

# Address change is volatile: XSHUT low (or power off) resets the sensor
# back to the default 0x29, so every run re-assigns addresses from scratch.
RESET_HOLD_S = 0.5
POWER_UP_S = 0.1


class SensorArray:
    def __init__(self, pi):
        """pi: a connected pigpio instance, used only to drive XSHUT pins."""
        self._pi = pi
        self._sensors = [None] * len(config.XSHUT_PINS)
        self.ok = [False] * len(config.XSHUT_PINS)
        self._windows = [deque(maxlen=config.NUM_READINGS)
                         for _ in config.XSHUT_PINS]

    def setup(self):
        """Reset every sensor, then enable them one at a time, giving each
        a unique I2C address before the next one comes online (they all
        power up at the default 0x29)."""
        for pin in config.XSHUT_PINS:
            self._pi.set_mode(pin, pigpio.OUTPUT)
        for pin in config.XSHUT_PINS:
            self._pi.write(pin, 0)
        time.sleep(RESET_HOLD_S)

        for i, pin in enumerate(config.XSHUT_PINS):
            self._pi.write(pin, 1)
            time.sleep(POWER_UP_S)

            tof = VL53L0X.VL53L0X(i2c_bus=config.I2C_BUS)
            try:
                tof.change_address(config.I2C_BASE_ADDRESS + i)
                tof.open()
                tof.start_ranging(RANGING_MODE)
            except (OSError, VL53L0X.Vl53l0xError) as exc:
                # Leave the sensor shut down so the rest of the bus keeps
                # working; the control loop treats it as dead.
                logging.warning('Sensor %d failed to initialize: %s',
                                i + 1, exc)
                self._pi.write(pin, 0)
                continue

            self._sensors[i] = tof
            self.ok[i] = True

        logging.info('Sensors online: %s',
                     ', '.join(str(i + 1) if ok else '-'
                               for i, ok in enumerate(self.ok)))

    def read(self, i):
        """Take one reading from sensor i and update its moving average."""
        if not self.ok[i]:
            return
        distance = self._sensors[i].get_distance()
        if 0 < distance < config.SENSOR_MAX_MM:
            self._windows[i].append(distance)

    def read_all(self):
        for i in range(len(self._sensors)):
            self.read(i)

    def average(self, i):
        """Moving average in mm, or None if there is no valid data yet."""
        window = self._windows[i]
        if not window:
            return None
        return sum(window) / len(window)

    def has_data(self, i):
        return bool(self._windows[i])

    def shutdown(self):
        for i, tof in enumerate(self._sensors):
            if tof is None:
                continue
            try:
                tof.stop_ranging()
                tof.close()
            except Exception as exc:
                logging.warning('Sensor %d shutdown error: %s', i + 1, exc)
            self._sensors[i] = None
            self.ok[i] = False
        for pin in config.XSHUT_PINS:
            self._pi.write(pin, 0)
