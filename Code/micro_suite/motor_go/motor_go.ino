#define M1_FORWARD A4
#define M1_BACKWARD A5
#define M2_FORWARD A3
#define M2_BACKWARD 13
void setup() {
  pinMode(M1_FORWARD, OUTPUT);
  pinMode(M1_BACKWARD, OUTPUT);
  pinMode(M2_FORWARD, OUTPUT);
  pinMode(M2_BACKWARD, OUTPUT);
  pinMode(9, INPUT);
  pinMode(10, INPUT);
  // delay(500);
  // Serial.begin(9600);
  // analogWrite(M1_BACKWARD, 255);
}
void loop() {
  analogWrite(M2_BACKWARD, 255);
  analogWrite(M2_FORWARD,0);
  delay(500);
  // analogWrite(M2_BACKWARD, 50);
analogWrite(M2_FORWARD, 255);
  analogWrite(M2_BACKWARD,0);
  delay(500);
  // Serial.println(digitalRead(10));
}