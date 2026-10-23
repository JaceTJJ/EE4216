#include <WiFi.h>

// ESP32 Access Point details
const char* ssid = "A0302054X";
const char* password = "12345678";

void setup() {
  Serial.begin(115200);

  // Start Wi-Fi access point (max 2 connections)
  WiFi.softAP(ssid, password, 1, 0, 2); // channel, ssid_hidden, max connection

  // Print AP information
  Serial.print("AP SSID: ");
  Serial.println(ssid);

  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  Serial.print("AP MAC Address: ");
  Serial.println(WiFi.softAPmacAddress());
}

void loop() {
  // Print the number of connected devices
  int numStations = WiFi.softAPgetStationNum();

  Serial.print("Connected devices: ");
  Serial.println(numStations); // should show 0 currently

  delay(5000);
}
