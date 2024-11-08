#include <Wire.h>
#include <VL53L0X.h>

#define xshutpin_1 5
#define xshutpin_2 6
#define xshutpin_3 7
#define xshutpin_4 8

#define sensor1_newaddress 42
#define sensor2_newaddress 43
#define sensor3_newaddress 44
#define sensor4_newaddress 45

#define NUM_READINGS 2 // Number of readings to average

int working[5] = {1, 1, 1, 1, 1};


VL53L0X vl53l0x;
int readings[NUM_READINGS];  // Array to store the readings
int index = 0;               // Index to keep track of the current reading position
float runningAvg;

VL53L0X vl53l0x_2;
int readings_2[NUM_READINGS];  // Arrato store the readings
int index_2 = 0;               // Index to keep track of the current reading position
float runningAvg_2;

VL53L0X vl53l0x_3;
int readings_3[NUM_READINGS];  // Array to store the readings
int index_3 = 0;               // Index to keep track of the current reading position
float runningAvg_3;

VL53L0X vl53l0x_4;
int readings_4[NUM_READINGS];  // Array to store the readings
int index_4 = 0;               // Index to keep track of the current reading position
float runningAvg_4;

VL53L0X vl53l0x_5;
int readings_5[NUM_READINGS];  // Array to store the readings
int index_5 = 0;               // Index to keep track of the current reading position
float runningAvg_5;

void setup() {
  
  Wire.begin();

  pinMode(xshutpin_1, OUTPUT);
  vl53l0x.setAddress(sensor1_newaddress);
  pinMode(xshutpin_1, INPUT);
  delay(10);
  pinMode(xshutpin_2, OUTPUT);
  vl53l0x_2.setAddress(sensor2_newaddress);
  pinMode(xshutpin_2, INPUT);
  delay(10);
  pinMode(xshutpin_3, OUTPUT);
  vl53l0x_3.setAddress(sensor3_newaddress);
  pinMode(xshutpin_3, INPUT);
  delay(10);
  pinMode(xshutpin_4, OUTPUT);
  vl53l0x_4.setAddress(sensor4_newaddress);
  pinMode(xshutpin_4, INPUT);
  delay(10);

  vl53l0x.setTimeout(500);
  if (!vl53l0x.init()) {
    // Serial.begin(9600);
    // while (1) Serial.println("Failed to detect and initialize sensor!");
    working[0] = 0;
  }

  vl53l0x_2.setTimeout(500);
  if (!vl53l0x_2.init()) {
    // Serial.begin(9600);
    // while (1) Serial.println("Failed to detect and initialize sensor 2!");
    working[1] = 0;
  }

  vl53l0x_3.setTimeout(500);
  if (!vl53l0x_3.init()) {
    // Serial.begin(9600);
    // while (1) Serial.println("Failed to detect and initialize sensor 3!");
    working[2] = 0;
  }

  vl53l0x_4.setTimeout(500);
  if (!vl53l0x_4.init()) {
    // Serial.begin(9600);
    // while (1) Serial.println("Failed to detect and initialize sensor 4!");
    working[3] = 0;
  }

  vl53l0x_5.setTimeout(500);
  if (!vl53l0x_5.init()) {
    // Serial.begin(9600);
    // while (1) Serial.println("Failed to detect and initialize sensor 5!");
    working[4] = 0;
  }

  if (working[0]) vl53l0x.startContinuous(5); // ms delay between readings
  if (working[1]) vl53l0x_2.startContinuous(5); // ms delay between readings
  if (working[2]) vl53l0x_3.startContinuous(5); // ms delay between readings
  if (working[3]) vl53l0x_4.startContinuous(5); // ms delay between readings
  if (working[4]) vl53l0x_5.startContinuous(5); // ms delay between readings

}
void loop() {

  int distance = working[0]?vl53l0x.readRangeContinuousMillimeters():0;
  int distance_2 = working[1]?vl53l0x_2.readRangeContinuousMillimeters():0;
  int distance_3 = working[2]?vl53l0x_3.readRangeContinuousMillimeters():0;
  int distance_4 = working[3]?vl53l0x_4.readRangeContinuousMillimeters():0;
  int distance_5 = working[4]?vl53l0x_5.readRangeContinuousMillimeters():0;

  if (working[0]) {
    // Store the reading in the array and update the index
    
    readings[index] = distance;
    index = (index + 1) % NUM_READINGS;
  
    // Calculate the running average
    int sum = 0;
    for (int i = 0; i < NUM_READINGS; i++) {
      sum += readings[i];
    }
    runningAvg = (float)sum / NUM_READINGS;
  }

  if (working[1]) {
    // Store the reading in the array and update the index
    readings_2[index] = distance_2;
    index = (index + 1) % NUM_READINGS;
    
    // Calculate the running average
    int sum = 0;
    for (int i = 0; i < NUM_READINGS; i++) {
      sum += readings_2[i];
    }
    runningAvg_2 = (float)sum / NUM_READINGS;
  }

  if (working[2]) {
    // Store the reading in the array and update the index
    readings_3[index] = distance_3;
    index = (index + 1) % NUM_READINGS;
    
    // Calculate the running average
    int sum = 0;
    for (int i = 0; i < NUM_READINGS; i++) {
      sum += readings_3[i];
    }
    runningAvg_3 = (float)sum / NUM_READINGS;
  }

  if (working[3]) {
    // Store the reading in the array and update the index
    readings_4[index] = distance_4;
    index = (index + 1) % NUM_READINGS;
    
    // Calculate the running average
    int sum = 0;
    for (int i = 0; i < NUM_READINGS; i++) {
      sum += readings_4[i];
    }
    runningAvg_4 = (float)sum / NUM_READINGS;
  }

  if (working[4]) {
    // Store the reading in the array and update the index
    readings_5[index] = distance_5;
    index = (index + 1) % NUM_READINGS;
    
    // Calculate the running average
    int sum = 0;
    for (int i = 0; i < NUM_READINGS; i++) {
      sum += readings_5[i];
    }
    runningAvg_5 = (float)sum / NUM_READINGS;
  }

  Serial.println(String(runningAvg) + "," + String(runningAvg_2) + "," + String(runningAvg_3) + "," + String(runningAvg_4) + "," + String(runningAvg_5));

}
