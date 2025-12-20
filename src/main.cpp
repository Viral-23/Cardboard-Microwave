#include <Arduino.h>
#include <Keypad.h>

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
bool keypadActive = false;

String input = "";
const int motorPin = 4;

const int startPin = 18;
int startLastState = LOW;
int startCurrentState;
unsigned long lastStartPress = 0;

const int clearPin = 19;
int clearLastState = LOW;
int clearCurrentState;
unsigned long lastClearPress = 0;

const unsigned long debounceDelay = 500; // 100ms

// bool keyIsActive = false;
bool isMotorRunning = false;

int timer = 0;
bool secondClick = false;
unsigned long lastTick = 0;
// const unsigned long keyLock = 1000; // 0.5s
unsigned long lastKeyPress = 0;



void setup()
{
  Serial.begin(115200); 
  pinMode(motorPin, OUTPUT);
  pinMode(startPin, INPUT_PULLUP);
  pinMode(clearPin, INPUT_PULLUP);
  digitalWrite(motorPin, LOW); // motor off by default
}


void loop() {
  // OLD CODE FOR SERIAL MONITOR INPUT ACTIVATION
  // send data only when you receive data:
  // while (Serial.available() > 0) {
  //   read the incoming bytes:
  //   char c = Serial.read();
  //   if (c == '\r') {
  //     continue;  // ignore carriage return
  //   }
  //   Serial.print(c);
  //   if (c == '\n')
  //   {
  //     // say what you got:
  //     Serial.print("Input confirmed: ");
  //     Serial.println(input);
  //     if (input == "start") {
  //       digitalWrite(motorPin, HIGH);
  //       Serial.println("Motor ON");
  //     }
  //     else if (input == "stop") {
  //       digitalWrite(motorPin, LOW);
  //       Serial.println("Motor OFF");
  //     }
  //     else {
  //         Serial.println("Invalid Input");
  //     }
  //     input = "";
  //   }
  //   else {
  //     input += c;
  //   }
  // }

  // start and stop/clear microwave logic
  startCurrentState = digitalRead(startPin);
  clearCurrentState = digitalRead(clearPin);

   if (!keypadActive) {
    // If Start Button Pressed Then
    if (startCurrentState == LOW && startLastState == HIGH) {
      if (millis() - lastStartPress > debounceDelay) {
        // insert code here
        if (!isMotorRunning) {
          timer = (input.toInt() / 100) * 60 + (input.toInt() % 100);
          Serial.println("Timer is: " + String(timer));
          if (timer > 0) {
            input = String(min(input.toInt(), 9959L));
            digitalWrite(motorPin, HIGH);
            isMotorRunning = true;
            lastTick = millis();
          }
        }

        // blocking technique that doesnt work
        // while (timer > 0)
        // {
        //   delay(1000);
        //   timer -= 1;
        //   Serial.println("Time remaining: " + timer);
        // }
        lastStartPress = millis();
      }
    }

    // If Clear Button Pressed Then
    if (clearCurrentState == LOW && clearLastState == HIGH) {
      if (millis() - lastClearPress > debounceDelay) {
        // insert code here
        if (!isMotorRunning) {
            timer = 0;
            input = "";
            // secondClick = false;
            Serial.println("Timer cleared");
          } else {
              // First click: store remaining time as input
              digitalWrite(motorPin, LOW);
              isMotorRunning = false;
              // if (timer > 0) {
              //     input = String(timer / 60) + ((timer % 60 < 10) ? "0" : "") + String(timer % 60);
              // } else {
              //     input = "";
              // }
                input = String((String(timer / 60) + ((timer % 60 < 10) ? "0" : "") + String(timer % 60)).toInt()); 
                Serial.println("Input is: " + input);
                Serial.println("Timer stopped: " + String(timer));
            }

        lastClearPress = millis();
        // input = (String(timer / 60)) + String((timer % 60) < 10 ? "0" : "") + String(timer % 60);

      }
    }
  }
    startLastState = startCurrentState;
    clearLastState = clearCurrentState;

  if (isMotorRunning && timer > 0) {
    if (millis() - lastTick >= 1000) {
      lastTick = millis();
      timer--;

      Serial.print("Time remaining: ");
      Serial.println(timer);

      if (timer == 0) {
        digitalWrite(motorPin, LOW);
        isMotorRunning = false;
        Serial.println("Done");
      }
    }
  }




char num = numPad.getKey();

if (num != NO_KEY) {
  keypadActive = true;

  if (!isMotorRunning) {
    if (input.length() < 4 && !(input.length() == 0 && num == '0')) {
      input += num;
    }
    Serial.println("The current input is: " + input);
  }
} else {
  keypadActive = false;
}
  // if (startCurrentState == HIGH && clearCurrentState == HIGH && numPad.getKeys() == 0) {
  //   keyIsActive = false;
  // }

}
