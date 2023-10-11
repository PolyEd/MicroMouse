#include <Plotter.h>
#include <Wire.h>
#include <VL53L0X.h>


#define NUM_READINGS 10 // Number of readings to average

VL53L0X vl53l0x;
int readings[NUM_READINGS];  // Array to store the readings
int index = 0;               // Index to keep track of the current reading position
float runningAvg;

Plotter plot;

void setup() {
  Wire.begin();

  vl53l0x.setTimeout(500);
  if (!vl53l0x.init()) {
    Serial.begin(9600);
    Serial.println("Failed to detect and initialize sensor!");
    while (1) {}
  }

  // We only do Serial.begin() if there is a fatal error because the Serial API cannot be used while using the plotter library

  plot.Begin();
  plot.AddTimeGraph("Distance running avg in mm", 500, "", runningAvg); // title of graph, zoom level as num of points displayed at once, label of variable, variable
                                                                        // you have to put the thing thats being plotted in a global variable then pass it to this
                                                                        // and update the global in loop()

  vl53l0x.startContinuous(5); // ms delay between readings
}

void loop() {
  // Read the distance
  int distance = vl53l0x.readRangeContinuousMillimeters();

  // Store the reading in the array and update the index
  readings[index] = distance;
  index = (index + 1) % NUM_READINGS;

  // Calculate the running average
  int sum = 0;
  for (int i = 0; i < NUM_READINGS; i++) {
    sum += readings[i];
  }
  runningAvg = (float)sum / NUM_READINGS;

  // Update the plotter
  plot.Plot();
  
}
