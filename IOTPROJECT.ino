#include <Wire.h>
#include "rgb_lcd.h"
#include <Keypad.h>
#include <dht11.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>  // For sending data to ThingSpeak

rgb_lcd lcd;
dht11 DHT;

const char* ssid = "VODAFONE-F578";
const char* password = "J9crHhK6x4fKyeth";

// Replace with your ThingSpeak Write API Key
const String apiKey = "DOCZWGRUNT3RW3N9";
const String thingSpeakURL = "http://api.thingspeak.com/update";

WebServer server(80);

#define led1 18
#define led2 19
#define pirSensorPin 12
#define buzzerPin 23
#define DHT11_PIN 27
#define trigPin 32
#define echoPin 33

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
bool systemTriggered = false;

float temp = 0;
float hum = 0;
float distance = 0;
unsigned long lastThingSpeakUpdate = 0;

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
  updateThingSpeak();
  server.handleClient();
  delay(1000);
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
    systemTriggered = true;
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
  String page = "<html><head><meta http-equiv='refresh' content='1'><title>Home Security Alarm System</title>";
  page += "<style>body{font-family:sans-serif;background:#f4f4f4;margin:20px;}h1{color:#333;}div.card{padding:15px;background:white;border-radius:10px;box-shadow:0 0 10px #ccc;}p{font-size:18px;}</style></head><body>";
  page += "<h1>Home Security Alarm System</h1><div class='card'>";
  page += "<p><strong>Temperature:</strong> " + String(temp) + " &deg;C</p>";
  page += "<p><strong>Humidity:</strong> " + String(hum) + " %</p>";
  page += "<p><strong>Distance:</strong> " + String(distance) + " cm</p>";

  if (alarmActive) {
    page += "<p style='color:red;font-weight:bold;'>ALARM ACTIVE!</p>";
  } else if (systemTriggered) {
    page += "<p style='color:orange;'>System Triggered - Awaiting Reset</p>";
  } else {
    page += "<p style='color:green;'>System Normal</p>";
  }

  page += "</div></body></html>";
  server.send(200, "text/html", page);
}

void updateThingSpeak() {
  if (millis() - lastThingSpeakUpdate > 15000) {  
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      String url = thingSpeakURL + "?api_key=" + apiKey +
                   "&field1=" + String(temp) +
                   "&field2=" + String(hum) +
                   "&field3=" + String(distance);

      http.begin(url);
      int httpCode = http.GET();
      http.end();

      Serial.print("ThingSpeak update code: ");
      Serial.println(httpCode);
      lastThingSpeakUpdate = millis();
    }
  }
}
