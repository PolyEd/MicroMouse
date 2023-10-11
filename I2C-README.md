## This is a short FAQ about I2C.

### What sensors in this project use I2C?
- The VL53L0X time-of-flight distance sensor
- The HMC5883 Magnetometer

## What pins are used for I2C?
Which pins I2C uses varies from board to board. Besides the power pins, I2C uses two other pins, generally called SCL and SDA. On Arduino Uno, these correspond to A5 and A4. On Pro Micro, pins 3 and 2 are used.

## How can I connect two I2C sensors to the board?
You can just connect them both to the same pins and connect the grounds.
