/*
 * ============================================================================
 *  EncoderMotor_Simple.ino   (for EncoderMotor.h version 3)
 * ============================================================================
 *  Minimal demo of the EncoderMotor class. Every feature is listed below.
 *  Exactly ONE command in setup() is active; to try another, comment the
 *  active one out and uncomment a different one.
 *
 *  Version 3: smooth under load - no hard-coded boosts, the PID handles
 *  load, and a following-error limit prevents catch-up surges.
 *
 *  As written, the demo swings the shaft back and forth between -180 and
 *  +180 degrees forever (see the bottom of loop()).
 *
 *  Folder layout (folder name must match the .ino name):
 *    Documents/Arduino/EncoderMotor_Simple/
 *        EncoderMotor_Simple.ino
 *        EncoderMotor.h            <- the version 3 file
 *
 *  Needs the "Encoder" library by Paul Stoffregen (Library Manager).
 *
 *  Motor: ServoCity Premium N20, 30:1, 900 RPM @ 12V, with encoder.
 *         Run straight from a 2S LiPo -> roughly 560 RPM top speed.
 * ============================================================================
 */

#include "EncoderMotor.h"

// ----------------------------------------------------------------------------
//  Create the motor:
//    driver pin A, driver pin B, encoder pin A, encoder pin B,
//    encoder pulses per MOTOR rev, gear ratio
//  ServoCity 30:1 page: 3 cycles per motor rev, exact ratio 29.977:1
//    -> 3 x 4 x 29.977 = 359.72 counts per output revolution (matches the
//       "Countable Events Per Revolution" on their page)
// ----------------------------------------------------------------------------
//  BOTH driver pins must be PWM pins. Arduino Micro PWM pins: 3, 5, 6, 9,
//  10, 11, 13 (pin 4 is NOT PWM on the Micro!).
// ----------------------------------------------------------------------------
EncoderMotor motor(6, 5, 2, 3, 3.0, 29.977);

// Optional 7th argument: quadrature multiplier (default 4).
// Use 1 only if your PPR number is already given in "counts".
// EncoderMotor motor(4, 5, 2, 3, 3.0, 29.977, 4);

void setup() {
  Serial.begin(115200);
  motor.begin();                         // REQUIRED: sets up pins, motor off

  // ==========================================================================
  //  SETTINGS (all optional - defaults shown in comments)
  // ==========================================================================

  // Deadband (fixed friction compensation): start the motor with setPWM(x),
  // then LOWER x until it just stops, in each direction. Use the "keeps
  // turning" value, not the higher "breaks loose" value - the PID's
  // integral adds whatever extra push is needed to get going or carry load.
  motor.setMinPWM(50, 50);               // forward, reverse (default 0, 0)

  // Speed mode (setRPM). kff ~ 255 / top RPM.
  motor.setSpeedGains(0.25, 0.05, 1.5);  // kff, kp, ki (default 0.45, 0.30, 1.5)

  // Position mode (moveTo / moveBy). Units: PWM per degree, PWM per
  // degree-second, PWM per RPM.
  motor.setPositionGains(3.5, 2.0, 0.45);// kp, ki, kd (default 3.5, 2.0, 0.45)

  motor.setAcceleration(1000);           // RPM per second, 0 = instant (default 1000)
  motor.setPositionTolerance(2.0);       // degrees counted as "arrived" (default 2)
  motor.setStallProtection(500);         // cut power after 0.5 s stalled (default 0 = off)
  motor.setMaxLag(30);                   // plan may lead the shaft by max 30 deg (default 30)

  // motor.setMaxPWM(200);               // cap PWM / average voltage (default 255)
  // motor.setIntegralLimit(0.4);       // max extra push from ki, fraction of maxPWM (default 0.4)
  // motor.setSlewRate(5000);           // max PWM change per second (default 0 = off)
  // motor.setHoldPosition(false);       // coast after moves instead of holding (default true)
  // motor.setBrakeWhenHolding(false);   // coast instead of brake inside the zone (default true)
  // motor.setReversed(true, false);     // flip motor dir, encoder dir (default false, false)
  // motor.setUpdateInterval(5);         // control loop period in ms (default 5)
  // motor.setSpeedWindow(5);            // speed measured over N steps (default 5 = 25 ms)
  // motor.setRPMFilter(0.5);            // extra speed smoothing 0..1 (default 0.5)
  // motor.resetPosition(0);             // call the current shaft angle 0 (homing)

  // ==========================================================================
  //  COMMANDS - keep exactly ONE uncommented
  // ==========================================================================

  // --- Speed control --------------------------------------------------------
  // motor.setRPM(200);                  // spin forward at 200 RPM, forever
  // motor.setRPM(-100);                 // spin backward at 100 RPM

  // --- Position control -----------------------------------------------------
  motor.moveTo(180, 150);                // go to absolute angle 180 at 150 RPM
  // motor.moveTo(-90, 100);             // go to absolute angle -90 at 100 RPM
  // motor.moveBy(360, 150);             // go 360 degrees from where it is now

  // --- Open loop / stopping (no encoder feedback) ---------------------------
  // motor.setPWM(60);                   // raw PWM -255..255, use to find minPWM
  // motor.stop();                       // coast (free-wheel)
  // motor.brake();                      // short the motor terminals to stop hard
}

int dir = -1;                            // next swing direction for the demo

void loop() {
  motor.update();                        // REQUIRED: call constantly, no long delay()s

  // --- Demo: swing back and forth ------------------------------------------
  // Checked EVERY loop (not inside the print block) so the next move starts
  // the moment the previous one finishes. Delete this block when testing
  // one of the other commands above.
  if (motor.isDone() && !motor.isStalled()) {
    motor.moveTo(dir * 180, 150);
    dir = -dir;
  }

  // Print status 10x per second (works in Serial Monitor and Serial Plotter)
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 100) {
    lastPrint = millis();

    Serial.print("angle:");    Serial.print(motor.getAngle(), 1);        // output shaft degrees
    Serial.print(" target:");  Serial.print(motor.getTargetAngle(), 1);  // where the move is headed
    Serial.print(" profile:"); Serial.print(motor.getProfileAngle(), 1); // smooth moving setpoint
    Serial.print(" rpm:");     Serial.print(motor.getRPM(), 1);          // measured speed
    Serial.print(" pwm:");     Serial.println(motor.getPWM());           // signed PWM sent

    // Other readouts you can print:
    // motor.isDone()           - 1 when a move has finished
    // motor.isStalled()        - 1 if stall protection cut the power
    // motor.getTargetRPM()     - speed requested by setRPM()
    // motor.getCount()         - raw encoder count
    // motor.getCountsPerRev()  - counts per output shaft revolution
    // motor.isMoving()         - true while a position move is underway
    // motor.getMode()          - MODE_IDLE / MODE_OPEN_LOOP / MODE_SPEED / MODE_POSITION
  }
}
