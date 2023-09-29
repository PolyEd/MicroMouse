This sensor is a simple one (not I2C) and has power pins + analog pins for XYZ. Z seems like acceleration but XY seems like tilt. Also an extra 5v->3v power inverter onboard which is nice, on 3vo.

### Pins (can be changed at the top of the code)
Arduino | ADXL335
--------|--------
A0      | Xout
A1      | Yout
A2      | Zout
GND     | GND
VCC/5V  | VCC
