/*
 * ============================================================================
 *  EncoderMotor_Demo.ino
 * ============================================================================
 *  Example for the EncoderMotor class:
 *    L9110H H-bridge on pins 4 and 5
 *    N20 gearmotor with quadrature Hall/magnetic encoder on pins 2 and 3
 *
 *  Put EncoderMotor.h in the SAME folder as this .ino file.
 *  Install "Encoder" by Paul Stoffregen from the Library Manager.
 *
 *  Wiring:
 *    Arduino 4  -> L9110H IA (input A)
 *    Arduino 5  -> L9110H IB (input B)
 *    L9110H OA / OB -> motor terminals
 *    L9110H VCC -> motor supply (+), GND -> common ground with Arduino
 *    Encoder VCC -> 3.3V or 5V (check your motor), GND -> GND
 *    Encoder A  -> Arduino 2   (interrupt pin)
 *    Encoder B  -> Arduino 3   (interrupt pin)
 *
 *  Note for Arduino Micro / Leonardo (32U4): pins 2 and 3 are also the I2C
 *  pins (SDA/SCL). If you need I2C later, move the encoder to 0 and 1 or 7
 *  (0 and 1 are Serial1, which USB Serial does NOT use on the Micro).
 *
 *  The demo runs a repeating sequence:
 *    1. spin forward at 60 RPM for 3 s
 *    2. spin backward at 30 RPM for 3 s
 *    3. go to 0 degrees
 *    4. go to +180 degrees at 40 RPM
 *    5. go back -90 degrees (relative) at 80 RPM, four times in a row
 *    6. pause, repeat
 *  and prints status over Serial (open Serial Plotter or Monitor at 115200).
 * ============================================================================
 */

#include "EncoderMotor.h"

// ----------------------------------------------------------------------------
//  Pin and motor definitions - change these to match your hardware
// ----------------------------------------------------------------------------
const uint8_t DRV_PIN_A = 4;     // L9110H input A
const uint8_t DRV_PIN_B = 5;     // L9110H input B
const uint8_t ENC_PIN_A = 2;     // encoder channel A (interrupt pin)
const uint8_t ENC_PIN_B = 3;     // encoder channel B (interrupt pin)

// !!! CHECK YOUR MOTOR'S PRODUCT PAGE FOR THESE TWO NUMBERS !!!
// ENCODER_PPR : pulses per revolution of the MOTOR shaft (before gearbox),
//               per channel.
// GEAR_RATIO  : the gearbox reduction (e.g. 50, 150, 298 for common N20s).
// If the reported angle is off by a constant factor (e.g. 360 commanded
// gives 90 actual), one of these is wrong.
const float ENCODER_PPR = 7.0;
const float GEAR_RATIO  = 298.0;

// ----------------------------------------------------------------------------
//  Create the motor object
// ----------------------------------------------------------------------------
EncoderMotor motor(DRV_PIN_A, DRV_PIN_B,
                   ENC_PIN_A, ENC_PIN_B,
                   ENCODER_PPR, GEAR_RATIO);

// ----------------------------------------------------------------------------
//  Demo sequence bookkeeping
// ----------------------------------------------------------------------------
int           demoStep  = 0;   // which step of the sequence we're on
unsigned long stepStart = 0;   // millis() when the current step began
int           repeatCnt = 0;   // used for the repeated moveBy step

// Start a new demo step: remember when it started.
void nextStep() {
  demoStep++;
  stepStart = millis();
}

// How long the current step has been running, in ms.
unsigned long stepTime() {
  return millis() - stepStart;
}

void setup() {
  Serial.begin(115200);

  motor.begin();   // sets pin modes, motor starts coasting

  // ---- Tuning: start with these, then adjust for your motor -------------
  //
  // Step 1 - find minPWM:
  //   temporarily replace the demo with motor.setPWM(x) and increase x
  //   until the motor JUST starts turning. Put that number here.
  motor.setMinPWM(60);

  // Step 2 - speed gains (kff, kp, ki):
  //   with kp = ki = 0, run setRPM(60) and adjust kff until the measured
  //   RPM is roughly 60. Then raise kp until it reacts quickly without
  //   jitter, then raise ki until it holds speed when you load the shaft.
  motor.setSpeedGains(1.5, 1.0, 5.0);

  // Step 3 - position gain (RPM per degree of error):
  //   higher = stiffer. If it buzzes or oscillates at the target, lower it.
  motor.setPositionGain(1.0);

  // Smooth starts and stops (RPM per second). 0 = instant.
  motor.setAcceleration(200);

  // A move counts as finished within this many degrees of the target.
  motor.setPositionTolerance(2.0);

  // Keep holding position after moves (true) or coast (false).
  motor.setHoldPosition(true);

  // If the motor runs away at full speed when you start closed-loop
  // control, the motor and encoder disagree on direction: flip one.
  motor.setReversed(false, false);

  // If your supply is above the motor's rated voltage, cap the PWM, e.g.
  // motor.setMaxPWM(170);

  stepStart = millis();
}

void loop() {
  // ALWAYS call update() every pass through loop(). Keep loop() free of
  // long delay()s, or the controller stops controlling.
  motor.update();

  runDemo();
  printStatus();
}

// ----------------------------------------------------------------------------
//  Demo sequence - a simple state machine, so nothing ever blocks.
// ----------------------------------------------------------------------------
void runDemo() {
  switch (demoStep) {

    case 0:                                   // spin forward at 60 RPM
      motor.setRPM(60);
      nextStep();
      break;

    case 1:                                   // ...for 3 seconds
      if (stepTime() > 3000) {
        motor.setRPM(-30);                    // then backward at 30 RPM
        nextStep();
      }
      break;

    case 2:                                   // ...for 3 seconds
      if (stepTime() > 3000) {
        motor.moveTo(0, 60);                  // then return to 0 degrees
        nextStep();
      }
      break;

    case 3:                                   // wait until at 0
      if (motor.isDone()) {
        motor.moveTo(180, 40);                // go to +180 at 40 RPM
        nextStep();
      }
      break;

    case 4:                                   // wait until at 180
      if (motor.isDone()) {
        repeatCnt = 0;
        motor.moveBy(-90, 80);                // first of four -90 steps
        nextStep();
      }
      break;

    case 5:                                   // four relative -90 moves
      if (motor.isDone()) {
        repeatCnt++;
        if (repeatCnt < 4) {
          motor.moveBy(-90, 80);
        } else {
          nextStep();                         // ends at 180 - 360 = -180
        }
      }
      break;

    case 6:                                   // hold for 2 s, then restart
      if (stepTime() > 2000) {
        demoStep = 0;
      }
      break;
  }
}

// ----------------------------------------------------------------------------
//  Print status 10 times per second (printing every loop would slow the
//  controller down). Format works in both Serial Monitor and Serial Plotter.
// ----------------------------------------------------------------------------
void printStatus() {
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint < 100) return;
  lastPrint = millis();

  Serial.print("step:");     Serial.print(demoStep);
  Serial.print(" rpm:");     Serial.print(motor.getRPM(), 1);
  Serial.print(" angle:");   Serial.print(motor.getAngle(), 1);
  Serial.print(" target:");  Serial.print(motor.getTargetAngle(), 1);
  Serial.print(" pwm:");     Serial.println(motor.getPWM());
}
