/*
 * ============================================================================
 *  EncoderMotor.h   (version 3 - smooth under load)
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
 *  control math (every 5 ms by default).
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
 *
 *  ---------------------------------------------------------------------------
 *  WHAT CHANGED IN VERSION 2 (why version 1 was jumpy):
 *
 *   1. Position control no longer goes "position -> speed loop -> PWM".
 *      Stacking two loops on a fast motor with a coarse encoder made them
 *      fight each other. Now position control computes PWM DIRECTLY:
 *          PWM = feed-forward (profile speed)
 *              + kp * position error          (spring pulling toward target)
 *              + kd * speed error             (damper, stops overshoot)
 *              + ki * accumulated error       (only during final approach)
 *
 *   2. Arrival dead-zone. Once the shaft is inside the tolerance, the motor
 *      output goes to zero (brake by default) instead of constantly nudging
 *      back and forth. It only wakes up again if pushed clearly out of the
 *      zone (hysteresis). No more hunting / buzzing at the target.
 *
 *   3. Deadband compensation is now ADDED on top of the controller output
 *      instead of squeezing every tiny request up to minPWM. Separate
 *      forward / reverse minimums handle gearboxes that are stiffer one way.
 *
 *   4. Speed is measured over a short window of samples (default 25 ms)
 *      instead of a single control step, so a low-resolution encoder gives a
 *      smooth, usable reading.
 *
 *   5. Optional stall protection cuts power if the motor is pushing hard
 *      but not moving (protects the motor, gearbox and driver).
 *
 *  WHAT CHANGED IN VERSION 3 (why version 2 surged under load):
 *
 *   - Version 2 had hard-coded switches: the friction offset doubled below
 *     20 RPM, and the integral only ran below 30 RPM. Loading the motor
 *     slowed it across those thresholds, so the push JUMPED, and when the
 *     load let go the motor surged. Those switches are gone.
 *   - Load is now handled purely by the PID: the integral is always on,
 *     with proper anti-windup (no charging while saturated, a hard limit,
 *     and a reset when the shaft crosses the target).
 *   - Optional slew limit (setSlewRate) to cap how fast the output changes.
 *   - Following-error limit: under load the motion plan waits for the
 *     motor instead of running ahead, so there's no catch-up surge when
 *     the load is released.
 *   - The friction offset (minPWM) is now a fixed, smooth compensation.
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

  // How many past encoder readings we keep for speed measurement (max).
  static const uint8_t MAX_SPEED_WINDOW = 16;

  // --------------------------------------------------------------------------
  //  Constructor
  // --------------------------------------------------------------------------
  //  drvPinA, drvPinB : the two L9110H input pins (both must be PWM-capable!
  //                     Arduino Micro PWM pins: 3, 5, 6, 9, 10, 11, 13.
  //                     Pin 4 is NOT PWM on the Micro: analogWrite on it
  //                     just switches fully off below 128, fully on above.)
  //  encPinA, encPinB : the two encoder signal pins (use INTERRUPT pins for
  //                     reliable counting; on an Arduino Micro / 32U4 those
  //                     are 0, 1, 2, 3 and 7)
  //  encoderPPR       : encoder pulses per ONE revolution of the MOTOR shaft
  //                     (before the gearbox), per channel. Check the motor's
  //                     product page. (ServoCity N20 "3 cycles" -> 3.
  //                     Pololu "12 CPR" -> 3, because 12 already counts all
  //                     4 edges.)
  //  gearRatio        : gearbox reduction, e.g. 29.977 for ServoCity's 30:1
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
    resetSpeedHistory();            // fill the speed window with "not moving"
  }

  // ==========================================================================
  //  CONFIGURATION  (all optional - defaults suit a ServoCity 30:1 N20
  //                  running straight from a 2S LiPo, ~560 RPM top speed)
  // ==========================================================================

  // ---- Speed mode gains (used by setRPM) -----------------------------------
  //   PWM = kff * wantedRPM            (feed-forward: "about this much PWM
  //                                     gives about this much speed")
  //       + kp  * speedError           (proportional correction)
  //       + ki  * integral(speedError) (slowly removes leftover error)
  // kff is roughly 255 / (top speed in RPM). Tune kff first (kp = ki = 0),
  // then add kp, then ki.
  void setSpeedGains(float kff, float kp, float ki) {
    _kff = kff; _kp = kp; _ki = ki;
  }

  // ---- Position mode gains (used by moveTo / moveBy) -----------------------
  //   kp : PWM per DEGREE of position error. The "spring". Higher = stiffer,
  //        too high = oscillation.
  //   ki : PWM per (degree * second). Slowly adds the extra push a load or
  //        friction needs (and slowly removes it again). Too high = slow
  //        wobble around the target, too low = sags under load.
  //   kd : PWM per RPM of speed error vs the motion profile. The "damper".
  //        Raise it if the motor overshoots, lower it if it gets jittery.
  // Position mode also uses kff (from setSpeedGains) for the feed-forward.
  void setPositionGains(float kp, float ki, float kd) {
    _kpPos = kp; _kiPos = ki; _kdPos = kd;
  }

  // Version-1 compatibility: sets only the position kp (now PWM per degree).
  void setPositionGain(float kp) { _kpPos = kp; }

  // ---- Deadband ------------------------------------------------------------
  // Fixed friction compensation, added on top of any drive request so it is
  // never wasted below the point where the motor moves. Use the "keeps
  // turning" value, NOT the "breaks loose from standstill" value: start the
  // motor with setPWM(x), then lower x until it JUST stops - use that.
  // Anything extra needed to break loose or carry a load is the integral's
  // job. Too high = overshoot and surging, too low = sluggish small moves.
  void setMinPWM(int minPWM) { setMinPWM(minPWM, minPWM); }
  // Separate values for forward and reverse (gearboxes are often stiffer one way).
  void setMinPWM(int forward, int reverse) {
    _minFwd = constrain(forward, 0, 255);
    _minRev = constrain(reverse, 0, 255);
  }

  // Upper PWM limit. Use it to keep the AVERAGE motor voltage at or below its
  // rating, e.g. 6V motor on an 8.4V battery -> 255 * 6 / 8.4 = 182.
  void setMaxPWM(int maxPWM) { _maxPWM = constrain(maxPWM, 0, 255); }

  // ---- Motion ----------------------------------------------------------------
  // Acceleration limit in RPM per second (applies to speed changes AND to
  // the motion profile of position moves). 0 = no limit (instant changes).
  void setAcceleration(float rpmPerSecond) { _accelRPMps = fabs(rpmPerSecond); }

  // How close (in degrees) the shaft must get to the target for a position
  // move to count as finished. Inside this zone the motor stops pushing.
  // Don't set it below ~2 encoder counts (360/countsPerRev * 2 degrees).
  void setPositionTolerance(float degrees) { _tolDeg = fabs(degrees); }

  // If true (default), the motor keeps holding its position after a move:
  // it brakes inside the tolerance zone and corrects if pushed out of it.
  // If false, it coasts once the move is done (shaft can be pushed away).
  void setHoldPosition(bool hold) { _hold = hold; }

  // While holding inside the tolerance zone: true = brake (terminals shorted,
  // resists being turned, default), false = coast (free).
  void setBrakeWhenHolding(bool brake) { _brakeHold = brake; }

  // ---- Timing / measurement ------------------------------------------------
  // How often the control math runs, in milliseconds (default 5).
  // Faster = less lag = less overshoot on a fast motor.
  void setUpdateInterval(uint16_t ms) { _intervalUs = (uint32_t)ms * 1000UL; }

  // How many control steps the speed is measured over (default 5, so
  // 5 x 5 ms = 25 ms). More = smoother but laggier speed reading.
  void setSpeedWindow(uint8_t samples) {
    _window = constrain(samples, 1, MAX_SPEED_WINDOW - 1);
    resetSpeedHistory();
  }

  // Extra smoothing of the measured speed, 0..1. Lower = smoother but
  // laggier. 1.0 = no extra filtering (default 0.5).
  void setRPMFilter(float alpha) { _rpmAlpha = constrain(alpha, 0.01f, 1.0f); }

  // ---- Direction -----------------------------------------------------------
  // Flip directions without rewiring.
  //   motorReversed   : swaps which driver pin means "forward"
  //   encoderReversed : flips the sign of the encoder count
  // If the motor runs away at full speed under closed-loop control, the
  // motor and encoder directions disagree: flip ONE of these.
  void setReversed(bool motorReversed, bool encoderReversed) {
    _motorRev = motorReversed;
    _encRev   = encoderReversed;
    resetSpeedHistory();
  }

  // ---- Smoothness ------------------------------------------------------------
  // Most extra push the integral term alone may add, as a fraction of
  // maxPWM (0..1, default 0.4). Limits how hard it fights a load and how
  // much "stored" push can be released when the load suddenly disappears.
  void setIntegralLimit(float fraction) { _integLimit = constrain(fraction, 0.0f, 1.0f); }

  // How fast the motor output may change, in PWM per second (default 0 =
  // off). Example: 5000 -> 0 to full power in about 0.05 s. Caps the
  // harshest output steps, but it delays the controller's reactions too,
  // and too low a value makes overshoot WORSE. Only use it if you need
  // gentler starts (e.g. wheels slipping), and keep it high.
  void setSlewRate(float pwmPerSecond) { _slewPerSec = fabs(pwmPerSecond); }

  // Following-error limit, in degrees (default 30). During a move, the
  // planned position may never get further ahead of the real shaft than
  // this. Under load the plan waits for the motor instead of running away,
  // so there is no flat-out catch-up surge when the load disappears.
  // Must be bigger than the normal lag during a move (watch angle vs
  // profile in the plotter). 0 = no limit.
  void setMaxLag(float degrees) { _maxLagDeg = fabs(degrees); }

  // ---- Safety ----------------------------------------------------------------
  // Stall protection: if the output is above 80% of maxPWM but the shaft is
  // turning slower than 3 RPM for longer than 'ms', the motor is switched off
  // (coast) and isStalled() returns true until the next command.
  // 0 = disabled (default). 500 ms is a sensible value.
  void setStallProtection(uint16_t ms) { _stallMs = ms; }

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
    _stalled   = false;
  }

  // Move to an ABSOLUTE angle (degrees of the output shaft, relative to
  // where the shaft was at power-up or the last resetPosition()).
  // maxRPM is the cruising speed of the move (sign is ignored).
  void moveTo(float degrees, float maxRPM) {
    _goalCounts = degreesToCounts(degrees);
    _moveRPM    = fabs(maxRPM);

    if (_mode != MODE_POSITION || _moveDone) {
      // Starting a fresh move: the motion profile's "virtual shaft" starts
      // exactly where the real shaft is now, moving at the real speed.
      _spCounts = (float)getCount();
      _profRPM  = _rpm;
    }
    // If we were in the MIDDLE of a move, the profile just continues
    // smoothly from wherever it currently is toward the new goal.

    _integ     = 0;      // fresh approach, fresh integral
    _lastErr   = 0;
    _mode      = MODE_POSITION;
    _moveDone  = false;
    _inZone    = false;
    _stalled   = false;
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
    _mode    = MODE_OPEN_LOOP;
    _stalled = false;
    _lastOut = pwm;   // so a later closed-loop command slews from here
    writeRaw(pwm);
  }

  // Stop controlling and let the motor free-wheel.
  void stop() {
    _mode  = MODE_IDLE;
    _integ = 0;
    coastOutputs();
  }

  // Stop controlling and short the motor terminals (stops much faster
  // than coasting).
  void brake() {
    _mode  = MODE_IDLE;
    _integ = 0;
    brakeOutputs();
  }

  // Redefine the current shaft position as 'degrees' (default 0).
  // Example: after hitting a limit switch, call resetPosition(0) to "home".
  void resetPosition(float degrees = 0) {
    long c = (long)degreesToCounts(degrees);
    _enc.write(_encRev ? -c : c);   // write raw value, honouring reversal
    resetSpeedHistory();            // avoid a fake speed spike next update
    _spCounts   = c;
    _goalCounts = c;
  }

  // ==========================================================================
  //  STATUS
  // ==========================================================================

  float getRPM()          const { return _rpm; }                 // measured, filtered
  float getAngle()              { return countsToDegrees(getCount()); }
  float getTargetAngle()  const { return countsToDegrees(_goalCounts); }
  float getProfileAngle() const { return countsToDegrees(_spCounts); } // moving setpoint
  float getTargetRPM()    const { return _targetRPM; }
  int   getPWM()          const { return _pwm; }                 // last PWM sent (signed)
  Mode  getMode()         const { return _mode; }
  float getCountsPerRev() const { return _countsPerRev; }
  bool  isStalled()       const { return _stalled; }

  // Raw encoder count (with encoder reversal applied).
  long getCount() {
    long c = _enc.read();
    return _encRev ? -c : c;
  }

  // True once a position move has reached its target (within tolerance).
  // Stays true while holding, even if small corrections happen afterwards.
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

    // ---- 1. Measure speed (always, so getRPM() works in every mode) -------
    measureSpeed(now);

    // ---- 2. Run the controller for the current mode ------------------------
    switch (_mode) {
      case MODE_SPEED:    runSpeedMode(dt);    break;
      case MODE_POSITION: runPositionMode(dt); break;
      default:            return;              // IDLE / OPEN_LOOP: nothing to do
    }

    // ---- 3. Stall protection ------------------------------------------------
    checkStall(elapsed);
  }

private:

  // ---------------- hardware ----------------
  uint8_t _drvA, _drvB;     // L9110H input pins
  Encoder _enc;             // Paul Stoffregen's interrupt-driven encoder reader

  // ---------------- geometry ----------------
  float _countsPerRev;      // encoder counts per OUTPUT shaft revolution

  // ---------------- tuning (defaults: ServoCity 30:1 on 2S LiPo) ----------
  float    _kff   = 0.45f;  // PWM per RPM (feed-forward), ~255 / top RPM
  float    _kp    = 0.30f;  // speed mode: PWM per RPM of error
  float    _ki    = 1.50f;  // speed mode: PWM per (RPM * second)
  float    _kpPos = 3.50f;  // position: PWM per degree of error
  float    _kiPos = 2.00f;  // position: PWM per (degree * second), final approach only
  float    _kdPos = 0.45f;  // position: PWM per RPM of speed error
  int      _minFwd = 0;     // deadband compensation, forward
  int      _minRev = 0;     // deadband compensation, reverse
  int      _maxPWM = 255;
  float    _accelRPMps = 1000.0f;  // RPM per second, 0 = unlimited
  float    _tolDeg     = 2.0f;     // "arrived" tolerance in degrees
  float    _arriveRPM  = 15.0f;    // must also be slower than this to count as arrived
  bool     _hold       = true;     // keep holding after a move
  bool     _brakeHold  = true;     // brake (true) or coast (false) inside the zone
  uint32_t _intervalUs = 5000UL;   // 5 ms control period
  uint8_t  _window     = 5;        // speed measured over 5 steps = 25 ms
  float    _rpmAlpha   = 0.5f;     // extra speed smoothing
  bool     _motorRev = false;
  bool     _encRev   = false;
  uint16_t _stallMs  = 0;          // 0 = stall protection off
  float    _integLimit = 0.4f;     // integral may supply at most 40% of maxPWM
  float    _slewPerSec = 0.0f;     // max output change, PWM per second (0 = off)
  float    _maxLagDeg  = 30.0f;    // virtual shaft may lead the real one by at most this

  // ---------------- state ----------------
  Mode     _mode = MODE_IDLE;
  uint32_t _lastMicros = 0;
  float    _rpm        = 0;   // measured (filtered) speed
  float    _integ      = 0;   // integral accumulator (speed OR position mode)
  float    _lastErr    = 0;   // previous position error (zero-crossing check)
  float    _lastOut    = 0;   // previous controller output (slew limit)
  int      _pwm        = 0;   // last signed PWM actually written

  // speed measurement ring buffer
  long     _histCount[MAX_SPEED_WINDOW];
  uint32_t _histTime[MAX_SPEED_WINDOW];
  uint8_t  _histIdx = 0;

  float _targetRPM = 0;       // speed mode: where we're heading
  float _cmdRPM    = 0;       // speed mode: ramped command

  float _goalCounts = 0;      // position mode: final target
  float _spCounts   = 0;      // position mode: moving "virtual shaft" setpoint
  float _profRPM    = 0;      // position mode: speed of the virtual shaft
  float _moveRPM    = 0;      // position mode: cruise speed of the move
  bool  _moveDone   = true;   // reached target at least once
  bool  _inZone     = false;  // currently resting inside the tolerance zone

  bool     _stalled    = false;
  uint32_t _stallTimer = 0;   // microseconds spent "pushing but not moving"

  // ==========================================================================
  //  Unit conversions
  // ==========================================================================
  float degreesToCounts(float deg) const { return deg / 360.0f * _countsPerRev; }
  float countsToDegrees(float c)   const { return c * 360.0f / _countsPerRev; }
  float rpmToCountsPerSec(float r) const { return r * _countsPerRev / 60.0f; }
  float countsPerSecToRPM(float c) const { return c * 60.0f / _countsPerRev; }

  // ==========================================================================
  //  Speed measurement over a sliding window.
  // --------------------------------------------------------------------------
  //  We remember the encoder count at each of the last few control steps.
  //  Speed = (count now - count N steps ago) / (time between them).
  //  With a coarse encoder (e.g. 360 counts/rev) a single 5 ms slice holds
  //  only a handful of counts, so the reading would jump around; a 25 ms
  //  window holds 5x more counts and gives a far steadier number.
  // ==========================================================================
  void resetSpeedHistory() {
    long c = getCount();
    uint32_t t = micros();
    for (uint8_t i = 0; i < MAX_SPEED_WINDOW; i++) {
      _histCount[i] = c;
      _histTime[i]  = t;
    }
    _histIdx = 0;
    _rpm = 0;
  }

  void measureSpeed(uint32_t now) {
    long count = getCount();

    // Slot that holds the reading from '_window' steps ago.
    uint8_t oldIdx = (_histIdx + MAX_SPEED_WINDOW - _window) % MAX_SPEED_WINDOW;
    long     dCount = count - _histCount[oldIdx];
    uint32_t dTime  = now   - _histTime[oldIdx];

    // Store the newest reading for future steps.
    _histIdx = (_histIdx + 1) % MAX_SPEED_WINDOW;
    _histCount[_histIdx] = count;
    _histTime[_histIdx]  = now;

    if (dTime == 0) return;
    float rawRPM = countsPerSecToRPM(dCount / (dTime / 1000000.0f));

    // Light extra smoothing on top of the window average.
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
  //  SPEED MODE: wanted RPM -> PWM using feed-forward + PI.
  // ==========================================================================
  void runSpeedMode(float dt) {
    // Ramp the commanded speed toward the target at the accel limit.
    _cmdRPM = rampToward(_cmdRPM, _targetRPM, dt);

    // Asked to stand still and basically are: output nothing, no creeping.
    if (_cmdRPM == 0 && fabs(_rpm) < 2.0f) {
      _integ = 0;
      coastOutputs();
      return;
    }

    float err = _cmdRPM - _rpm;                   // positive = too slow
    float out = _kff * _cmdRPM + _kp * err + _ki * _integ;

    // Anti-windup ("conditional integration"): only let the integral grow
    // if the output is NOT already pinned at the limit in that direction.
    // Otherwise it "charges up" while saturated and causes big overshoot.
    bool saturatedUp   = (out >=  _maxPWM && err > 0);
    bool saturatedDown = (out <= -_maxPWM && err < 0);
    if (!saturatedUp && !saturatedDown) _integ += err * dt;
    if (_ki > 0) {                                // hard limit, same as position mode
      float maxInteg = (_maxPWM * _integLimit) / _ki;
      _integ = constrain(_integ, -maxInteg, maxInteg);
    }

    writeDrive(out, dt);
  }

  // ==========================================================================
  //  POSITION MODE: trapezoidal motion profile + direct PID on position.
  // --------------------------------------------------------------------------
  //  A "virtual shaft" (_spCounts) travels smoothly to the goal: speeds up
  //  at the accel limit, cruises at _moveRPM, and slows down in time to stop
  //  exactly on target. The real motor is pulled along behind it:
  //    PWM = kff * profileRPM                 (roughly the right power)
  //        + kp  * (virtual - real) degrees   (spring)
  //        + kd  * (profileRPM - realRPM)     (damper)
  //        + ki  * integral                   (slowly matches load/friction)
  //  Once the real shaft is inside the tolerance zone at the end, the motor
  //  output is switched off (brake) until something pushes it out again.
  // ==========================================================================
  void runPositionMode(float dt) {
    updateProfile(dt);

    long  count      = getCount();

    // ---- Following-error limit ------------------------------------------------
    // Under load the real shaft falls behind the virtual one. Without a limit
    // the gap keeps growing, and the moment the load lets go the motor races
    // flat-out to catch up (a surge). So the virtual shaft is never allowed
    // to get more than _maxLagDeg ahead of the real one: under load, the plan
    // simply waits for the motor. When the load goes away there is no big
    // gap to make up, so the motor just carries on at the planned speed.
    if (_maxLagDeg > 0) {
      float maxLag = degreesToCounts(_maxLagDeg);
      float lag    = _spCounts - (float)count;
      if (lag >  maxLag) _spCounts = (float)count + maxLag;
      if (lag < -maxLag) _spCounts = (float)count - maxLag;
    }
    float errDeg     = countsToDegrees(_spCounts - (float)count);
    bool  profileEnd = (_spCounts == _goalCounts);

    // ---- Arrival dead-zone with hysteresis -------------------------------
    if (profileEnd) {
      float absErr = fabs(errDeg);
      // "Arrived" = inside the tolerance AND nearly stopped. Without the
      // speed check, a shaft flying THROUGH the zone would count as done.
      if (!_inZone && absErr <= _tolDeg && fabs(_rpm) < _arriveRPM) {
        _inZone   = true;          // just arrived
        _moveDone = true;
        _integ    = 0;
      } else if (_inZone && absErr > _tolDeg * 2.0f) {
        _inZone   = false;         // got pushed clearly out: correct again
        _integ    = 0;
      }

      if (_inZone) {
        if (!_hold) {              // finished and told not to hold
          _mode = MODE_IDLE;
          coastOutputs();
        } else if (_brakeHold) {
          brakeOutputs();
        } else {
          coastOutputs();
        }
        return;                    // nothing else to do while resting
      }
    }

    // ---- PID output ------------------------------------------------------------
    // No special cases, no speed thresholds: the same smooth equation runs
    // the whole time, so the push can never suddenly jump. Load (a finger on
    // the wheel, a ramp, a stiff gearbox) shows up as growing position error,
    // and the controller answers it smoothly:
    //   - the spring (kp) pushes harder the further the shaft falls behind
    //   - the integral (ki) slowly adds whatever constant extra push the load
    //     needs, and slowly gives it back when the load goes away
    //   - the damper (kd) resists sudden speed changes in either direction
    float pTerm  = _kpPos * errDeg;
    float dTerm  = _kdPos * (_profRPM - _rpm);
    float ffTerm = _kff   * _profRPM;
    float out    = ffTerm + pTerm + dTerm + _kiPos * _integ;

    // ---- Integral with anti-windup --------------------------------------------
    // 1. Conditional integration: do NOT add to the integral while the output
    //    is already pinned at max in the same direction. That's where windup
    //    comes from: the motor can't go faster anyway, the integral would just
    //    "charge up" and then release as a surge once the load disappears.
    // 2. Hard limit: the integral alone may never supply more than
    //    _integLimit (fraction of maxPWM, default 40%).
    // 3. Zero-crossing reset at the end of a move: if the shaft crosses the
    //    target, the old integral was pushing the wrong way now -> drop it.
    // 4. Only once the motion profile has finished. While the profile is
    //    still moving, the shaft always trails it a little (inertia); an
    //    integral collecting that normal lag would release it as a big
    //    overshoot at the end. During the move, load is handled by the
    //    spring (kp): the further behind, the harder it pushes. This is
    //    decided by the PLAN (profile finished), not by measured speed, so a
    //    load can't flip it on and off - and it starts from zero, so it
    //    grows in smoothly.
    if (_kiPos > 0 && profileEnd) {
      bool satUp   = (out >=  _maxPWM && errDeg > 0);
      bool satDown = (out <= -_maxPWM && errDeg < 0);
      if (!satUp && !satDown) _integ += errDeg * dt;

      if (profileEnd && (errDeg * _lastErr) < 0) _integ = 0;   // crossed target

      float maxInteg = (_maxPWM * _integLimit) / _kiPos;
      _integ = constrain(_integ, -maxInteg, maxInteg);
    }
    _lastErr = errDeg;

    writeDrive(out, dt);
  }

  // Advance the virtual shaft one step along the trapezoidal profile.
  void updateProfile(float dt) {
    float remaining = _goalCounts - _spCounts;    // distance left for the virtual shaft
    if (remaining == 0) { _profRPM = 0; return; }
    float dir = (remaining > 0) ? 1.0f : -1.0f;

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
    if (fabs(step) >= fabs(remaining) || (step * dir) < 0) {
      _spCounts = _goalCounts;
      _profRPM  = 0;
    } else {
      _spCounts += step;
    }
  }

  // ==========================================================================
  //  Stall protection: pushing hard but not moving for too long -> cut power.
  // ==========================================================================
  void checkStall(uint32_t elapsedUs) {
    if (_stallMs == 0) return;
    bool pushingHard = abs(_pwm) >= (int)(_maxPWM * 0.8f);
    bool notMoving   = fabs(_rpm) < 3.0f;
    if (pushingHard && notMoving) {
      _stallTimer += elapsedUs;
      if (_stallTimer >= (uint32_t)_stallMs * 1000UL) {
        _stalled = true;
        stop();
      }
    } else {
      _stallTimer = 0;
    }
  }

  // ==========================================================================
  //  Send a controller output (signed, roughly -255..255) to the driver.
  //
  //  Step 1 - optional slew limit: the output may only change by
  //           _slewPerSec PWM per second (off by default, see setSlewRate).
  //  Step 2 - deadband offset: a FIXED friction compensation, added smoothly.
  //             output 0   -> coast
  //             output +x  -> minFwd + x  (capped at maxPWM)
  //             output -x  -> minRev + x  (capped at maxPWM)
  //           The offset fades in over the first DEADBAND_RAMP units of
  //           output, so the drive changes smoothly through zero. It never
  //           depends on speed or load - extra push for load is the
  //           integral's job, not a hard-coded boost.
  // ==========================================================================
  static constexpr float DEADBAND_RAMP = 8.0f;

  void writeDrive(float out, float dt) {
    // Step 1: slew limit
    if (_slewPerSec > 0) {
      float maxStep = _slewPerSec * dt;
      out = _lastOut + constrain(out - _lastOut, -maxStep, maxStep);
    }
    out = constrain(out, (float)-_maxPWM, (float)_maxPWM);
    _lastOut = out;

    // Step 2: deadband offset
    float a = fabs(out);
    if (a < 0.5f) { coastOutputs(); return; }           // effectively zero
    float offset = (out > 0 ? _minFwd : _minRev) * min(1.0f, a / DEADBAND_RAMP);
    int mag = (int)(offset + a + 0.5f);
    mag = min(mag, _maxPWM);
    writeRaw(out > 0 ? mag : -mag);
  }

  // ==========================================================================
  //  Send a signed PWM (-255..255) straight to the L9110H. No deadband math.
  // ==========================================================================
  void writeRaw(int pwm) {
    pwm = constrain(pwm, -_maxPWM, _maxPWM);
    int mag = abs(pwm);

    bool forward = (pwm >= 0);
    if (_motorRev) forward = !forward;            // honour motor reversal

    // IMPORTANT: always set the "off" pin to 0 BEFORE driving the other,
    // so both inputs are never PWM'd at the same time.
    if (mag == 0) {
      coastOutputs();
      return;
    } else if (forward) {
      analogWrite(_drvB, 0);
      analogWrite(_drvA, mag);
    } else {
      analogWrite(_drvA, 0);
      analogWrite(_drvB, mag);
    }
    _pwm = pwm;                                   // remember what we sent
  }

  // Both inputs low = motor free-wheels.
  void coastOutputs() {
    analogWrite(_drvA, 0);
    analogWrite(_drvB, 0);
    _pwm = 0;
    _lastOut = 0;     // next drive request slews up from zero
  }

  // Both inputs high = motor terminals shorted = brake.
  void brakeOutputs() {
    analogWrite(_drvA, 255);
    analogWrite(_drvB, 255);
    _pwm = 0;
    _lastOut = 0;     // next drive request slews up from zero
  }
};

#endif // ENCODER_MOTOR_H
