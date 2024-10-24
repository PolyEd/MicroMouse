#define M1_FORWARD A4
#define M1_BACKWARD A5
#define M2_FORWARD A3
#define M2_BACKWARD 13
void setup() {
  pinMode(M1_FORWARD, OUTPUT);
  pinMode(M1_BACKWARD, OUTPUT);
  pinMode(M2_FORWARD, OUTPUT);
  pinMode(M2_BACKWARD, OUTPUT);
}
void loop() {
  analogWrite(M1_BACKWARD, 0);
  //analogWrite(M2_BACKWARD, 0);
  analogWrite(M1_FORWARD, 255);
  //analogWrite(M2_FORWARD, 255);
  delay(1000);
  analogWrite(M1_FORWARD, 0);
  //analogWrite(M2_FORWARD, 0);
  analogWrite(M1_BACKWARD, 255);
  //analogWrite(M2_BACKWARD, 255);
  delay(1000);
}
