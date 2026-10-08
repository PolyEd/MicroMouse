"""MotorDriver: pigpio PWM for the two drive motors.

Same scheme as the Arduino code: each motor has a forward pin and a
backward pin; the backward pin is always driven low and speed (0-255) is
PWM on the forward pin. pigpio's hardware-timed PWM keeps the output
steady, unlike RPi.GPIO's software PWM.
"""
import pigpio

import config


class MotorDriver:
    def __init__(self, pi):
        """pi: a connected pigpio instance."""
        self._pi = pi
        self._pins = [config.M1_FORWARD, config.M1_BACKWARD,
                      config.M2_FORWARD, config.M2_BACKWARD]

    def setup(self):
        for pin in self._pins:
            self._pi.set_mode(pin, pigpio.OUTPUT)
            self._pi.set_PWM_frequency(pin, config.PWM_FREQUENCY)
            self._pi.set_PWM_range(pin, 255)
        self.stop()

    def drive(self, left_speed, right_speed):
        """left/right_speed in 0-255 (M1 = left, M2 = right)."""
        left_speed = self._clamp(left_speed)
        right_speed = self._clamp(right_speed)

        self._pi.set_PWM_dutycycle(config.M1_FORWARD, left_speed)
        self._pi.set_PWM_dutycycle(config.M1_BACKWARD, 0)
        self._pi.set_PWM_dutycycle(config.M2_FORWARD, right_speed)
        self._pi.set_PWM_dutycycle(config.M2_BACKWARD, 0)

    def stop(self):
        for pin in self._pins:
            self._pi.set_PWM_dutycycle(pin, 0)

    @staticmethod
    def _clamp(speed):
        return max(0, min(255, int(speed)))
