/*
 * ============================================================================
 *  EncoderMotor.h
 * ============================================================================
 *  Closed-loop control of a brushed DC gearmotor with a quadrature (Hall /
 *  magnetic) encoder, driven by a 2-input H-bridge such as the L9110H.
 *
 *  What it gives you:
 *    - setRPM(rpm)            -> hold a constant speed (signed: + / - direction)
 *    - moveTo(deg, rpm)       -> go to an absolute angle at a given max speed
 *    - moveBy(deg, rpm)       -> go a relative distance at a given max speed
 *    - stop() / brake()       -> coast or actively brake
 *    - getRPM(), getAngle()   -> read back what the motor is actually doing
 *
 *  Everything is NON-BLOCKING: you must call update() as often as possible
 *  from loop(). update() decides internally when it is time to run the
 *  control math (every 10 ms by default).
 *
 *  Requires the "Encoder" library by Paul Stoffregen
 *  (Arduino IDE -> Library Manager -> search "Encoder").
 *
 *  How the L9110H is driven (2 inputs, no separate enable/PWM pin):
 *      IN_A    IN_B    Result
 *      PWM     0       forward, speed set by PWM duty
 *      0       PWM     reverse, speed set by PWM duty
 *      0       0       coast (motor free-wheels)
 *      1       1       brake (motor terminals shorted together)
 *  Because speed is set by PWM on whichever input is active, BOTH driver pins
 *  must be PWM-capable on your board.
 * ============================================================================
 */

#ifndef ENCODER_MOTOR_H
#define ENCODER_MOTOR_H

#include <Arduino.h>
#include <Encoder.h>

class EncoderMotor {
public:

  // --------------------------------------------------------------------------
  //  Modes the controller can be in
  // --------------------------------------------------------------------------
  enum Mode {
    MODE_IDLE,       // not controlling anything, driver outputs off (coast)
    MODE_OPEN_LOOP,  // raw PWM set by you with setPWM(), no feedback used
    MODE_SPEED,      // closed-loop speed control (setRPM)
    MODE_POSITION    // closed-loop position control (moveTo / moveBy)
  };

  // --------------------------------------------------------------------------
  //  Constructor
  // --------------------------------------------------------------------------
  //  drvPinA, drvPinB : the two L9110H input pins (both must be PWM-capable)
  //  encPinA, encPinB : the two encoder signal pins (use INTERRUPT pins for
  //                     reliable counting; on an Arduino Micro / 32U4 those
  //                     are 0, 1, 2, 3 and 7)
  //  encoderPPR       : encoder pulses per ONE revolution of the MOTOR shaft
  //                     (before the gearbox), per channel. Check the motor's
  //                     product page.
  //  gearRatio        : gearbox reduction, e.g. 150 for a 150:1 gearmotor
  //  quadMultiplier   : how many counts the Encoder library produces per
  //                     pulse. The library counts every rising AND falling
  //                     edge on BOTH channels, so this is 4 for a normal
  //                     quadrature encoder. Only change it if your PPR figure
  //                     is already given in "counts" (then use 1).
  // --------------------------------------------------------------------------
  EncoderMotor(uint8_t drvPinA, uint8_t drvPinB,
               uint8_t encPinA, uint8_t encPinB,
               float encoderPPR, float gearRatio,
               uint8_t quadMultiplier = 4)
    : _drvA(drvPinA),
      _drvB(drvPinB),
      _enc(encPinA, encPinB)   // the Encoder object is built right here with our pins
  {
    // Total encoder counts for ONE revolution of the OUTPUT shaft
    // (the shaft you actually care about, after the gearbox).
    _countsPerRev = encoderPPR * quadMultiplier * gearRatio;
  }

  // --------------------------------------------------------------------------
  //  begin() - call once from setup()
  // --------------------------------------------------------------------------
  void begin() {
    pinMode(_drvA, OUTPUT);
    pinMode(_drvB, OUTPUT);
    coastOutputs();                 // start with the motor off
    _lastMicros = micros();         // timing reference for the first update
    _lastCount  = getCount();       // speed reference for the first update
  }

  // ==========================================================================
  //  CONFIGURATION  (all optional - sensible defaults are already set)
  // ==========================================================================

  // Speed loop gains. Output of the speed loop is PWM (0..255):
  //   PWM = kff * wantedRPM            (feed-forward: "about this much PWM
  //                                     gives about this much speed")
  //       + kp  * speedError           (proportional correction)
  //       + ki  * integral(speedError) (slowly removes leftover error)
  // Tune kff first (with kp = ki = 0), then add kp, then ki.
  void setSpeedGains(float kff, float kp, float ki) {
    _kff = kff; _kp = kp; _ki = ki;
  }

  // Position gain: how many extra RPM to command per DEGREE of position
  // error. Higher = stiffer and snappier, too high = oscillation / buzzing.
  void setPositionGain(float rpmPerDegree) { _kpPos = rpmPerDegree; }

  // Smallest PWM that actually makes the motor turn (the "deadband").
  // Any non-zero output gets mapped into minPWM..maxPWM so the controller
  // never wastes effort on PWM values too weak to move the motor.
  void setMinPWM(int minPWM) { _minPWM = constrain(minPWM, 0, 255); }

  // Upper PWM limit. Useful if your supply voltage is higher than the
  // motor's rating (e.g. 6V motor on 9V -> limit to about 170).
  void setMaxPWM(int maxPWM) { _maxPWM = constrain(maxPWM, 0, 255); }

  // Acceleration limit in RPM per second (applies to speed changes AND to
  // the motion profile of position moves). 0 = no limit (instant changes).
  void setAcceleration(float rpmPerSecond) { _accelRPMps = fabs(rpmPerSecond); }

  // How close (in degrees) the shaft must get to the target for a position
  // move to count as finished.
  void setPositionTolerance(float degrees) { _tolDeg = fabs(degrees); }

  // If true (default), the motor keeps actively holding its position after a
  // move finishes. If false, it coasts once the move is done (saves power,
  // but the shaft can be pushed away by load).
  void setHoldPosition(bool hold) { _hold = hold; }

  // How often the control math runs, in milliseconds. 10 ms is a good
  // default. Shorter = more responsive but noisier speed readings.
  void setUpdateInterval(uint16_t ms) { _intervalUs = (uint32_t)ms * 1000UL; }

  // Smoothing of the measured speed, 0..1. Lower = smoother but laggier.
  // 1.0 = no filtering at all.
  void setRPMFilter(float alpha) { _rpmAlpha = constrain(alpha, 0.01f, 1.0f); }

  // Flip directions without rewiring.
  //   motorReversed   : swaps which driver pin means "forward"
  //   encoderReversed : flips the sign of the encoder count
  // If the motor runs away at full speed under closed-loop control, the
  // motor and encoder directions disagree: flip ONE of these.
  void setReversed(bool motorReversed, bool encoderReversed) {
    _motorRev = motorReversed;
    _encRev   = encoderReversed;
  }

  // ==========================================================================
  //  COMMANDS
  // ==========================================================================

  // Closed-loop constant speed. Positive = forward, negative = reverse.
  void setRPM(float rpm) {
    if (_mode != MODE_SPEED) {
      // Coming from another mode: start the ramp from the CURRENT real speed
      // so there is no jump, and clear the integral from the old mode.
      _cmdRPM = _rpm;
      _integ  = 0;
    }
    _mode      = MODE_SPEED;
    _targetRPM = rpm;
  }

  // Move to an ABSOLUTE angle (degrees of the output shaft, relative to
  // where the shaft was at power-up or the last resetPosition()).
  // maxRPM is the cruising speed of the move (sign is ignored).
  void moveTo(float degrees, float maxRPM) {
    _goalCounts = degreesToCounts(degrees);
    _moveRPM    = fabs(maxRPM);

    if (_mode != MODE_POSITION) {
      // Starting a fresh move: the motion profile's "virtual shaft" starts
      // exactly where the real shaft is now, moving at the real speed.
      _spCounts = (float)getCount();
      _profRPM  = _rpm;
      _integ    = 0;
    }
    // If we were ALREADY in a move, the profile just continues smoothly
    // from wherever it currently is toward the new goal.

    _mode     = MODE_POSITION;
    _moveDone = false;
  }

  // Move RELATIVE to the current target (or current position if idle).
  // Chaining moveBy(90) four times gives exactly 360 with no drift.
  void moveBy(float degrees, float maxRPM) {
    float from = (_mode == MODE_POSITION) ? countsToDegrees(_goalCounts)
                                          : getAngle();
    moveTo(from + degrees, maxRPM);
  }

  // Open-loop: just set a raw PWM (-255..255). No encoder feedback.
  // Handy for testing wiring and for finding your minPWM.
  void setPWM(int pwm) {
    _mode = MODE_OPEN_LOOP;
    writePWM(pwm, false);   // false = raw, no deadband remapping
  }

  // Stop controlling and let the motor free-wheel.
  void stop() {
    _mode = MODE_IDLE;
    _integ = 0;
    coastOutputs();
  }

  // Stop controlling and short the motor terminals (stops much faster
  // than coasting).
  void brake() {
    _mode = MODE_IDLE;
    _integ = 0;
    analogWrite(_drvA, 255);
    analogWrite(_drvB, 255);
    _pwm = 0;
  }

  // Redefine the current shaft position as 'degrees' (default 0).
  // Example: after hitting a limit switch, call resetPosition(0) to "home".
  void resetPosition(float degrees = 0) {
    long c = (long)degreesToCounts(degrees);
    _enc.write(_encRev ? -c : c);   // write raw value, honouring reversal
    _lastCount  = c;                // avoid a fake speed spike next update
    _spCounts   = c;
    _goalCounts = c;
  }

  // ==========================================================================
  //  STATUS
  // ==========================================================================

  float getRPM()          const { return _rpm; }                 // measured, filtered
  float getAngle()              { return countsToDegrees(getCount()); }
  float getTargetAngle()  const { return countsToDegrees(_goalCounts); }
  float getTargetRPM()    const { return _targetRPM; }
  int   getPWM()          const { return _pwm; }                 // last PWM sent (signed)
  Mode  getMode()         const { return _mode; }
  float getCountsPerRev() const { return _countsPerRev; }

  // Raw encoder count (with encoder reversal applied).
  long getCount() {
    long c = _enc.read();
    return _encRev ? -c : c;
  }

  // True once a position move has reached its target (within tolerance).
  bool isDone() const { return _mode == MODE_POSITION ? _moveDone : true; }

  // True while a position move is still in progress.
  bool isMoving() const { return _mode == MODE_POSITION && !_moveDone; }

  // ==========================================================================
  //  update() - CALL THIS AS OFTEN AS POSSIBLE FROM loop()
  // ==========================================================================
  //  It returns immediately unless a full control interval has passed, so
  //  calling it thousands of times per second costs almost nothing.
  //  Avoid long delay() calls in loop() - they starve the controller.
  // --------------------------------------------------------------------------
  void update() {
    uint32_t now = micros();
    uint32_t elapsed = now - _lastMicros;     // unsigned math handles micros() rollover
    if (elapsed < _intervalUs) return;        // not time yet
    _lastMicros = now;

    float dt = elapsed / 1000000.0f;          // seconds since last control step

    // ---- 1. Measure speed --------------------------------------------------
    measureSpeed(dt);

    // ---- 2. Decide what speed we WANT right now, depending on mode --------
    float wantRPM;
    switch (_mode) {

      case MODE_SPEED:
        // Ramp the commanded speed toward the target at the accel limit.
        _cmdRPM = rampToward(_cmdRPM, _targetRPM, dt);
        wantRPM = _cmdRPM;
        break;

      case MODE_POSITION:
        wantRPM = updatePositionProfile(dt);
        if (_moveDone && !_hold) {             // finished and told not to hold
          _mode = MODE_IDLE;
          coastOutputs();
          return;
        }
        break;

      default:
        // IDLE or OPEN_LOOP: nothing to control.
        return;
    }

    // ---- 3. Turn wanted speed into PWM via the speed loop -----------------
    runSpeedLoop(wantRPM, dt);
  }

private:

  // ---------------- hardware ----------------
  uint8_t _drvA, _drvB;     // L9110H input pins
  Encoder _enc;             // Paul Stoffregen's interrupt-driven encoder reader

  // ---------------- geometry ----------------
  float _countsPerRev;      // encoder counts per OUTPUT shaft revolution

  // ---------------- tuning (defaults) ----------------
  float    _kff   = 1.5f;   // PWM per RPM (feed-forward)
  float    _kp    = 1.0f;   // PWM per RPM of error
  float    _ki    = 5.0f;   // PWM per (RPM * second) of accumulated error
  float    _kpPos = 1.0f;   // RPM per degree of position error
  int      _minPWM = 0;     // deadband compensation
  int      _maxPWM = 255;
  float    _accelRPMps = 300.0f;   // RPM per second, 0 = unlimited
  float    _tolDeg     = 2.0f;     // "arrived" tolerance in degrees
  bool     _hold       = true;     // keep holding after a move
  uint32_t _intervalUs = 10000UL;  // 10 ms control period
  float    _rpmAlpha   = 0.3f;     // speed filter strength
  bool     _motorRev = false;
  bool     _encRev   = false;

  // ---------------- state ----------------
  Mode     _mode = MODE_IDLE;
  uint32_t _lastMicros = 0;
  long     _lastCount  = 0;
  float    _rpm        = 0;   // measured (filtered) speed
  float    _integ      = 0;   // speed-loop integral accumulator
  int      _pwm        = 0;   // last signed PWM actually written

  float _targetRPM = 0;       // speed mode: where we're heading
  float _cmdRPM    = 0;       // speed mode: ramped command

  float _goalCounts = 0;      // position mode: final target
  float _spCounts   = 0;      // position mode: moving "virtual shaft" setpoint
  float _profRPM    = 0;      // position mode: speed of the virtual shaft
  float _moveRPM    = 0;      // position mode: cruise speed of the move
  bool  _moveDone   = true;

  // ==========================================================================
  //  Unit conversions
  // ==========================================================================
  float degreesToCounts(float deg) const { return deg / 360.0f * _countsPerRev; }
  float countsToDegrees(float c)   const { return c * 360.0f / _countsPerRev; }
  float rpmToCountsPerSec(float r) const { return r * _countsPerRev / 60.0f; }
  float countsPerSecToRPM(float c) const { return c * 60.0f / _countsPerRev; }

  // ==========================================================================
  //  Speed measurement: counts moved since last time -> RPM, then smoothed.
  // ==========================================================================
  void measureSpeed(float dt) {
    long count = getCount();
    long delta = count - _lastCount;      // counts moved during this interval
    _lastCount = count;

    float rawRPM = countsPerSecToRPM(delta / dt);

    // Exponential moving average: blends the new reading into the old one.
    // With only a handful of counts per 10 ms, raw readings are jumpy, and
    // this keeps the speed loop from reacting to that noise.
    _rpm += _rpmAlpha * (rawRPM - _rpm);
  }

  // ==========================================================================
  //  Move 'current' toward 'target' no faster than the acceleration limit.
  // ==========================================================================
  float rampToward(float current, float target, float dt) const {
    if (_accelRPMps <= 0) return target;          // unlimited acceleration
    float maxStep = _accelRPMps * dt;             // biggest change allowed this step
    return current + constrain(target - current, -maxStep, maxStep);
  }

  // ==========================================================================
  //  Position mode: trapezoidal motion profile + position correction.
  // --------------------------------------------------------------------------
  //  Instead of yanking the motor straight at the goal, a "virtual shaft"
  //  (_spCounts) travels smoothly to the goal: speeds up at the accel limit,
  //  cruises at _moveRPM, and slows down in time to stop exactly on target.
  //  The real motor is then made to follow the virtual shaft:
  //    wantRPM = speed of virtual shaft          (feed-forward)
  //            + kpPos * (virtual - real angle)  (catch-up correction)
  // ==========================================================================
  float updatePositionProfile(float dt) {
    float remaining = _goalCounts - _spCounts;    // distance left for the virtual shaft
    float dir = (remaining >= 0) ? 1.0f : -1.0f;

    // Highest speed from which we can still stop in the remaining distance:
    //   v^2 = 2 * a * d   ->   v = sqrt(2 * a * d)
    float cruiseRPM = _moveRPM;
    if (_accelRPMps > 0) {
      float a = rpmToCountsPerSec(_accelRPMps);   // accel in counts/s^2
      float vStop = sqrtf(2.0f * a * fabs(remaining));
      cruiseRPM = min(cruiseRPM, countsPerSecToRPM(vStop));
    }

    // Accelerate / decelerate the virtual shaft toward that speed.
    _profRPM = rampToward(_profRPM, dir * cruiseRPM, dt);

    // Advance the virtual shaft.
    float step = rpmToCountsPerSec(_profRPM) * dt;

    // If this step would reach or pass the goal, land exactly on it.
    if (fabs(step) >= fabs(remaining)) {
      _spCounts = _goalCounts;
      _profRPM  = 0;
    } else {
      _spCounts += step;
    }

    // How far the REAL shaft lags behind the virtual one, in degrees.
    float errDeg = countsToDegrees(_spCounts - (float)getCount());

    // Done when the profile has finished AND the real shaft is close enough.
    if (_spCounts == _goalCounts && fabs(errDeg) <= _tolDeg) {
      _moveDone = true;
    }

    // Final speed request: follow the profile + correct the lag.
    float want = _profRPM + _kpPos * errDeg;

    // Keep corrections sane so a big error can't command a crazy speed.
    float limit = _moveRPM * 1.25f + 5.0f;
    return constrain(want, -limit, limit);
  }

  // ==========================================================================
  //  Speed loop: wanted RPM -> PWM using feed-forward + PI.
  // ==========================================================================
  void runSpeedLoop(float wantRPM, float dt) {
    float err = wantRPM - _rpm;                   // positive = too slow

    // Integrate error over time (removes steady-state error, e.g. under load).
    _integ += err * dt;

    // Anti-windup: never let the integral term alone exceed full PWM,
    // otherwise it "charges up" while saturated and causes big overshoot.
    if (_ki > 0) {
      float maxInteg = _maxPWM / _ki;
      _integ = constrain(_integ, -maxInteg, maxInteg);
    }

    float out = _kff * wantRPM + _kp * err + _ki * _integ;

    // If we want to be stopped and basically are, output exactly zero
    // (prevents a quiet hum / creep at standstill in speed mode).
    if (_mode == MODE_SPEED && wantRPM == 0 && fabs(_rpm) < 1.0f) {
      out = 0;
      _integ = 0;
    }

    writePWM((int)out, true);   // true = apply deadband remapping
  }

  // ==========================================================================
  //  Send a signed PWM (-255..255) to the L9110H.
  //  'remap' = squeeze non-zero values into minPWM..maxPWM (deadband fix).
  // ==========================================================================
  void writePWM(int pwm, bool remap) {
    pwm = constrain(pwm, -255, 255);

    int mag = abs(pwm);
    if (remap && mag > 0) {
      // 1..255 -> minPWM..maxPWM, so even tiny requests produce real torque
      mag = map(mag, 1, 255, max(_minPWM, 1), _maxPWM);
    } else {
      mag = min(mag, _maxPWM);
    }

    bool forward = (pwm >= 0);
    if (_motorRev) forward = !forward;            // honour motor reversal

    // IMPORTANT: always set the "off" pin to 0 BEFORE driving the other,
    // so both inputs are never PWM'd at the same time.
    if (mag == 0) {
      coastOutputs();
    } else if (forward) {
      analogWrite(_drvB, 0);
      analogWrite(_drvA, mag);
    } else {
      analogWrite(_drvA, 0);
      analogWrite(_drvB, mag);
    }

    _pwm = (pwm >= 0) ? mag : -mag;               // remember what we sent
  }

  // Both inputs low = motor free-wheels.
  void coastOutputs() {
    analogWrite(_drvA, 0);
    analogWrite(_drvB, 0);
    _pwm = 0;
  }
};

#endif // ENCODER_MOTOR_H
