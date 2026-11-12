/*
 * ============================================================================
 *  EncoderCountTest.ino
 * ============================================================================
 *  Measures the REAL encoder counts per output-shaft revolution by turning
 *  the shaft by hand. The motor is never powered (driver pins held low =
 *  coast), so this also rules out electrical noise from the motor.
 *
 *  How to use (Serial Monitor, 115200 baud, line ending "Newline"):
 *    1. Put a mark on the shaft / wheel and line it up with something fixed.
 *    2. Send  z        -> zero the count.
 *    3. Turn the shaft by hand exactly N full turns, ONE direction only,
 *       until the mark lines up again.
 *    4. Send  N        (e.g. 10) -> prints counts per rev and the gear
 *       ratio to put in the EncoderMotor constructor.
 *
 *  Datasheet (ServoCity 30:1): 359.7222 counts/rev, ratio 29.9777.
 *  10 turns should read about 3597 counts if the motor matches it.
 *
 *  Needs the "Encoder" library by Paul Stoffregen (Library Manager).
 * ============================================================================
 */

#include <Encoder.h>

// Same pins as EncoderMotor_Simple.ino
const uint8_t DRV_A = 6;
const uint8_t DRV_B = 5;
const uint8_t ENC_A = 2;
const uint8_t ENC_B = 3;

const float ENCODER_PPR   = 3.0;      // pulses per MOTOR rev, per channel
const float QUAD_MULT     = 4.0;      // Encoder library counts all 4 edges
const float DATASHEET_CPR = 359.7222; // counts per OUTPUT rev, per datasheet

Encoder enc(ENC_A, ENC_B);

void setup() {
  // Keep the motor off (coast) so the shaft can be turned freely.
  pinMode(DRV_A, OUTPUT);
  pinMode(DRV_B, OUTPUT);
  digitalWrite(DRV_A, LOW);
  digitalWrite(DRV_B, LOW);

  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}  // wait for USB serial (Micro)

  Serial.println(F("EncoderCountTest: send 'z' to zero, then turn N times and send N"));
}

void loop() {
  // ---- Commands ---------------------------------------------------------
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();

    if (line.equalsIgnoreCase("z")) {
      enc.write(0);
      Serial.println(F("--- zeroed ---"));
    } else if (line.length() > 0) {
      float turns = line.toFloat();
      if (turns > 0) {
        long  counts = abs(enc.read());
        float cpr    = counts / turns;
        float ratio  = cpr / (ENCODER_PPR * QUAD_MULT);
        float errPct = (cpr / DATASHEET_CPR - 1.0) * 100.0;

        Serial.println(F("=========================================="));
        Serial.print(F("counts:          ")); Serial.println(counts);
        Serial.print(F("turns:           ")); Serial.println(turns, 2);
        Serial.print(F("counts per rev:  ")); Serial.println(cpr, 3);
        Serial.print(F("datasheet:       ")); Serial.println(DATASHEET_CPR, 3);
        Serial.print(F("difference:      ")); Serial.print(errPct, 2); Serial.println(F(" %"));
        Serial.print(F("gear ratio:      ")); Serial.println(ratio, 4);
        Serial.println(F("-> use that ratio in the EncoderMotor constructor"));
        Serial.println(F("=========================================="));
      } else {
        Serial.println(F("? send 'z' or a number of turns, e.g. 10"));
      }
    }
  }

  // ---- Live readout, 5x per second ----------------------------------------
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 200) {
    lastPrint = millis();
    long c = enc.read();
    Serial.print(F("count:"));  Serial.print(c);
    Serial.print(F(" turns:")); Serial.println(c / DATASHEET_CPR, 3);
  }
}
