/*
 * ============================================================================
 *  EncoderMotor_Simple.ino
 * ============================================================================
 *  Minimal demo of the EncoderMotor class. Every feature is listed below.
 *  Exactly ONE command in setup() is active; to try another, comment the
 *  active one out and uncomment a different one.
 *
 *  Folder layout (folder name must match the .ino name):
 *    Documents/Arduino/EncoderMotor_Simple/
 *        EncoderMotor_Simple.ino
 *        EncoderMotor.h            <- copy it in from the other demo
 *
 *  Needs the "Encoder" library by Paul Stoffregen (Library Manager).
 * ============================================================================
 */

#include "EncoderMotor.h"

// ----------------------------------------------------------------------------
//  Create the motor:
//    driver pin A, driver pin B, encoder pin A, encoder pin B,
//    encoder pulses per MOTOR rev, gear ratio
//  !!! Set PPR and gear ratio from your motor's product page !!!
// ----------------------------------------------------------------------------
EncoderMotor motor(4, 5, 2, 3, 7.0, 298.0);

// Optional 7th argument: quadrature multiplier (default 4).
// Use 1 only if your PPR number is already given in "counts".
// EncoderMotor motor(4, 5, 2, 3, 7.0, 150.0, 4);

void setup() {
  Serial.begin(115200);
  motor.begin();                         // REQUIRED: sets up pins, motor off

  // ==========================================================================
  //  SETTINGS (all optional - defaults shown in comments)
  // ==========================================================================
  motor.setMinPWM(60);                   // lowest PWM that turns the motor (default 0)
  motor.setSpeedGains(8.0, 4.0, 2.0);    // kff, kp, ki (default 1.5, 1.0, 5.0)
  motor.setPositionGain(4.0);            // RPM per degree of error (default 1.0)
  motor.setAcceleration(40);            // RPM per second, 0 = instant (default 300)
  // motor.setMaxPWM(170);               // cap PWM if supply > motor rating (default 255)
  // motor.setPositionTolerance(2.0);    // degrees counted as "arrived" (default 2)
  // motor.setHoldPosition(false);       // coast after moves instead of holding (default true)
  // motor.setReversed(true, false);     // flip motor dir, encoder dir (default false, false)
  // motor.setUpdateInterval(10);        // control loop period in ms (default 10)
  // motor.setRPMFilter(0.3);            // speed smoothing 0..1, lower = smoother (default 0.3)
  // motor.resetPosition(0);             // call the current shaft angle 0 (homing)

  // ==========================================================================
  //  COMMANDS - keep exactly ONE uncommented
  // ==========================================================================

  // --- Speed control --------------------------------------------------------
  //motor.setRPM(60);                      // spin forward at 60 RPM, forever
  // motor.setRPM(-30);                  // spin backward at 30 RPM

  // --- Position control -----------------------------------------------------
  //motor.moveTo(360, 50);              // go to absolute angle 360 at 50 RPM
  motor.moveTo(180, 25);              // go to absolute angle -90 at 20 RPM
  //motor.moveBy(360, 30);               // go 360 degrees from where it is now

  // --- Open loop / stopping (no encoder feedback) ---------------------------
  // motor.setPWM(100);                  // raw PWM -255..255, use to find minPWM
  // motor.stop();                       // coast (free-wheel)
  // motor.brake();                      // short the motor terminals to stop hard
}
int dir = -1;
void loop() {
  motor.update();                        // REQUIRED: call constantly, no long delay()s

  // Print status 10x per second (works in Serial Monitor and Serial Plotter)
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 100) {
    lastPrint = millis();

    Serial.print("rpm:");     Serial.print(motor.getRPM(), 1);        // measured speed
    Serial.print(" angle:");  Serial.print(motor.getAngle(), 1);      // output shaft degrees
    Serial.print(" pwm:");    Serial.print(motor.getPWM());           // signed PWM sent
    Serial.print(" done:");   Serial.println(motor.isDone());         // 1 when move finished

    // Other readouts you can print:
    // motor.getTargetAngle()   - where a position move is headed
    // motor.getTargetRPM()     - speed requested by setRPM()
    // motor.getCount()         - raw encoder count
    // motor.getCountsPerRev()  - counts per output shaft revolution
    // motor.isMoving()         - true while a position move is underway
    // motor.getMode()          - MODE_IDLE / MODE_OPEN_LOOP / MODE_SPEED / MODE_POSITION

    if(motor.isDone()){
      motor.moveTo(dir*180, 25); 
      dir = -dir;
    }
  }
}