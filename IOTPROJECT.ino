#include <Wire.h>
#include "rgb_lcd.h"
#include <Keypad.h>

// Creating an  object of the rgb_lcd 
rgb_lcd lcd;
int led1 = 18; // First LED
int led2 = 19; // Second LED
int pirSensorPin = 12; // PIR sensor pin
int buzzerPin = 23; // Buzzer pin

// Keypad setup
const byte ROWS = 4, COLS = 3;
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte rowPins[ROWS] = {15, 2, 0, 4};
byte colPins[COLS] = {16, 17, 5};
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);


String enteredCode = ""; //String to Store the passcode
const String correctCode = "123"; //the correct code 
bool alarmActive = false;

// LED flashing
unsigned long previousMillis = 0;
bool ledState = false;

void setup() {
  Serial.begin(9600);
  pinMode(pirSensorPin, INPUT); 
  pinMode(led1, OUTPUT);        
  pinMode(led2, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  lcd.begin(16, 2);   
  
  lcd.print("System Ready");
  delay(2000); 
  lcd.clear();
}

void loop() {
// Checks if motion is detected
int pirValue = digitalRead(pirSensorPin); 
if (pirValue == HIGH) {
  alarmActive = true;
}

if (alarmActive) {
  // when the alarm is triggered the buzzer and flash LEDs
  Serial.println("Alarm triggered. Activating buzzer...");
  tone(buzzerPin, 1000); // Sounds the buzzer
  flashLEDs(); // Function to flash LEDs
  displayMessage("Enter Passcode:");

  // Capture keypresses and check the entered code
  char customKey = customKeypad.getKey();
  if (customKey) {
    enteredCode += customKey; // Adds key pressed to the string
    lcd.clear();
    lcd.print("Code: ");
    lcd.print(enteredCode);

    // Check if the entered code is correct
    if (enteredCode == correctCode) {
      alarmActive = false;
      noTone(buzzerPin);
      enteredCode = ""; // Resets string is the code is correct 
      lcd.clear();
      lcd.print("Alarm Deactivated");
      delay(2000);
      lcd.clear();
    } else if (enteredCode.length() >= 3) {
      // If the entered code is incorrect, reset and display message
      enteredCode = "";
      lcd.clear();
      lcd.print("Wrong Passcode");
      delay(2000);
      lcd.clear();
    }
  }
} else {
  // If no motion is detected, turn off the buzzer and LEDs
  noTone(buzzerPin);
  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);
}
}

// functions
void flashLEDs() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 500) {
    previousMillis = currentMillis;
    ledState = !ledState;
    digitalWrite(led1, ledState ? HIGH : LOW);
    digitalWrite(led2, ledState ? LOW : HIGH);
  }
}

//Displays message on LCD if changed
void displayMessage(String message) {
  static String previousMessage = "";
  if (message != previousMessage) {
    lcd.clear();
    lcd.print(message);
    previousMessage = message;
  }
}
