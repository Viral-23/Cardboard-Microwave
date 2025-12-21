#include <Arduino.h>
#include <Keypad.h>
// Magic Numbers
const unsigned long ONE_SECOND = 1000;
const unsigned long HALF_SECOND = 500;

// Motor Initialization
const int motorPin = 4;
bool motorIsRunning = false;

void turnMotorOn() {
  digitalWrite(motorPin, HIGH);
  motorIsRunning = true;
}

void turnMotorOff() {
  digitalWrite(motorPin, LOW);
  motorIsRunning = false;
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

void setup()
{
  Serial.begin(115200); 
  pinMode(motorPin, OUTPUT);
  pinMode(startPin, INPUT_PULLUP);
  pinMode(clearPin, INPUT_PULLUP);
  digitalWrite(motorPin, LOW); // motor off by default
}


void loop() {
  startCurrentState = digitalRead(startPin);
  clearCurrentState = digitalRead(clearPin);

  boolean startButtonPressed = (startCurrentState == LOW && startLastState == HIGH);
  boolean clearButtonPressed = (clearCurrentState == LOW && clearLastState == HIGH);


   if (!keypadInUse) {
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

    if (clearButtonPressed) {
      if (amountOfMsElapsed(lastClearPress) > HALF_SECOND) {
        if (!motorIsRunning) {
            timer = 0;
            displayedInput = "";
            Serial.println("Timer cleared");
          } else {
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
        Serial.println("Done");
      }
    }
  }

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
