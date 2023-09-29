
const int Xin = A0;
const int Yin = A1;
const int Zin = A2;

float Xbase, Ybase, Zbase;

void setup() {
  Serial.begin(9600);

  delay(100);
  Xbase = analogRead(Xin);
  Ybase = analogRead(Yin);
  Zbase = analogRead(Zin);

//  Serial.println(Xbase);
//  Serial.println(Ybase);
//  Serial.println(Zbase);

  
}

void loop() {

  Serial.print("GraphMin:-180,GraphMax:180,");
  Serial.print("X:");
  Serial.print(analogRead(Xin) - Xbase);
  Serial.print(",");
  Serial.print("Y:");
  Serial.print(analogRead(Yin) - Ybase);
//  Serial.print(",");
//  Serial.print("Z:");
//  Serial.print(analogRead(Zin) - Zbase);
  Serial.println("");
}
