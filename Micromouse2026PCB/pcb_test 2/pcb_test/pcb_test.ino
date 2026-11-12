/*
 * ============================================================================
 *  pcb_test.ino - full board self-test for the Micromouse2026PCB
 * ============================================================================
 *  Tests every system on an assembled board and shows the result on the
 *  OLED. Everything is driven with the rotary encoder:
 *
 *      turn        move the cursor / change a value
 *      click       select / edit
 *      hold        back to the menu (on the motor page: STOP + back)
 *
 *  Pages:
 *      Status      I2C ACK + start-up result for OLED, IMU and all 5 ToF with
 *                  their live distance, VIN,
 *                  seconds since boot and the lowest free RAM seen
 *      ToF map     top-view picture: each sensor's beam and where it ends,
 *                  with the five distances in mm underneath
 *      IMU         BNO085 yaw / pitch / roll, report rate, calibration status
 *      Encoders    raw counts, revolutions and RPM for both motors
 *      Motor test  drive either / both motors in PWM or RPM mode.
 *                  Try PWM first: RPM mode is closed-loop, so a motor whose
 *                  encoder shows "REV" in PWM mode would run away in RPM mode.
 *      Re-init     run the sensor start-up again (after re-seating a part)
 *
 *  All pin numbers below were read out of Micromouse2026PCB.kicad_pcb.
 *
 *  Libraries (Arduino IDE -> Library Manager):
 *      "VL53L0X"                       by Pololu
 *      "SparkFun BNO080 Cortex Based IMU"  by SparkFun  (drives the BNO085;
 *                                      far smaller than the Adafruit one)
 *      "SSD1306Ascii"                  by Bill Greiman  (text only, no
 *                                      1 KB frame buffer)
 *      "Encoder"                       by Paul Stoffregen
 *  plus EncoderMotor.h, which sits next to this file.
 *
 *  Board: Arduino Micro (arduino:avr:micro)
 *
 *  FLASH IS NEARLY FULL: about 300 bytes are left of the Micro's 28,672
 *  with the library versions current in Nov 2026. If the IDE says "Sketch
 *  too big", set TOF_PERIOD_MS to 0 (frees ~190 bytes). Anything new means
 *  something else has to go.
 * ============================================================================
 */

#include <Wire.h>
#include <VL53L0X.h>
#include <SparkFun_BNO080_Arduino_Library.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>

// This sketch only uses the PWM and RPM modes of EncoderMotor. Leaving the
// position mode (moveTo / moveBy) out of the build saves 2.2 KB of flash,
// and without that the sketch does not fit on a Micro. The EncoderMotor.h
// in this folder is the same file as "Encoder Motor/EncoderMotor_SimpleDemo"
// plus the three #ifndef lines that make this switch work.
#define ENCODERMOTOR_NO_POSITION_MODE
#include "EncoderMotor.h"          // also pulls in <Encoder.h>

// ============================================================================
//  BOARD WIRING  (from the PCB netlist - only change if the PCB changes)
// ============================================================================
//  I2C: SDA = D2, SCL = D3 (fixed on the Micro)

// VL53L0X XSHUT lines, U1..U4. U5 has NO XSHUT trace on this board, so it
// is always awake; it gets moved off the default address first (see
// initSensors()).
const uint8_t PIN_XSHUT[4] = { A0, A1, A2, A3 };

// Motor M1 (L9110H "MD1") and its encoder
const uint8_t PIN_M1_IA = 5,  PIN_M1_IB = 6;
const uint8_t PIN_M1_ENC_A = 1,  PIN_M1_ENC_B = 12;   // D1 = interrupt pin
// Motor M2 (L9110H "MD2") and its encoder
const uint8_t PIN_M2_IA = 9,  PIN_M2_IB = 10;
const uint8_t PIN_M2_ENC_A = 0,  PIN_M2_ENC_B = 11;   // D0 = interrupt pin

// Rotary encoder RS1 (common + one switch leg go to GND -> internal pull-ups)
const uint8_t PIN_KNOB_A = 7, PIN_KNOB_B = 8, PIN_KNOB_SW = 4;

// VIN sense: 10k / 10k divider (R1, R2) from the switched battery rail
const uint8_t PIN_VIN_SENSE = A5;

// ============================================================================
//  SETTINGS YOU MAY NEED TO CHANGE
// ============================================================================

// Encoder pulses per MOTOR revolution (per channel) and gearbox ratio.
// These only affect the "rev" and "rpm" numbers and RPM mode - the raw
// counts on the Encoders page are always exact. To measure the real value:
// zero the counts (click on the Encoders page), turn a wheel exactly one
// turn by hand, and read "cnt". Then PPR * 4 * ratio must equal that number.
const float ENCODER_PPR = 3.0;
const float GEAR_RATIO  = 29.977;

// Flip these if a motor / encoder runs backwards. On the motor page,
// "REV" next to a motor means its encoder counts down while it is driven
// forwards: flip that motor's ENC_REVERSED (or MOTOR_REVERSED, if the wheel
// itself turns the wrong way).
const bool M1_MOTOR_REVERSED = false, M1_ENC_REVERSED = false;
const bool M2_MOTOR_REVERSED = false, M2_ENC_REVERSED = false;

// Motor test limits and step per knob click
const int PWM_STEP = 5,  PWM_LIMIT = 255;
const int RPM_STEP = 10, RPM_LIMIT = 500;

// The motor page refuses to drive the motors while VIN is below this
// (in 0.1 V). The drivers are fed from the battery rail; with only USB
// plugged in, that rail is back-fed through the Micro's 5 V regulator, and
// a motor pulling current that way drags the 5 V rail down far enough to
// reset the board. Set to 0 to switch the check off (e.g. if the VIN
// divider R1/R2 is not fitted).
const uint8_t MOTOR_MIN_VIN = 55;

// Rotary encoder: counts per detent (EC11 = 4). Set negative to reverse
// the turn direction.
const int8_t KNOB_COUNTS_PER_DETENT = 4;

// I2C addresses
const uint8_t ADDR_OLED = 0x3C;
const uint8_t ADDR_IMU  = 0x4A;          // Adafruit BNO085 default (SparkFun's board is 0x4B)
const uint8_t ADDR_TOF[5] = { 0x2A, 0x2B, 0x2C, 0x2D, 0x2E };   // U1..U5 after start-up

// How often each ToF takes a reading, in ms. Each reading takes about 33 ms
// with the laser on, so 100 keeps the laser off two thirds of the time
// (less current, less heat). 0 = range non-stop.
const uint16_t TOF_PERIOD_MS = 100;

// I2C speed. 400 kHz keeps the screen and the motor loop fast. If sensors
// drop out or the screen glitches, try 100000.
const uint32_t I2C_HZ = 400000;

// ToF map page: millimetres per pixel. 4 shows about 200 mm ahead of the
// mouse and 250 mm to each side.
const uint8_t MAP_MM_PER_PX = 4;

// Each ToF on the map: where it sits on the board and which way it looks,
// seen from above with the mouse facing up the screen. Position is in mm
// (right of the centre line, ahead of the two side sensors), taken from the
// footprint centres in the PCB file. Direction is one screen step: right
// (+1) / left (-1), then up (-1) / level (0).
//                                  right  ahead   dx  dy
const int8_t TOF_GEOM[5][4] PROGMEM = {
  /* U1 left        */            { -27,     0,   -1,  0 },
  /* U2 front-left  */            { -16,    17,   -1, -1 },
  /* U3 front       */            {   0,    22,    0, -1 },
  /* U4 front-right */            {  16,    17,    1, -1 },
  /* U5 right       */            {  27,     0,    1,  0 },
};

// Set to 1 to also print one line of numbers per second on USB serial
// (115200 baud; format is described at serialReport()). Off by default
// because the sketch nearly fills the Micro: with it on there are only a
// few dozen bytes of flash left.
#define SERIAL_REPORT 0

// ============================================================================
//  Objects
// ============================================================================
SSD1306AsciiWire oled;
VL53L0X  tof[5];
BNO080   imu;
// Both motors are built by one shared (not inlined) function: the
// EncoderMotor constructor is big, and this keeps a single copy of it in
// flash instead of two (saves about 0.5 KB).
__attribute__((noinline))
EncoderMotor makeMotor(uint8_t ia, uint8_t ib, uint8_t encA, uint8_t encB) {
  return EncoderMotor(ia, ib, encA, encB, ENCODER_PPR, GEAR_RATIO);
}
EncoderMotor m1 = makeMotor(PIN_M1_IA, PIN_M1_IB, PIN_M1_ENC_A, PIN_M1_ENC_B);
EncoderMotor m2 = makeMotor(PIN_M2_IA, PIN_M2_IB, PIN_M2_ENC_A, PIN_M2_ENC_B);
Encoder  knob(PIN_KNOB_A, PIN_KNOB_B);

// ============================================================================
//  State
// ============================================================================
// Device slots: 0..4 = ToF U1..U5, 5 = IMU, 6 = OLED
enum { DEV_IMU = 5, DEV_OLED = 6, DEV_COUNT = 7 };
bool devAck[DEV_COUNT];      // answers on the I2C bus right now
bool devInit[DEV_COUNT];     // start-up succeeded

uint16_t tofMm[5];           // last distance, mm
int16_t  imuYpr[3];          // yaw, pitch, roll in 0.1 degree
uint8_t  imuQuality;         // 0..3, the BNO085's own calibration/accuracy status
uint8_t  imuReports, imuHz;  // reports counted this second / last second
uint8_t  vinTenths;          // VIN in 0.1 V
bool     busError;           // an I2C transfer timed out (stuck SDA/SCL)
uint8_t  mapBand[128];       // one 8-pixel-high strip of the ToF map being drawn
uint16_t upSeconds;          // time since boot
uint16_t ramFreeMin;         // fewest bytes ever free between globals and stack

enum Page : uint8_t { PG_MENU, PG_STATUS, PG_MAP, PG_IMU, PG_ENC, PG_MOTOR, PG_REINIT };
const uint8_t MENU_ITEMS = 6;         // every page except the menu itself
uint8_t page = PG_MENU;
uint8_t menuSel = 0;
uint8_t drawRowNext = 0;

// Motor test page
uint8_t mSel = 0;            // highlighted line: 0 motor, 1 mode, 2 set
bool    mEdit = false;       // knob changes the highlighted value
uint8_t mWhich = 2;          // 0 = M1, 1 = M2, 2 = both
bool    mRpmMode = false;    // false = raw PWM, true = closed-loop RPM
int     mSet = 0;            // commanded PWM or RPM

// ============================================================================
//  Small print helpers (no printf / float printing: both cost a lot of flash)
// ============================================================================
// All screen text goes through these two. Keeping them out-of-line makes
// every call site a little smaller, which adds up over ~70 calls.
__attribute__((noinline)) void txt(const __FlashStringHelper *t) { oled.print(t); }
__attribute__((noinline)) void chr(char c) { oled.write(c); }

void spaces(int8_t n) { while (n-- > 0) chr(' '); }

// Right-aligned integer with 'decimals' digits after the point.
// fix(-1234, 1, 7) prints " -123.4".
void fix(long v, uint8_t decimals, uint8_t width) {
  char buf[12];
  bool neg = v < 0;
  if (neg) v = -v;
  uint8_t i = 0;
  do {
    buf[i++] = '0' + v % 10;
    v /= 10;
    if (i == decimals) buf[i++] = '.';
  } while (v > 0 || i <= decimals);          // always at least "0.x"
  if (decimals && buf[i - 1] == '.') buf[i++] = '0';
  if (neg) buf[i++] = '-';
  spaces(width - i);
  while (i) chr(buf[--i]);
}

void hexDigit(uint8_t v) { chr(v < 10 ? '0' + v : 'A' - 10 + v); }
void hex2(uint8_t v) { hexDigit(v >> 4); hexDigit(v & 15); }

// ============================================================================
//  Free-RAM check. At boot the unused RAM between the global variables and
//  the stack is filled with a marker byte; whatever the stack has since
//  overwritten is no longer marked. The count of surviving marker bytes is
//  the smallest amount of RAM that has ever been free. If this number is
//  comfortably above zero, the sketch is not running out of RAM.
// ============================================================================
#ifdef __AVR__
extern char __heap_start;
void ramPaint() {
  for (char *p = &__heap_start; p < (char *)SP - 16; p++) *p = 0x5A;
}
uint16_t ramUnused() {
  uint16_t n = 0;
  for (const char *p = &__heap_start; *p == 0x5A; p++) n++;
  return n;
}
#else
void ramPaint() {}
uint16_t ramUnused() { return 0; }
#endif

// ============================================================================
//  I2C helpers
// ============================================================================
bool ping(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

uint8_t devAddr(uint8_t dev) {
  if (dev < 5) return ADDR_TOF[dev];
  return dev == DEV_IMU ? ADDR_IMU : ADDR_OLED;
}

void pingAll() {
  for (uint8_t d = 0; d < DEV_COUNT; d++) devAck[d] = ping(devAddr(d));
  if (Wire.getWireTimeoutFlag()) {
    busError = true;
    Wire.clearWireTimeoutFlag();
  }
}

// ============================================================================
//  Sensor start-up (also used by "Re-init sensors")
// ============================================================================
void imuEnableReports() {
  // 50 Hz. "Game" rotation vector = gyro + accelerometer only; the plain one
  // also uses the magnetometer, and the motor magnets would drag it around.
  imu.enableGameRotationVector(20);
}

void initSensors() {
  // ---- ToF ------------------------------------------------------------------
  // Every VL53L0X powers up on 0x29. Hold U1..U4 in reset with XSHUT so that
  // only U5 (no XSHUT trace) is listening, and move it to its own address.
  // Then wake the others one at a time and give each its address.
  for (uint8_t i = 0; i < 4; i++) {
    pinMode(PIN_XSHUT[i], OUTPUT);
    digitalWrite(PIN_XSHUT[i], LOW);
  }
  delay(10);
  for (int8_t i = 4; i >= 0; i--) {             // U5 first, then U4..U1
    if (i < 4) {
      pinMode(PIN_XSHUT[i], INPUT);             // release: the breakout pulls it up to 2.8 V
      delay(10);                                // (never drive it HIGH - not 5 V tolerant)
    }
    tof[i] = VL53L0X();                         // forget any earlier address
    tof[i].setTimeout(100);
    tof[i].setAddress(ADDR_TOF[i]);
    devAck[i]  = ping(ADDR_TOF[i]);
    devInit[i] = devAck[i] && tof[i].init();
    if (devInit[i]) tof[i].startContinuous(TOF_PERIOD_MS);
    tofMm[i] = 0;
  }

  // ---- IMU ------------------------------------------------------------------
  devAck[DEV_IMU]  = ping(ADDR_IMU);
  devInit[DEV_IMU] = devAck[DEV_IMU] && imu.begin(ADDR_IMU, Wire);
  if (devInit[DEV_IMU]) imuEnableReports();
}

// ============================================================================
//  Background jobs - run every pass of loop(), on every page
// ============================================================================
void pollTof() {
  for (uint8_t i = 0; i < 5; i++) {
    if (!devInit[i] || !devAck[i]) continue;
    // Only fetch a reading that is already waiting, so this never blocks.
    if (tof[i].readReg(VL53L0X::RESULT_INTERRUPT_STATUS) & 0x07) {
      tofMm[i] = tof[i].readRangeContinuousMillimeters();
    }
  }
}

// ---- IMU -------------------------------------------------------------------
// The SparkFun getYaw() / getAccelX() / ... helpers push everything through
// pow() and floating point, which costs about 4 KB of flash - more than this
// sketch has left on a Micro. The library leaves the packet it just received
// in its public shtpData[] buffer, so the fixed-point numbers are taken
// straight from there (same byte offsets as the library's own
// parseInputReport()) and turned into angles with integer maths.

int16_t imuWord(uint8_t n) {           // n-th 16-bit value of the current report
  return (int16_t)(imu.shtpData[9 + 2 * n] | (uint16_t)imu.shtpData[10 + 2 * n] << 8);
}

inline uint32_t mul16(uint16_t a, uint16_t b) { return (uint32_t)a * b; }
inline int32_t  smul16(int16_t a, int16_t b)  { return (int32_t)a * b; }

// atan2() in 0.1 degree steps, integers only. Worst-case error 0.25 degree.
int16_t iatan2(long y, long x) {
  if (x == 0 && y == 0) return 0;
  unsigned long ay = labs(y), ax = labs(x);
  bool steep = ay > ax;
  unsigned long lo = steep ? ax : ay, hi = steep ? ay : ax;
  while (hi >= (1UL << 19)) { hi >>= 1; lo >>= 1; }      // so lo << 12 fits in 32 bits
  uint16_t r = (lo << 12) / hi;                          // ratio 0..1 as 0..4096
  // atan(r) ~= r * (45 + (1 - r) * (14.02 + 3.80 * r)) degrees, in 0.01 degree.
  // (16 x 16 -> 32 bit multiplies: much smaller than full 32-bit ones on AVR)
  uint16_t u = mul16(4096 - r, 1402 + (uint16_t)(mul16(380, r) >> 12)) >> 12;
  int16_t  a = (uint16_t)((mul16(r, 4500 + u) >> 12) + 5) / 10;      // 0..450
  if (steep) a = 900 - a;
  if (x < 0) a = 1800 - a;
  return y < 0 ? -a : a;
}

uint16_t isqrt(unsigned long v) {
  unsigned long r = 0, b = 1UL << 30;
  while (b > v) b >>= 2;
  while (b) {
    if (v >= r + b) { v -= r + b; r = (r >> 1) + b; }
    else            r >>= 1;
    b >>= 2;
  }
  return r;
}

void pollImu() {
  if (!devInit[DEV_IMU] || !devAck[DEV_IMU]) return;
  // Ask at most every 10 ms: a failed read makes the library wait 100 ms.
  static uint8_t tLast = 0;
  uint8_t tNow = millis();
  if ((uint8_t)(tNow - tLast) < 10) return;
  tLast = tNow;
  uint16_t report = imu.getReadings();                   // 0 = nothing new
  if (report == SENSOR_REPORTID_GAME_ROTATION_VECTOR) {
    imuReports++;
    imuQuality = imu.shtpData[7] & 3;
    // Unit quaternion, each part scaled by 2^14, so a product of two parts
    // is scaled by 2^28 ("ONE"). Standard quaternion -> yaw / pitch / roll.
    const long ONE = 1L << 28;
    int16_t x = imuWord(0), y = imuWord(1), z = imuWord(2), w = imuWord(3);
    int16_t sinPitch = constrain((smul16(w, y) - smul16(z, x)) >> 13, -16384, 16384);   // back to 2^14 scale
    long yy2 = 2 * smul16(y, y);
    imuYpr[0] = iatan2(2 * (smul16(w, z) + smul16(x, y)), ONE - yy2 - 2 * smul16(z, z));
    imuYpr[1] = iatan2(sinPitch, isqrt(ONE - smul16(sinPitch, sinPitch)));
    imuYpr[2] = iatan2(2 * (smul16(w, x) + smul16(y, z)), ONE - yy2 - 2 * smul16(x, x));
  }
  if (imu.hasReset()) imuEnableReports();                // brown-out etc: turn reports back on
}

void readVin() {
  // 10k/10k divider -> VIN = 2 * pin voltage; ADC reference is the 5 V rail.
  vinTenths = (analogRead(PIN_VIN_SENSE) * 100UL + 511) / 1023;
}

void motorsStop() {
  m1.stop();
  m2.stop();
  mSet = 0;
}

void motorsApply() {
  if (vinTenths < MOTOR_MIN_VIN) mSet = 0;     // no battery: see MOTOR_MIN_VIN
  for (uint8_t i = 0; i < 2; i++) {
    EncoderMotor &m = i ? m2 : m1;
    bool selected = (mWhich == 2 || mWhich == i);
    if (!selected || (mSet == 0 && !mRpmMode)) m.stop();
    else if (mRpmMode)                         m.setRPM(mSet);
    else                                       m.setPWM(mSet);
  }
}

// ============================================================================
//  Knob input
// ============================================================================
int8_t knobTurn() {                    // detents turned since the last call
  static long last = 0;
  long d = (knob.read() - last) / KNOB_COUNTS_PER_DETENT;
  last += d * KNOB_COUNTS_PER_DETENT;
  return d;
}

enum { BTN_NONE, BTN_CLICK, BTN_HOLD };
uint8_t knobButton() {
  static bool down = false, held = false;
  static uint32_t tChange = 0;
  bool now = digitalRead(PIN_KNOB_SW) == LOW;
  uint32_t t = millis();
  if (now != down) {
    if (t - tChange < 25) return BTN_NONE;            // debounce
    tChange = t;
    down = now;
    if (!down && !held) return BTN_CLICK;             // short press, on release
    held = false;
  } else if (down && !held && t - tChange > 600) {
    held = true;
    return BTN_HOLD;                                  // fires while still pressed
  }
  return BTN_NONE;
}

void gotoPage(uint8_t p) {
  motorsStop();                        // motors can only ever run on the motor page
  page = p;
  mEdit = false;
  mSel = 0;
  drawRowNext = 0;
  // Send the display its set-up commands again on every page change. If
  // electrical noise ever scrambles the OLED controller (blank, shifted or
  // mirrored picture), holding the knob brings it back.
  for (uint8_t i = 0; i < sizeof(Adafruit128x64init); i++) {
    oled.ssd1306WriteCmd(pgm_read_byte(&Adafruit128x64init[i]));
  }
}

void handleInput() {
  int8_t  turn = knobTurn();
  uint8_t btn  = knobButton();

  if (btn == BTN_HOLD && page != PG_MENU) { gotoPage(PG_MENU); return; }

  switch (page) {
    case PG_MENU:
      menuSel = (menuSel + MENU_ITEMS + turn % MENU_ITEMS) % MENU_ITEMS;
      if (btn == BTN_CLICK) {
        if (menuSel + 1 == PG_REINIT) {
          oled.clear();
          txt(F("Re-init sensors..."));
          initSensors();
          gotoPage(PG_STATUS);
        } else {
          gotoPage(menuSel + 1);
        }
      }
      break;

    case PG_ENC:
      if (btn == BTN_CLICK) { m1.resetPosition(0); m2.resetPosition(0); }
      break;

    case PG_MOTOR:
      if (btn == BTN_CLICK) mEdit = !mEdit;
      if (turn == 0) break;
      if (!mEdit) {
        mSel = constrain((int8_t)mSel + turn, 0, 2);
      } else if (mSel == 0) {
        motorsStop();
        mWhich = (mWhich + 3 + turn % 3) % 3;
      } else if (mSel == 1) {
        motorsStop();
        mRpmMode = !mRpmMode;
      } else {
        int step  = mRpmMode ? RPM_STEP  : PWM_STEP;
        int limit = mRpmMode ? RPM_LIMIT : PWM_LIMIT;
        mSet = constrain(mSet + turn * step, -limit, limit);
        motorsApply();
      }
      break;

    default:                           // read-only pages: click also goes back
      if (btn == BTN_CLICK) gotoPage(PG_MENU);
      break;
  }
}

// ============================================================================
//  Drawing. One text row is sent per pass of loop() (about 4 ms of I2C), so
//  the motor control loop and the knob are never starved by a full redraw.
// ============================================================================
void printVin() {
  txt(F("VIN"));
  fix(vinTenths, 1, 4);
  chr('V');
}

// "OK", "NO ACK" (nothing at that address) or "NOINIT" (answers, but start-up failed)
void printDevState(uint8_t dev) {
  if (!devAck[dev])                          txt(F("NO ACK"));
  else if (dev != DEV_OLED && !devInit[dev]) txt(F("NOINIT"));
  else                                       txt(F("OK    "));
}

void printTofValue(uint8_t i) {
  if (!devAck[i] || !devInit[i]) spaces(7);
  else if (tofMm[i] == 65535)    txt(F("timeout"));
  else if (tofMm[i] > 2000)      txt(F("  far  "));
  else { fix(tofMm[i], 0, 5); txt(F("mm")); }
}

void drawMotorRows(uint8_t r) {        // rows 4..7 of the motor page
  EncoderMotor &m = (r >= 6) ? m2 : m1;
  int pwm = m.getPWM();
  int rpm = m.getRPM();
  if (!(r & 1)) {
    txt(r >= 6 ? F("M2 pwm") : F("M1 pwm"));
    fix(pwm, 0, 5);
    txt(F(" rpm"));
    fix(rpm, 0, 5);
  } else {
    txt(F("   cnt"));
    fix(m.getCount(), 0, 8);
    chr(' ');
    // Quick verdict once the motor is being driven reasonably hard
    if (m.isStalled())             txt(F("STALL"));
    else if (abs(pwm) < 100)       ;
    else if (abs(rpm) < 3)         txt(F("STUCK"));    // no motion seen: motor, driver, battery or encoder
    else if ((pwm > 0) != (rpm > 0)) txt(F("REV"));    // encoder counts the wrong way
    else                           txt(F("ok"));
  }
}

// ---- ToF map ---------------------------------------------------------------
// Top view, mouse facing up. Each sensor draws a dotted beam from its place
// on the board out to whatever it sees, with a short solid bar there (the
// "wall"). Too far to measure = beam runs off the screen with no bar. A
// sensor that is not working draws nothing. The bottom text row repeats the
// five distances in mm, left to right.
//
// There is no frame buffer (the text library has none, and 1 KB would not
// fit in RAM next to everything else). Instead each 8-pixel strip of the
// screen is worked out on its own into mapBand[] and sent straight away.
const uint8_t MAP_X0 = 64, MAP_Y0 = 50;        // screen position of the point between the side sensors

// x and y are 8-bit on purpose: anything off the left/top wraps round to a
// big number and fails the same range test as off the right/bottom.
void mapDot(uint8_t x, uint8_t y, uint8_t r) {
  if (x < 128 && (y >> 3) == r) mapBand[x] |= 1 << (y & 7);
}

void drawMapRow(uint8_t r) {
  if (r == 7) {                                // numbers under the picture
    for (uint8_t i = 0; i < 5; i++) {
      if (!devAck[i] || !devInit[i]) txt(F("  --"));
      else if (tofMm[i] > 2000)      txt(F(" far"));
      else                           fix(tofMm[i], 0, 4);
    }
    return;
  }
  memset(mapBand, 0, sizeof(mapBand));
  if (r == MAP_Y0 / 8) {                       // the mouse: a small block
    mapBand[MAP_X0 - 1] = mapBand[MAP_X0] = mapBand[MAP_X0 + 1] = 0x0E;
  }
  for (uint8_t i = 0; i < 5; i++) {
    if (!devAck[i] || !devInit[i]) continue;
    int8_t dx = pgm_read_byte(&TOF_GEOM[i][2]);
    int8_t dy = pgm_read_byte(&TOF_GEOM[i][3]);
    uint8_t x = MAP_X0 + (int8_t)pgm_read_byte(&TOF_GEOM[i][0]) / (int8_t)MAP_MM_PER_PX;
    uint8_t y = MAP_Y0 - (int8_t)pgm_read_byte(&TOF_GEOM[i][1]) / (int8_t)MAP_MM_PER_PX;
    uint8_t len = min(tofMm[i] / MAP_MM_PER_PX, 255);      // "far" readings are > 8000 mm
    if (dx && dy) len = (uint16_t)len * 181 >> 8;          // diagonal steps are 1.41 px long
    while (len--) {                                        // dotted beam
      if (len & 1) mapDot(x, y, r);
      x += dx;
      y += dy;
      if (x >= 128 || y >= 56) break;                      // ran off the picture
    }
    if (len == 255) {                                      // got there: wall = bar across the beam
      for (int8_t j = -3; j <= 3; j++) mapDot(x - j * dy, y + j * dx, r);
    }
  }
  for (uint8_t c = 0; c < 128; c++) oled.ssd1306WriteRamBuf(mapBand[c]);
}

void drawRow(uint8_t r) {
  oled.setCursor(0, r);
  switch (page) {

    case PG_MENU: {
      if (r == 0) { txt(F("PCB TEST    ")); printVin(); break; }
      if (r > MENU_ITEMS) break;
      chr(menuSel == r - 1 ? '>' : ' ');
      switch (r) {
        case 1: txt(F("Status"));          break;
        case 2: txt(F("ToF map"));         break;
        case 3: txt(F("IMU"));             break;
        case 4: txt(F("Encoders"));        break;
        case 5: txt(F("Motor test"));      break;
        case 6: txt(F("Re-init sensors")); break;
      }
      break;
    }

    case PG_STATUS: {
      if (r == 0) {
        if (busError) txt(F("!I2C BUS ERR"));
        else { txt(F("UP")); fix(upSeconds, 0, 6); txt(F("s   ")); }      // seconds since boot
        printVin();
      } else {
        uint8_t dev = (r == 1) ? DEV_OLED : (r == 2) ? DEV_IMU : r - 3;
        if (dev < 5) { txt(F("ToF")); chr('1' + dev); }
        else         txt(dev == DEV_IMU ? F("IMU ") : F("OLED"));
        chr(' ');
        hex2(devAddr(dev));
        chr(' ');
        printDevState(dev);
        if (dev < 5) printTofValue(dev);
        else if (dev == DEV_OLED) { txt(F("RAM")); fix(ramFreeMin, 0, 4); }  // lowest free RAM, bytes
      }
      break;
    }

    case PG_IMU: {
      if (r == 0) {
        txt(F("IMU BNO085 "));
        hex2(ADDR_IMU);
        chr(' ');
        printDevState(DEV_IMU);
      } else if (r <= 3) {
        txt(r == 1 ? F("Yaw  ") : r == 2 ? F("Pitch") : F("Roll "));
        fix(imuYpr[r - 1], 1, 8);
        txt(F(" deg"));
      } else if (r == 5) {
        txt(F("rate"));
        fix(imuHz, 0, 3);
        txt(F("Hz  cal "));
        chr('0' + imuQuality);
      }
      break;
    }

    case PG_ENC: {
      if (r == 0) { txt(F("ENCODERS")); break; }
      if (r == 7) { txt(F("click=zero hold=back")); break; }
      EncoderMotor &m = (r >= 4) ? m2 : m1;
      long cnt = m.getCount();
      switch ((r - 1) % 3) {
        case 0: txt(r >= 4 ? F("M2 cnt") : F("M1 cnt")); fix(cnt, 0, 10); break;
        case 1: txt(F("   rev")); fix(cnt * 100.0f / m.getCountsPerRev(), 2, 10); break;
        case 2: txt(F("   rpm")); fix((long)m.getRPM(), 0, 10); break;
      }
      break;
    }

    case PG_MOTOR: {
      if (r == 0) {
        txt(vinTenths < MOTOR_MIN_VIN ? F("NO BATTERY! ") : F("MOTOR TEST  "));
        printVin();
      } else if (r <= 3) {
        chr(mSel != r - 1 ? ' ' : mEdit ? '*' : '>');
        if (r == 1) {
          txt(F("Motor  "));
          txt(mWhich == 0 ? F("M1") : mWhich == 1 ? F("M2") : F("BOTH"));
        } else if (r == 2) {
          txt(F("Mode   "));
          txt(mRpmMode ? F("RPM") : F("PWM"));
        } else {
          txt(F("Set  "));
          fix(mSet, 0, 5);
          txt(F(" hold=STOP"));
        }
      } else {
        drawMotorRows(r);
      }
      break;
    }

    case PG_MAP:
      drawMapRow(r);
      break;
  }
  oled.clearToEOL();
}

// ============================================================================
//  Serial report (useful when the OLED itself is the broken part)
// ============================================================================
#if SERIAL_REPORT
// One comma-separated line per second:
//   VIN(0.1V), IMU ok, yaw(0.1deg), ToF1..5 (mm, -1 = not working), enc M1, enc M2
void serialReport() {
  Serial.print(vinTenths);
  Serial.write(',');
  Serial.print(devAck[DEV_IMU] && devInit[DEV_IMU]);
  Serial.write(',');
  Serial.print(imuYpr[0]);
  for (uint8_t i = 0; i < 5; i++) {
    Serial.write(',');
    Serial.print(devAck[i] && devInit[i] ? (int)tofMm[i] : -1);
  }
  Serial.write(',');
  Serial.print(m1.getCount());
  Serial.write(',');
  Serial.println(m2.getCount());
}
#endif

// ============================================================================
//  setup / loop
// ============================================================================
void setup() {
  ramPaint();
  // Motors first, so the driver inputs are never left floating.
  m1.begin();
  m2.begin();
  m1.setReversed(M1_MOTOR_REVERSED, M1_ENC_REVERSED);
  m2.setReversed(M2_MOTOR_REVERSED, M2_ENC_REVERSED);
  // Same tuning as EncoderMotor_Simple.ino (only matters in RPM mode)
  m1.setSpeedGains(0.25, 0.05, 1.5);   m2.setSpeedGains(0.25, 0.05, 1.5);
  m1.setMinPWM(50, 50);                m2.setMinPWM(50, 50);
  m1.setStallProtection(500);          m2.setStallProtection(500);

  pinMode(PIN_KNOB_SW, INPUT_PULLUP);

#if SERIAL_REPORT
  Serial.begin(115200);
#endif

  Wire.begin();
  Wire.setClock(I2C_HZ);
  // Without this, a shorted or missing I2C line would hang the whole tester.
  Wire.setWireTimeout(25000, true);

  oled.begin(&Adafruit128x64, ADDR_OLED);
  oled.setFont(System5x7);
  oled.clear();
  txt(F("PCB TEST\n\nStarting sensors..."));

  initSensors();
  knob.write(0);
  gotoPage(PG_STATUS);                 // show the verdict straight away
}

void loop() {
  // 1. Motor control loops (they decide for themselves when 5 ms has passed)
  m1.update();
  m2.update();

  // 2. Knob
  handleInput();

  // 3. Sensors
  pollTof();
  pollImu();

  // 4. Slow jobs
  static uint32_t tSlow = 0;
  static uint8_t  slowTick = 0;
  if (millis() - tSlow >= 250) {
    tSlow = millis();
    readVin();
    pingAll();
    if (++slowTick >= 4) {             // once a second
      slowTick = 0;
      upSeconds++;
      ramFreeMin = ramUnused();
      imuHz = imuReports;
      imuReports = 0;
#if SERIAL_REPORT
      serialReport();
#endif
    }
  }

  // 5. Screen: one row per pass
  drawRow(drawRowNext);
  drawRowNext = (drawRowNext + 1) & 7;
}
