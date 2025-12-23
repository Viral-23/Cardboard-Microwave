#include <Arduino.h>
#include <Keypad.h>
#include <TM1637Display.h>
#include <WiFi.h>
#include <time.h>
#include "secrets.h"

// Magic Numbers
const unsigned long ONE_SECOND = 1000;
const unsigned long HALF_SECOND = 500;
const unsigned long ONE_MINUTE = 60000;
const unsigned long FIVE_SECOND = 5000;

const int DISPLAY_BRIGHTNESS = 2;
uint8_t data[] = {0x00, 0x00, 0x00, 0x00};

// Timer Display Initialization
const int CLKPin = 32;
const int DIOPin = 13;
TM1637Display display(CLKPin, DIOPin);
const uint8_t COLON_ON = 0b01000000;
const char* TZ_EST = "EST5EDT,M3.2.0/2,M11.1.0/2";
unsigned long doneDisplayStart = 0;
const unsigned long DONE_DISPLAY_DURATION = 5000; 
const unsigned long DONE_FLASH_INTERVAL = 500;    
bool showDone = false;
bool doneVisible = false; 
unsigned long lastDoneFlash = 0;


// LED Initialization
const int LEDPin = 14;

// Motor Initialization
const int motorPin = 4;
bool motorIsRunning = false;

void turnMotorOn() {
  digitalWrite(motorPin, HIGH);
  motorIsRunning = true;
  digitalWrite(LEDPin, HIGH);
}

void turnMotorOff() {
  digitalWrite(motorPin, LOW);
  motorIsRunning = false;
  digitalWrite(LEDPin, LOW);
}

// KeyPad Initialization
const byte ROWS = 4;
const byte COLS = 3;

char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

byte rowPins[ROWS] = { 21, 22, 23, 25 }; 
byte colPins[COLS] = { 26, 27, 33 };

Keypad numPad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
bool keypadInUse = false;
String displayedInput = "";
int timer = 0;

// Start and Clear Buttons Intialization
const int startPin = 18;
int startLastState = LOW;
int startCurrentState;
unsigned long lastStartPress = 0;

const int clearPin = 19;
int clearLastState = LOW;
int clearCurrentState;
unsigned long lastClearPress = 0;

// Timer/Delay Logic
unsigned long lastSecondUpdate = 0;
unsigned long updateTimestamp() {
  return millis();
}

unsigned long amountOfMsElapsed(int timestamp) {
  return millis() - timestamp;
}

unsigned long lastClockUpdate = 0; 


bool wifiSetup(unsigned long timeoutMs = 15000) {
  WiFi.begin(wifi_ssid, wifi_password);

  unsigned long start = millis();
  int dotCount = 0;

  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > timeoutMs) {
      Serial.println("\nWiFi failed, continuing without time sync");
      return false;
    }

    delay(500);
    Serial.print("\rConnecting to WiFi");
    for (int i = 0; i < dotCount; i++) Serial.print(".");
    dotCount = (dotCount + 1) % 4;
  }

  Serial.println("\nConnected to WiFi!");
  configTzTime(TZ_EST, "pool.ntp.org", "time.nist.gov");
  return true;
}

unsigned long lastWifiCheck = 0;
const unsigned long WIFI_CHECK_INTERVAL = FIVE_SECOND; 
bool ntpSynced = false;  

void reconnectWiFi() {

    if (amountOfMsElapsed(lastWifiCheck) >= WIFI_CHECK_INTERVAL) {
        lastWifiCheck = updateTimestamp();

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Wi-Fi lost! Attempting reconnect...");
            WiFi.disconnect();
            WiFi.begin(wifi_ssid, wifi_password);
            ntpSynced = false;  
        }
        else if (!ntpSynced) {
            Serial.println("Wi-Fi reconnected! Syncing NTP time...");
            configTzTime(TZ_EST, "pool.ntp.org", "time.nist.gov");
            ntpSynced = true;
        }
    }
}

void clockUpdate() {
      if (amountOfMsElapsed(lastClockUpdate) >= ONE_SECOND) { 
        lastClockUpdate = updateTimestamp();

        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            int hour = timeinfo.tm_hour;
            int minute = timeinfo.tm_min;

            int displayHour = hour;
            if (displayHour == 0) displayHour = 12;
            else if (displayHour > 12) displayHour -= 12;

            int displayTime = displayHour * 100 + minute;

            if (!motorIsRunning && displayedInput.length() == 0) {
                display.showNumberDecEx(displayTime, COLON_ON, true, 4, 0);
            }
        }
    }
}

void updateDisplay() {
    if (showDone) {

        if (amountOfMsElapsed(doneDisplayStart) >= DONE_DISPLAY_DURATION) {
            showDone = false; // stop flashing after duration
        } else {
            if (amountOfMsElapsed(lastDoneFlash) >= DONE_FLASH_INTERVAL) {
                doneVisible = !doneVisible;
                lastDoneFlash = updateTimestamp();
            }

            if (doneVisible) {
            uint8_t DONE[] = {
              0b1011110, // D 1011110
              0b0111111, // O 0111111
              0b0110111, // N 0110111
              0b1111001  // E 1111001
            };
                display.setSegments(DONE);
            } else {
                display.clear();
            }
            return; 
        }
    }

    if (motorIsRunning && timer > 0) {
        int minutes = timer / 60;
        int seconds = timer % 60;
        int displayTime = minutes * 100 + seconds;
        display.showNumberDecEx(displayTime, COLON_ON, false, 4, 0);
    }
    else if (displayedInput.length() > 0) {
        display.showNumberDecEx(displayedInput.toInt(), COLON_ON, false, 4, 0);
    }
    else {
        clockUpdate();
    }
}


void setup()
{
  Serial.begin(115200); 
  bool timeSynced = wifiSetup();  
  if (!timeSynced) {
    Serial.println("Continuing without wifi");
  }
  pinMode(motorPin, OUTPUT);
  pinMode(LEDPin, OUTPUT);
  pinMode(startPin, INPUT_PULLUP);
  pinMode(clearPin, INPUT_PULLUP);
  digitalWrite(motorPin, LOW); 
  digitalWrite(LEDPin, LOW);
  display.clear();           
  display.setBrightness(DISPLAY_BRIGHTNESS); 
}

void loop() {
  reconnectWiFi();
  updateDisplay();
  startCurrentState = digitalRead(startPin);
  clearCurrentState = digitalRead(clearPin);

  boolean startButtonPressed = (startCurrentState == LOW && startLastState == HIGH);
  boolean clearButtonPressed = (clearCurrentState == LOW && clearLastState == HIGH);


   if (!keypadInUse) {
    if (!showDone) {
      if (startButtonPressed) {
        if (amountOfMsElapsed(lastStartPress) > HALF_SECOND) {
          if (!motorIsRunning) {
            timer = (displayedInput.toInt() / 100) * 60 + (displayedInput.toInt() % 100);
            Serial.println("Timer is: " + String(timer));
            if (timer > 0) {
              displayedInput = String(min(displayedInput.toInt(), 9959L));
              turnMotorOn();
              lastSecondUpdate = updateTimestamp();
            }
          }
          lastStartPress = updateTimestamp();
        }
      }
    }


    if (clearButtonPressed) {
      if (amountOfMsElapsed(lastClearPress) > HALF_SECOND) {
        if (!motorIsRunning) {
            timer = 0;
            displayedInput = "";
            Serial.println("Timer cleared");
            showDone = false;
            clockUpdate();
        }
        else
        {
          // First click: store remaining time as input
          turnMotorOff();
          displayedInput = String((String(timer / 60) + ((timer % 60 < 10) ? "0" : "") + String(timer % 60)).toInt());
          Serial.println("Input is: " + displayedInput);
          Serial.println("Timer stopped: " + String(timer));
        }

        lastClearPress = updateTimestamp();

      }
    }
  }
    startLastState = startCurrentState;
    clearLastState = clearCurrentState;

  if (motorIsRunning && timer > 0) {
    if (amountOfMsElapsed(lastSecondUpdate) >= ONE_SECOND) {
      lastSecondUpdate = updateTimestamp();
      timer--;

      Serial.print("Time remaining: ");
      Serial.println(timer);

    if (timer == 0) {
        turnMotorOff();
        displayedInput = ""; 
        Serial.println("Done");

        showDone = true;
        doneVisible = true;          
        doneDisplayStart = millis();
        lastDoneFlash = millis();
      }
    }
  }

  if (!showDone) {
    char num = numPad.getKey();

    if (num != NO_KEY) {
      keypadInUse = true;

      if (!motorIsRunning) {
        if (displayedInput.length() < 4 && !(displayedInput.length() == 0 && num == '0')) {
          displayedInput += num;
        }
        Serial.println("The current input is: " + displayedInput);
      }
    } else {
      keypadInUse = false;
    } 
  }
}
