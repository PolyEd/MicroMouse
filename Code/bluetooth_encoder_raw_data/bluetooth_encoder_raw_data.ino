#define BLUETOOTH_IRQ 7
#define BLUETOOTH_SS 17
#define BLUETOOTH_RST -1
#define VERBOSE_MODE true
#include <Adafruit_BLE.h>
#include <Adafruit_BluefruitLE_SPI.h>


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
  pinMode(0, INPUT);
  pinMode(1, INPUT);

}
void loop() {
  analogWrite(13, 255);
  ble.println(analogRead(0));
}
