#define BLUETOOTH_IRQ 7
#define BLUETOOTH_SS 17
#define BLUETOOTH_RST -1
#define VERBOSE_MODE true
#include <Adafruit_BLE.h>
#include <Adafruit_BluefruitLE_SPI.h>


// Pin definitions
#define ENCODER_A_PIN 0  // Encoder A output
#define ENCODER_B_PIN 1  // Encoder B output

// Variables to store encoder data
volatile long encoderPosition = 0;  // Tracks the position of the encoder
volatile int lastEncoded = 0;       // Last encoded value

Adafruit_BluefruitLE_SPI ble(BLUETOOTH_SS, BLUETOOTH_IRQ, BLUETOOTH_RST);
void setup() {
  if (!ble.begin(VERBOSE_MODE)) {
    
  }
  ble.echo(false);
  if (
    !ble.sendCommandCheckOK(F( "AT+GAPDEVNAME=Bluefruit (SPI)" ))
  ) ;
  if (
    !ble.sendCommandCheckOK(F( "+++" ))
  ) ;

  pinMode(13, OUTPUT);

  // Set encoder pins as inputs
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);

  // Attach interrupts to handle encoder signals
  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN), updateEncoder, CHANGE);
}

void loop() {
  analogWrite(13, 255);
  ble.println(encoderPosition);
}

// Interrupt service routine (ISR) for the encoder
void updateEncoder() {
  // Read the current state of the encoder pins
  int MSB = digitalRead(ENCODER_A_PIN); // Most Significant Bit
  int LSB = digitalRead(ENCODER_B_PIN); // Least Significant Bit

  int encoded = (MSB << 1) | LSB;       // Combine the two bits to form a 2-bit number
  int sum = (lastEncoded << 2) | encoded; // Calculate the change

  // Determine direction based on the transition table
  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderPosition++;
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderPosition--;

  lastEncoded = encoded; // Update the last encoded value
}
