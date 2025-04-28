#include <Wire.h>
#include "rgb_lcd.h"
#include <Keypad.h>
#include <dht11.h>
#include <WiFi.h>
#include <WebServer.h>

rgb_lcd lcd;
dht11 DHT;

// WiFi credentials
const char* ssid = "VODAFONE-F578";         
const char* password = "J9crHhK6x4fKyeth"; 

WebServer server(80);

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
Keypad customKeypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

String enteredCode = "";
const String correctCode = "123";
bool alarmActive = false;
unsigned long previousMillis = 0;
bool ledState = false;

float temp = 0;
float hum = 0;
float distance = 0;

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

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.begin();
}

void loop() {
  readSensors();
  checkAlarm();
  server.handleClient();
  delay(300);
}

void readSensors() {
  int chk = DHT.read(DHT11_PIN);
  if (chk == DHTLIB_OK) {
    temp = DHT.temperature;
    hum = DHT.humidity;
  }

  long duration;
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
}

void checkAlarm() {
  int pirValue = digitalRead(pirSensorPin);

  if (pirValue == HIGH || distance <= 30) {
    alarmActive = true;
  }

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
  } else {
    noTone(buzzerPin);
    digitalWrite(led1, LOW);
    digitalWrite(led2, LOW);

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
}

void flashLEDs() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 500) {
    previousMillis = currentMillis;
    ledState = !ledState;
    digitalWrite(led1, ledState ? HIGH : LOW);
    digitalWrite(led2, ledState ? LOW : HIGH);
  }
}

void displayMessage(String message) {
  static String previousMessage = "";
  if (message != previousMessage) {
    lcd.clear();
    lcd.print(message);
    previousMessage = message;
  }
}

void handleRoot() {
  String page = "<html><head><title>ESP32 Sensor</title></head><body>";
  page += "<h1>ESP32 Sensor Data</h1>";
  page += "<p>Temperature: " + String(temp) + " C</p>";
  page += "<p>Humidity: " + String(hum) + " %</p>";
  page += "<p>Distance: " + String(distance) + " cm</p>";
  page += "</body></html>";
  
  server.send(200, "text/html", page);
}
