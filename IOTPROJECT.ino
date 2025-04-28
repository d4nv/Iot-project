#include <Wire.h>
#include "rgb_lcd.h"
#include <Keypad.h>
#include <dht11.h>

// Objects
rgb_lcd lcd;
dht11 DHT;

// Pins
#define led1 18
#define led2 19
#define pirSensorPin 12
#define buzzerPin 23
#define DHT11_PIN 27

#define trigPin 32
#define echoPin 33

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

// Variables
String enteredCode = "";
const String correctCode = "123";
bool alarmActive = false;
unsigned long previousMillis = 0;
bool ledState = false;

void setup() {
  Serial.begin(9600);

  pinMode(pirSensorPin, INPUT);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  lcd.begin(16, 2);
  lcd.print("System Ready");
  delay(2000);
  lcd.clear();
}

void loop() {
  // Read DHT11
  int chk = DHT.read(DHT11_PIN);
  float temp = DHT.temperature;
  float hum = DHT.humidity;

  // Read Ultrasonic
  long duration;
  float distance;

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = (duration * 0.0343) / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Check PIR or Distance to trigger alarm
  int pirValue = digitalRead(pirSensorPin);
  if (pirValue == HIGH || distance <= 30) {  // 🚨 TRIGGER if motion OR object closer than 30cm
    alarmActive = true;
  }

  // If ALARM is ACTIVE
  if (alarmActive) {
    tone(buzzerPin, 1000);
    flashLEDs();
    displayMessage("Enter Passcode:");

    char customKey = customKeypad.getKey();
    if (customKey) {
      enteredCode += customKey;
      lcd.clear();
      lcd.print("Code: ");
      lcd.print(enteredCode);

      if (enteredCode == correctCode) {
        alarmActive = false;
        noTone(buzzerPin);
        enteredCode = "";
        lcd.clear();
        lcd.print("Alarm Deactivated");
        delay(2000);
        lcd.clear();
      } else if (enteredCode.length() >= 3) {
        enteredCode = "";
        lcd.clear();
        lcd.print("Wrong Passcode");
        delay(2000);
        lcd.clear();
      }
    }
  } 
  // If ALARM is NOT ACTIVE
  else {
    noTone(buzzerPin);
    digitalWrite(led1, LOW);
    digitalWrite(led2, LOW);

    // Display Temp & Humidity when normal
    lcd.setCursor(0, 0);
    lcd.print("Temp:");
    lcd.print(temp, 0);
    lcd.print((char)223);
    lcd.print("C   ");

    lcd.setCursor(0, 1);
    lcd.print("Hum:");
    lcd.print(hum, 0);
    lcd.print("%   ");
  }

  delay(300); // Small delay to keep it smooth
}

// Flash LEDs function
void flashLEDs() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 500) {
    previousMillis = currentMillis;
    ledState = !ledState;
    digitalWrite(led1, ledState ? HIGH : LOW);
    digitalWrite(led2, ledState ? LOW : HIGH);
  }
}

// Display message function
void displayMessage(String message) {
  static String previousMessage = "";
  if (message != previousMessage) {
    lcd.clear();
    lcd.print(message);
    previousMessage = message;
  }
}
