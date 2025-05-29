// #include <Wire.h>
// #include <VL53L0X.h>

// #define xshutpin_1 5
// #define xshutpin_2 6
// #define xshutpin_3 7
// #define xshutpin_4 8
// #define xshutpin_5 9

// #define sensor1_newaddress 42
// #define sensor2_newaddress 43
// #define sensor3_newaddress 44
// #define sensor4_newaddress 45
// #define sensor5_newaddress 46

// #define NUM_READINGS 4 // Number of readings to average for stability

// // Motor speed settings
// #define MOTOR_SPEED_STRAIGHT 180
// #define MOTOR_SPEED_TURN 200
// #define TURN_THRESHOLD 15

// int working[5] = {1, 1, 1, 1, 1};


// VL53L0X vl53l0x;
// int readings[NUM_READINGS];  // Array to store the readings
// int index = 0;               // Index to keep track of the current reading position
// float runningAvg;

// VL53L0X vl53l0x_2;
// int readings_2[NUM_READINGS];  // Arrato store the readings
// int index_2 = 0;               // Index to keep track of the current reading position
// float runningAvg_2;

// VL53L0X vl53l0x_3;
// int readings_3[NUM_READINGS];  // Array to store the readings
// int index_3 = 0;               // Index to keep track of the current reading position
// float runningAvg_3;

// VL53L0X vl53l0x_4;
// int readings_4[NUM_READINGS];  // Array to store the readings
// int index_4 = 0;               // Index to keep track of the current reading position
// float runningAvg_4;

// VL53L0X vl53l0x_5;
// int readings_5[NUM_READINGS];  // Array to store the readings
// int index_5 = 0;               // Index to keep track of the current reading position
// float runningAvg_5;


// #define M1_BACKWARD A5
// #define M1_FORWARD A4
// #define M2_FORWARD A3
// #define M2_BACKWARD 13


// void setup() {
//   Serial.begin(115200);
//   Wire.begin();

//   // Shutdown all sensors initially
//   pinMode(xshutpin_1, OUTPUT);
//   pinMode(xshutpin_2, OUTPUT);
//   pinMode(xshutpin_3, OUTPUT);
//   pinMode(xshutpin_4, OUTPUT);
//   pinMode(xshutpin_5, OUTPUT);

//   digitalWrite(xshutpin_1, LOW);
//   digitalWrite(xshutpin_2, LOW);
//   digitalWrite(xshutpin_3, LOW);
//   digitalWrite(xshutpin_4, LOW);
//   digitalWrite(xshutpin_5, LOW);
//   delay(10);

//   // Initialize each sensor one by one with unique address
//   digitalWrite(xshutpin_1, HIGH);
//   delay(10);
//   vl53l0x.setAddress(sensor1_newaddress);

//   digitalWrite(xshutpin_2, HIGH);
//   delay(10);
//   vl53l0x_2.setAddress(sensor2_newaddress);

//   digitalWrite(xshutpin_3, HIGH);
//   delay(10);
//   vl53l0x_3.setAddress(sensor3_newaddress);

//   digitalWrite(xshutpin_4, HIGH);
//   delay(10);
//   vl53l0x_4.setAddress(sensor4_newaddress);

//   digitalWrite(xshutpin_5, HIGH);
//   delay(10);
//   vl53l0x_5.setAddress(sensor5_newaddress);

//   vl53l0x.setTimeout(500);
//   if (!vl53l0x.init()) {
//     // Serial.begin(9600);
//     // while (1) Serial.println("Failed to detect and initialize sensor 1!");
//     working[0] = 0;
//   }

//   vl53l0x_2.setTimeout(500);
//   if (!vl53l0x_2.init()) {
//     // Serial.begin(9600);
//     // while (1) Serial.println("Failed to detect and initialize sensor 2!");
//     working[1] = 0;
//   }

//   vl53l0x_3.setTimeout(500);
//   if (!vl53l0x_3.init()) {
//     // Serial.begin(9600);
//     // while (1) Serial.println("Failed to detect and initialize sensor 3!");
//     working[2] = 0;
//   }

//   vl53l0x_4.setTimeout(500);
//   if (!vl53l0x_4.init()) {
//     // Serial.begin(9600);
//     // while (1) Serial.println("Failed to detect and initialize sensor 4!");
//     working[3] = 0;
//   }

//   vl53l0x_5.setTimeout(500);
//   if (!vl53l0x_5.init()) {
//     // Serial.begin(9600);
//     // while (1) Serial.println("Failed to detect and initialize sensor 5!");
//     working[4] = 0;
//   }

//   if (working[0]) vl53l0x.startContinuous(5); // ms delay between readings
//   if (working[1]) vl53l0x_2.startContinuous(5); // ms delay between readings
//   if (working[2]) vl53l0x_3.startContinuous(5); // ms delay between readings
//   if (working[3]) vl53l0x_4.startContinuous(5); // ms delay between readings
//   if (working[4]) vl53l0x_5.startContinuous(5); // ms delay between readings


//   pinMode(M1_FORWARD, OUTPUT);
//   pinMode(M1_BACKWARD, OUTPUT);
//   pinMode(M2_FORWARD, OUTPUT);
//   pinMode(M2_BACKWARD, OUTPUT);


// }
// void loop() {
//   // Initialize readings arrays if not done
//   static boolean initialized = false;
//   if (!initialized) {
//     // Initialize all sensor reading arrays with current values for a faster startup
//     for (int i = 0; i < NUM_READINGS; i++) {
//       readings[i] = working[0] ? vl53l0x.readRangeContinuousMillimeters() : 32000;
//       readings_2[i] = working[1] ? vl53l0x_2.readRangeContinuousMillimeters() : 32000;
//       readings_3[i] = working[2] ? vl53l0x_3.readRangeContinuousMillimeters() : 32000;
//       readings_4[i] = working[3] ? vl53l0x_4.readRangeContinuousMillimeters() : 32000;
//       readings_5[i] = working[4] ? vl53l0x_5.readRangeContinuousMillimeters() : 32000;
//       delay(5);
//     }
//     initialized = true;
//   }

//   int distance = working[0]?vl53l0x.readRangeContinuousMillimeters():32000;
//   int distance_2 = working[1]?vl53l0x_2.readRangeContinuousMillimeters():32000;
//   int distance_3 = working[2]?vl53l0x_3.readRangeContinuousMillimeters():32000;
//   int distance_4 = working[3]?vl53l0x_4.readRangeContinuousMillimeters():32000;
//   int distance_5 = working[4]?vl53l0x_5.readRangeContinuousMillimeters():32000;

//   if (working[0]) {
//     // Store the reading in the array and update the index
//     readings[index] = distance;
//     index = (index + 1) % NUM_READINGS;

//     // Calculate the running average
//     int sum = 0;
//     for (int i = 0; i < NUM_READINGS; i++) {
//       sum += readings[i];
//     }
//     runningAvg = (float)sum / NUM_READINGS;
//   }

//   if (working[1]) {
//     // Store the reading in the array and update the index
//     readings_2[index_2] = distance_2;
//     index_2 = (index_2 + 1) % NUM_READINGS;

//     // Calculate the running average
//     int sum = 0;
//     for (int i = 0; i < NUM_READINGS; i++) {
//       sum += readings_2[i];
//     }
//     runningAvg_2 = (float)sum / NUM_READINGS;
//   }

//   if (working[2]) {
//     // Store the reading in the array and update the index
//     readings_3[index_3] = distance_3;
//     index_3 = (index_3 + 1) % NUM_READINGS;

//     // Calculate the running average
//     int sum = 0;
//     for (int i = 0; i < NUM_READINGS; i++) {
//       sum += readings_3[i];
//     }
//     runningAvg_3 = (float)sum / NUM_READINGS;
//   }

//   if (working[3]) {
//     // Store the reading in the array and update the index
//     readings_4[index_4] = distance_4;
//     index_4 = (index_4 + 1) % NUM_READINGS;

//     // Calculate the running average
//     int sum = 0;
//     for (int i = 0; i < NUM_READINGS; i++) {
//       sum += readings_4[i];
//     }
//     runningAvg_4 = (float)sum / NUM_READINGS;
//   }

//   if (working[4]) {
//     // Store the reading in the array and update the index
//     readings_5[index_5] = distance_5;
//     index_5 = (index_5 + 1) % NUM_READINGS;

//     // Calculate the running average
//     int sum = 0;
//     for (int i = 0; i < NUM_READINGS; i++) {
//       sum += readings_5[i];
//     }
//     runningAvg_5 = (float)sum / NUM_READINGS;
//   }

//   // Calculate difference between left and right sensors
//   float diff = runningAvg_2 - runningAvg_4;

//   // Debug output to monitor sensor values
//   Serial.println(String(runningAvg) + "," + String(runningAvg_2) + "," + String(runningAvg_3) + "," + String(runningAvg_4) + "," + String(runningAvg_5) + ",diff:" + String(diff));

//   // Adjust motor control based on sensor difference
//   if (diff > TURN_THRESHOLD) { // Wall is closer on the left side, turn right
//     // Right turn - left motor forward, right motor backward
//     analogWrite(M1_FORWARD, MOTOR_SPEED_TURN);
//     analogWrite(M1_BACKWARD, 0);
//     analogWrite(M2_FORWARD, 0);
//     analogWrite(M2_BACKWARD, MOTOR_SPEED_TURN);
//     delay(5);
//   }
//   else if (diff < -TURN_THRESHOLD) { // Wall is closer on the right side, turn left
//     // Left turn - left motor backward, right motor forward
//     analogWrite(M1_FORWARD, 0);
//     analogWrite(M1_BACKWARD, MOTOR_SPEED_TURN);
//     analogWrite(M2_FORWARD, MOTOR_SPEED_TURN);
//     analogWrite(M2_BACKWARD, 0);
//     delay(5);
//   }
//   else { // Centered between walls, go straight
//     // Both motors forward at same speed
//     analogWrite(M1_FORWARD, MOTOR_SPEED_STRAIGHT);
//     analogWrite(M1_BACKWARD, 0);
//     analogWrite(M2_FORWARD, MOTOR_SPEED_STRAIGHT);
//     analogWrite(M2_BACKWARD, 0);
//     delay(10);
//   }

//   // Stop if obstacle detected in front (sensor 3)
//   if (runningAvg_3 < 300) {
//     // Stop all motors
//     analogWrite(M1_BACKWARD, 0);
//     analogWrite(M2_BACKWARD, 0);
//     analogWrite(M1_FORWARD, 0);
//     analogWrite(M2_FORWARD, 0);

//     Serial.println("OBSTACLE DETECTED - STOPPING");
//     delay(100); // Longer delay when stopped
//   }

// }
