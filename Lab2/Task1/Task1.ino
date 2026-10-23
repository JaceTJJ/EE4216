#include <WiFi.h>

// Phone hotspot details
const char* ssid = "TP-Link_6FCB";
const char* password = "72972786";

void setup() {
  Serial.begin(115200);

  // Set ESP32 to Station mode
  WiFi.mode(WIFI_STA);

  // Connect to phone hotspot
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");

  // See how long it takes to connect
  int time = 0;

  // Wait up to 15 seconds max
  while (WiFi.status() != WL_CONNECTED && time < 15) {
    delay(1000);
    Serial.print(".");
    time++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");S
    Serial.print("Network SSID: ");
    Serial.println(WiFi.SSID()); // Print network SSID
    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI()); // Print the RSSI
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP()); // Print the IP address in STA mode
    Serial.print("MAC Address: ");
    Serial.println(WiFi.macAddress()); // Print the MAC address in STA mode
    Serial.printf("Time to connect: %d sec\n", time); // Print how long it took to connect
    Serial.print(WiFi.status()); // prints '3' which is WL_CONNECTED
  } 
  else {
    Serial.println("\nWiFi Connection Failed!");
  }
}

void loop() {

}
