#include <Arduino.h>
#include "DFRobotDFPlayerMini.h"

#define RX_PIN 26
#define TX_PIN 27
#define DFP_BAUD 9600

HardwareSerial DFPSerial(2); // UART2
DFRobotDFPlayerMini myDFPlayer;

void printDetail(uint8_t type, int value);

void setup() {
  Serial.begin(115200);
  DFPSerial.begin(DFP_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(1000); // allow serial to start

  Serial.println();
  Serial.println("DFPlayer Mini Test - ESP32");
  Serial.println("Initializing DFPlayer...");

  if (!myDFPlayer.begin(DFPSerial, true, true)) {
    Serial.println("Unable to begin DFPlayer!");
    Serial.println("1. Check wiring.");
    Serial.println("2. Make sure SD card is inserted.");
      while(true){
        delay(0); // Code to compatible with ESP8266 watch dog.
    }  
  }

  Serial.println("DFPlayer Mini online.");
  myDFPlayer.volume(10); // high volume for testing
  myDFPlayer.play(1);    // start first file
}

void loop() {
 
}
