#define Xin A0
#define Yin A1
#define Zin A2

float Xbase, Ybase, Zbase;
float t;
void setup() {
  while (!Serial);
  Serial.begin(9600);

  delay(100);
  Xbase = 0;
  Zbase = 0;
  for(int i = 0; i < 10; i++) {
    Xbase += analogRead(Xin);
    Ybase += analogRead(Yin);
    Zbase += analogRead(Zin);
    delay(100);
  }
  Xbase /= 10;
  Ybase /= 10;
  Zbase /= 10;
}

void loop() {
  float x_g = analogRead(Xin) - Xbase;
  float y_g = analogRead(Yin) - Ybase;
  float z_g = analogRead(Zin) - Zbase;
  Serial.print("90,-90,");
  // Serial.print(x_g);Serial.print(',');Serial.print(y_g);Serial.print(',');
  Serial.println(y_g);
  delay(10);
}
