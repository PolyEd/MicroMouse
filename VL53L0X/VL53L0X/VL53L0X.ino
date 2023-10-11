#include <Wire.h>
#include <VL53L0X.h>

#define NUM_READINGS 10 // Number of readings to average

VL53L0X sensor;
int readings[NUM_READINGS];  // Array to store the readings
int index = 0;               // Index to keep track of the current reading position

void setup() {
  Serial.begin(9600);
  Wire.begin();

  sensor.setTimeout(500);
  if (!sensor.init()) {
    Serial.println("Failed to detect and initialize sensor!");
    while (1) {}
  }

  sensor.startContinuous(5); // ms delay between readings
}

void loop() {
  // Read the distance
  int distance = sensor.readRangeContinuousMillimeters();

  // Store the reading in the array and update the index
  readings[index] = distance;
  index = (index + 1) % NUM_READINGS;

  // Calculate the running average
  int sum = 0;
  for (int i = 0; i < NUM_READINGS; i++) {
    sum += readings[i];
  }
  float average = (float)sum / NUM_READINGS;

  // Print the running average
  Serial.println(average);
  
}
