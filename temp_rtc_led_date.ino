#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <RTClib.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>  // Biblioteca pentru SPIFFS

// Configurarea senzorului DHT11
#define DHTPIN 14  // GPIO14 (D5)
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Configurarea modulului RTC
RTC_DS1307 rtc;

// Configurarea LED-urilor
#define GREEN_LED_PIN 12  // GPIO12
#define RED_LED_PIN 13    // GPIO13

// Configurarea punctului de acces și a serverului web
const char* ssid = "Room-Temp&Umidity";
const char* password = "12345678";
ESP8266WebServer server(80);

float maxTemp = -1000.0;
float minTemp = 1000.0;
float maxHumi = -1000.0;
float minHumi = 1000.0;

unsigned long previousMillis = 0;
const long interval = 30000;  // Interval de 30 de secunde
const long saveInterval = 1800000;  // Interval de 30 minute
unsigned long previousSaveMillis = 0;

void setup() {
  // Inițializează comunicarea I2C
  Wire.begin(4, 5); // GPIO4 (SDA), GPIO5 (SCL)

  // Inițializează senzorul DHT11
  dht.begin();

  // Inițializează modulul RTC
  if (!rtc.begin()) {
    while (1);
  }

  // Eliminați această linie:
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // Configurarea pinurilor LED-urilor
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);

  // Configurarea punctului de acces
  WiFi.softAP(ssid, password);

  // Configurarea serverului web
  server.on("/", handleRoot);
  server.on("/download", handleDownload);
  server.on("/delete", handleDelete);  // Adăugați ruta pentru ștergerea datelor
  server.on("/setDateTime", handleSetDateTime);  // Adăugați ruta pentru setarea datei și orei
  server.begin();

  // Inițializează SPIFFS
  if (!SPIFFS.begin()) {
    while (1);
  }
}

void loop() {
  server.handleClient();

  unsigned long currentMillis = millis();

  // Realizează măsurători la fiecare interval
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float temp = dht.readTemperature();
    float humi = dht.readHumidity();

    // Verifică dacă citirea a reușit
    if (isnan(temp) || isnan(humi)) {
      digitalWrite(RED_LED_PIN, HIGH);
      delay(500);
      digitalWrite(RED_LED_PIN, LOW);
      delay(500);
    } else {
      digitalWrite(GREEN_LED_PIN, HIGH);
      delay(500);
      digitalWrite(GREEN_LED_PIN, LOW);
      delay(500);

      // Actualizează temperatura și umiditatea maximă și minimă
      if (temp > maxTemp) maxTemp = temp;
      if (temp < minTemp) minTemp = temp;
      if (humi > maxHumi) maxHumi = humi;
      if (humi < minHumi) minHumi = humi;
    }

    // Salvează datele la fiecare 30 minute
    if (currentMillis - previousSaveMillis >= saveInterval) {
      previousSaveMillis = currentMillis;
      saveData(temp, humi);
    }
  }

  // Comentați sau ștergeți această linie
  // checkDataAge();
}

void handleRoot() {
  DateTime now = rtc.now();
  char dateStr[11];
  char timeStr[6];
  snprintf(dateStr, sizeof(dateStr), "%02d/%02d/%04d", now.day(), now.month(), now.year());
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d", now.hour(), now.minute());

  String html = "<html><head>";
  html += "<style>";
  html += "body { background-color: #f0f8ff; color: #333333; font-family: Arial, sans-serif; }";
  html += "h1 { color: #0066cc; }";
  html += "p { color: #333333; }";
  html += "</style>";
  html += "</head><body>";
  html += "<h1>Room-Temp&Umidity Web Server</h1>";
  html += "<p>Datum: " + String(dateStr) + "</p>";
  html += "<p>Zeit: " + String(timeStr) + "</p>";
  html += "<p>Temperatur: " + String(dht.readTemperature()) + "C</p>";
  html += "<p>Feuchtigkeit: " + String(dht.readHumidity()) + "%</p>";
  html += "<br><br>";  // Adăugați 2 rânduri libere
  html += "<p>Maximaltemperatur: " + String(maxTemp) + "C</p>";
  html += "<p>Mindesttemperatur: " + String(minTemp) + "C</p>";
  html += "<br><br>";  // Adăugați 2 rânduri libere
  html += "<p>Maximale Luftfeuchtigkeit: " + String(maxHumi) + "%</p>";
  html += "<p>Mindest Luftfeuchtigkeit: " + String(minHumi) + "%</p>";
  html += "<br><br>";  // Adăugați 2 rânduri libere
  html += "<a href=\"/download\">Download Data</a>";
  html += "<button onclick=\"deleteData()\">Delete Data</button>";  // Adăugați butonul pentru ștergerea datelor
  html += "<br><br><br><br>";  // Adăugați 4 rânduri libere
  html += "<form action=\"/setDateTime\" method=\"GET\">";
  html += "<label for=\"date\">Data (DD/MM/YYYY):</label>";
  html += "<input type=\"text\" id=\"date\" name=\"date\"><br>";
  html += "<label for=\"time\">Ora (HH:MM):</label>";
  html += "<input type=\"text\" id=\"time\" name=\"time\"><br>";
  html += "<input type=\"submit\" value=\"Set Date and Time\">";
  html += "</form>";
  html += "<script>";
  html += "function deleteData() {";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('GET', '/delete', true);";
  html += "  xhr.send();";
  html += "}";
  html += "</script>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleDownload() {
  File file = SPIFFS.open("/data.txt", "r");
  if (!file) {
    server.send(500, "text/plain", "Failed to open file for reading");
    return;
  }

  server.streamFile(file, "text/plain");
  file.close();
}

void handleDelete() {
  SPIFFS.remove("/data.txt");
  server.send(200, "text/plain", "Data deleted");
}

void handleSetDateTime() {
  if (server.hasArg("date") && server.hasArg("time")) {
    String date = server.arg("date");
    String time = server.arg("time");

    int day = date.substring(0, 2).toInt();
    int month = date.substring(3, 5).toInt();
    int year = date.substring(6, 10).toInt();
    int hour = time.substring(0, 2).toInt();
    int minute = time.substring(3, 5).toInt();

    rtc.adjust(DateTime(year, month, day, hour, minute, 0));
    server.send(200, "text/plain", "Date and time updated");
  } else {
    server.send(400, "text/plain", "Invalid request");
  }
}

void saveData(float temp, float humi) {
  File file = SPIFFS.open("/data.txt", "a");
  if (!file) {
    return;
  }

  DateTime now = rtc.now();
  char dateStr[11];
  char timeStr[6];
  snprintf(dateStr, sizeof(dateStr), "%02d/%02d/%04d", now.day(), now.month(), now.year());
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d", now.hour(), now.minute());

  file.print("Data: ");
  file.print(dateStr);
  file.print(" Ora: ");
  file.print(timeStr);
  file.print(" Temperatura: ");
  file.print(temp);
  file.print("C Umiditate: ");
  file.print(humi);
  file.println("%");

  file.close();
}
