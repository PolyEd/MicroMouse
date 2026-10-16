"""All hardware pins and tuning constants for the Raspberry Pi Zero W port.

Everything you might need to change on the bench lives here; nothing else
should hard-code a pin number or a tuning value.
"""

# --- I2C / sensors ---
I2C_BUS = 1
I2C_BASE_ADDRESS = 42  # decimal; sensors get 42..46 (0x2A-0x2E)

# One XSHUT (shutdown) GPIO per sensor, indexes 0-4 = sensors 1-5.
# Unlike the Arduino build, sensor 5 also gets its own XSHUT pin, so every
# sensor is assigned an address the same way (and the old pin-7 conflict
# between XSHUT_3 and the Bluefruit IRQ no longer exists).
# BCM numbering.
XSHUT_PINS = [4, 5, 6, 17, 27]

# Sensor indexes by role (same as the Arduino code: sensors 1-5 = 0-4)
FRONT = 2
LEFT = 1
RIGHT = 4

SENSOR_MAX_MM = 8000  # VL53L0X reports ~8190/8191 when it sees nothing
NUM_READINGS = 4      # moving-average window per sensor

# --- Motors (BCM numbering) ---
# *** NOT FINALIZED — proposed defaults only, update to match your wiring. ***
# M1 = left motor, M2 = right motor (same naming as the Arduino code).
# Speed 0-255 on the forward pin, backward pin held low (same scheme as
# the Arduino code).
M1_FORWARD = 12
M1_BACKWARD = 13
M2_FORWARD = 18
M2_BACKWARD = 19

PWM_FREQUENCY = 1000  # Hz
ENABLE_MOTORS = True  # master switch; --no-motors overrides this too

# --- Driving parameters (same values as straight_code_2.ino) ---
BASE_SPEED = 129        # PWM (0-255) applied to both motors when centered
STEERING_KP = 3.0       # PWM units per mm of left/right imbalance
STEERING_DEADBAND = 7   # mm of imbalance ignored, keeps the mouse from twitching
FRONT_STOP_MM = 100     # stop when the front wall is closer than this
FRONT_RESUME_MM = 140   # and only start moving again past this (hysteresis)

# --- BLE telemetry ---
BLE_LOCAL_NAME = 'Micromouse'
BLE_TX_INTERVAL = 0.05  # seconds between notifications (~20 Hz)
