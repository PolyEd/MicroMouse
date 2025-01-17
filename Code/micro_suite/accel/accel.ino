#define Xin A0
#define Yin A1
#define Zin A2

float Xbase, Ybase, Zbase;
float x_vel, x_pos;
float y_vel, y_pos;
float z_vel, z_pos;
float t;
void setup() {
  while (!Serial);
  Serial.begin(9600);

  delay(100);
  Xbase = 0;
  Ybase = 0;
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
  t = millis();
}

void loop() {
  float dt = millis()-t;
  t = millis();
  float x_g = analogRead(Xin) - Xbase;
  float y_g = analogRead(Yin) - Ybase;
  float z_g = analogRead(Zin) - Zbase;
  x_vel += dt * x_g;
  y_vel += dt * y_g;
  z_vel += dt * z_g;
  x_pos += dt * x_vel;
  y_pos += dt * y_vel;
  z_pos += dt * z_vel;
  Serial.print(t); Serial.print(',');Serial.print(x_pos);Serial.print(',');Serial.print(y_pos);Serial.print(',');Serial.print(z_pos);
  Serial.print(',');Serial.print(x_vel);Serial.print(',');Serial.print(y_vel);Serial.print(',');Serial.print(z_vel);
  Serial.print(',');Serial.print(x_g);Serial.print(',');Serial.print(y_g);Serial.print(',');Serial.println(z_g);
}
