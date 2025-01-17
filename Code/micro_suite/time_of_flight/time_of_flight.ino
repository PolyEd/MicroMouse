const int xshutpin[4] = {5,6,7,8};
const int sensor_new_address[4] = {42,43,44,45};
#include <VL53L0X.h>
#include <Wire.h>
#define NUM_READINGS 10
#include <stdbool.h>
VL53L0X sensors[5];
int readings[5][NUM_READINGS];
int index = 0;
float running_avg[5];
bool working[5];
void setup() {
  while (!Serial);
  Serial.begin(9600);
  Wire.begin();
  for(int i = 0; i < 4; i++) {
    pinMode(xshutpin[i],OUTPUT);
    sensors[i].setAddress(sensor_new_address[i]);
    pinMode(xshutpin[i],INPUT);
    delay(10);
  }
  for(int i = 0; i < 5; i++) {
    sensors[i].setTimeout(500);
    working[i] = sensors[i].init();
    sensors[i].startContinuous(5);
  }
  Serial.print("\x1b[?25l");
}
void loop() {
  for(int i = 0; i < 5; i++) {
    if (working[i]) {
      readings[i][index] = sensors[i].readRangeContinuousMillimeters();
      int sum = 0;
      for(int j = 0; j < NUM_READINGS; j++) {
        sum += readings[i][j];
      }
      running_avg[i] = (float)sum / NUM_READINGS;
    }
  }
  Serial.print("\x1b[0G");
  Serial.print(millis());
  Serial.print("[");
  for(int i = 0; i < 4; i++) {
    if(working[i]) Serial.print(running_avg[i]); else Serial.print(0);
    Serial.print(',');
  }
  if(working[4]) Serial.print(running_avg[4]);
  Serial.print("],");
}
