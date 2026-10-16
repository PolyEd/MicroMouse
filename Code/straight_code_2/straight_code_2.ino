// straight_code_2 — improved version of straight_code/straight_code.ino
//
// Same idea (drive straight down a corridor using side ToF sensors,
// stop when a front wall is close), but with:
//   - arrays/loops instead of 5x copy-pasted sensor code
//   - invalid readings (timeouts / out-of-range) are skipped instead of
//     poisoning the running average
//   - proportional steering (smooth correction) instead of bang-bang
//     "one wheel at full speed" pivots, with a small deadband for jitter
//   - no blocking delay() calls in the control loop, so it reacts to
//     sensor data as fast as the sensors can produce it
//   - hysteresis on the front-wall stop so the mouse doesn't stutter
//     right at the stop distance
//   - BLE telemetry without String concatenation (avoids heap churn)
//
// Sensor roles (same as the original):
//   sensor 3 (index 2) = front, sensors 2 and 5 (indexes 1 and 4) = sides.
//   If the right side (sensor 5) reads farther than the left (sensor 2),
//   the mouse is drifting toward the left wall and steers right.

#include <Wire.h>
#include <VL53L0X.h>
#include <Adafruit_BLE.h>
#include <Adafruit_BluefruitLE_SPI.h>

// ---- BLE ----
#define BLUETOOTH_IRQ 7
#define BLUETOOTH_SS 17
#define BLUETOOTH_RST -1
#define VERBOSE_MODE true

// ---- Sensor pins / addresses (same wiring as straight_code) ----
// NOTE: XSHUT_3 and BLUETOOTH_IRQ are both pin 7, inherited from the
// original wiring. Kept identical so this runs on the same robot.
#define XSHUT_1 5
#define XSHUT_2 6
#define XSHUT_3 7
#define XSHUT_4 8

#define FIRST_SENSOR_ADDRESS 42 // sensors 1-4 get addresses 42-45, sensor 5 stays at default

// ---- Motor pins (M1 = left motor, M2 = right motor) ----
#define M1_BACKWARD A5
#define M1_FORWARD A4
#define M2_FORWARD A3
#define M2_BACKWARD 13

// ---- Driving parameters (tune these) ----
#define BASE_SPEED 129        // PWM (0-255) applied to both motors when centered
#define STEERING_KP 3.0       // PWM units per mm of left/right imbalance
#define STEERING_DEADBAND 7   // mm of imbalance ignored, keeps the mouse from twitching
#define FRONT_STOP_MM 100     // stop when the front wall is closer than this
#define FRONT_RESUME_MM 140   // and only start moving again past this (hysteresis)
#define SENSOR_MAX_MM 8000   // VL53L0X returns ~8190/8191 when it sees nothing

#define NUM_SENSORS 5
#define NUM_READINGS 4        // readings averaged per sensor (was 2)

// Index 4 (sensor 5) has no XSHUT pin, it just keeps the default address.
const uint8_t XSHUT_PINS[NUM_SENSORS] = {XSHUT_1, XSHUT_2, XSHUT_3, XSHUT_4, 255};

// Sensor indexes by role
#define FRONT 2
#define LEFT 1
#define RIGHT 4

VL53L0X sensors[NUM_SENSORS];
bool sensorOK[NUM_SENSORS];

uint16_t readings[NUM_SENSORS][NUM_READINGS]; // ring buffer per sensor
uint8_t readingsIndex[NUM_SENSORS];
uint8_t readingsCount[NUM_SENSORS];
float readingsAvg[NUM_SENSORS];

Adafruit_BluefruitLE_SPI ble(BLUETOOTH_SS, BLUETOOTH_IRQ, BLUETOOTH_RST);

bool driving = true; // becomes false when the front wall is close

void setup() {
  if (!ble.begin(VERBOSE_MODE)) {
  }
  ble.echo(false);
  ble.sendCommandCheckOK(F("AT+GAPDEVNAME=Bluefruit (SPI)"));
  ble.sendCommandCheckOK(F("+++"));

  Wire.begin();

  // Bring up the XSHUT-addressed sensors one at a time (same sequence the
  // original used, just loop-ified), then init + start them all.
  for (uint8_t i = 0; i < NUM_SENSORS; i++) {
    if (XSHUT_PINS[i] != 255) {
      pinMode(XSHUT_PINS[i], OUTPUT);
      sensors[i].setAddress(FIRST_SENSOR_ADDRESS + i);
      pinMode(XSHUT_PINS[i], INPUT);
      delay(10);
    }

    sensors[i].setTimeout(500);
    sensorOK[i] = sensors[i].init();
    if (sensorOK[i]) {
      sensors[i].startContinuous(5); // ms between readings
    }
  }

  pinMode(M1_FORWARD, OUTPUT);
  pinMode(M1_BACKWARD, OUTPUT);
  pinMode(M2_FORWARD, OUTPUT);
  pinMode(M2_BACKWARD, OUTPUT);
}

void updateSensor(uint8_t i) {
  if (!sensorOK[i]) return;

  uint16_t mm = sensors[i].readRangeContinuousMillimeters();

  // A timeout returns 0 and "nothing in range" returns ~8190/8191.
  // Skip those instead of averaging them in.
  if (mm == 0 || mm >= SENSOR_MAX_MM) return;

  readings[i][readingsIndex[i]] = mm;
  readingsIndex[i] = (readingsIndex[i] + 1) % NUM_READINGS;
  if (readingsCount[i] < NUM_READINGS) readingsCount[i]++;

  uint32_t sum = 0;
  for (uint8_t j = 0; j < readingsCount[i]; j++) {
    sum += readings[i][j];
  }
  readingsAvg[i] = (float)sum / readingsCount[i];
}

void driveMotors(int leftSpeed, int rightSpeed) {
  analogWrite(M1_BACKWARD, 0);
  analogWrite(M2_BACKWARD, 0);
  analogWrite(M1_FORWARD, leftSpeed);
  analogWrite(M2_FORWARD, rightSpeed);
}

void loop() {
  for (uint8_t i = 0; i < NUM_SENSORS; i++) {
    updateSensor(i);
  }

  // Telemetry: same comma-separated format as the original, but built from
  // direct prints instead of String concatenation.
  for (uint8_t i = 0; i < NUM_SENSORS; i++) {
    ble.print((int)readingsAvg[i]);
    if (i < NUM_SENSORS - 1) ble.print(",");
  }
  ble.println();

  // Wait for a first valid front reading before moving (like the original,
  // which stayed still until runningAvg_3 > 100).
  if (readingsCount[FRONT] == 0) {
    driveMotors(0, 0);
    return;
  }

  // Front wall: stop/restart with hysteresis so a reading hovering near
  // FRONT_STOP_MM doesn't make the mouse stutter.
  if (readingsAvg[FRONT] < FRONT_STOP_MM) {
    driving = false;
  } else if (readingsAvg[FRONT] > FRONT_RESUME_MM) {
    driving = true;
  }

  if (!driving) {
    driveMotors(0, 0);
    return;
  }

  // Steering: positive error means the right wall is farther away than the
  // left one, i.e. we're drifting left, so speed up the left motor.
  // If a side sensor is dead, just drive straight on the base speed.
  int correction = 0;
  if (sensorOK[LEFT] && sensorOK[RIGHT] && readingsCount[LEFT] > 0 && readingsCount[RIGHT] > 0) {
    float error = readingsAvg[RIGHT] - readingsAvg[LEFT];
    if (error > STEERING_DEADBAND || error < -STEERING_DEADBAND) {
      correction = (int)(STEERING_KP * error);
    }
  }

  int leftSpeed = constrain(BASE_SPEED + correction, 0, 255);
  int rightSpeed = constrain(BASE_SPEED - correction, 0, 255);
  driveMotors(leftSpeed, rightSpeed);
}
