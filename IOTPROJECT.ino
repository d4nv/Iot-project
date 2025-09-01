#include <Wire.h>
#include "rgb_lcd.h"
#include <Keypad.h>
#include <dht11.h>
#include <WiFi.h>
#include <WebServer.h>

rgb_lcd lcd;
dht11 DHT;

const char* ssid     = "iPhone";      
const char* password = "Daniel11";  

WebServer server(80);

#define PIR_PIN    12
#define BUZ_PIN    23
#define DHT11_PIN  27



const byte ROWS = 4, COLS = 3;
char hexaKeys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

byte rowPins[ROWS] = {15, 2, 0, 4};
byte colPins[COLS] = {16, 17, 5};
Keypad customKeypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);


String enteredCode = "";
const String correctCode = "123";
bool alarmActive = false;

float temp = 0;
float hum  = 0;

void setup() {
  Serial.begin(9600);

  pinMode(PIR_PIN, INPUT);
  pinMode(BUZ_PIN, OUTPUT);

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
  delay(200);
}

void readSensors() {
  int chk = DHT.read(DHT11_PIN);
  if (chk == DHTLIB_OK) {
    temp = DHT.temperature;
    hum  = DHT.humidity;
  }
}

void checkAlarm() {
  int pirValue = digitalRead(PIR_PIN);

  if (pirValue == HIGH) {
    alarmActive = true;
  }

  if (alarmActive) {
    tone(BUZ_PIN, 1000);
    displayMessage("Enter Passcode:");

    char customKey = customKeypad.getKey();
    if (customKey) {
      enteredCode += customKey;
      lcd.clear();
      lcd.print("Code: ");
      lcd.print(enteredCode);

      if (enteredCode == correctCode) {
        alarmActive = false;
        noTone(BUZ_PIN);
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
    noTone(BUZ_PIN);

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

  if (alarmActive) {
    page += "<p style='color:red;font-weight:bold;'>ALARM ACTIVE!</p>";
  } else {
    page += "<p style='color:green;'>System Normal</p>";
  }

  page += "</div></body></html>";
  server.send(200, "text/html", page);
}
