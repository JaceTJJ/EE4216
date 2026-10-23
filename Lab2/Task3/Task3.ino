#include <WiFi.h>
#include <WebServer.h>
#include "lab_page.h"
#include <NewPing.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

// Pins used for Lab 1 hardware
#define TRIG_PIN 5
#define ECHO_PIN 18
#define MAX_DISTANCE 200

#define PIN 6
#define NUMPIXELS 8

NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);
Adafruit_NeoPixel strip(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// Wi-Fi AP settings
const char* ssid = "A0302054X";
const char* password = "12345678";

// HTTP server on port 80
WebServer server(80);

// Store the colour and brightness of each LED
struct LEDState {
  int r;
  int g;
  int b;
  int brightness;
};

// All LEDs start with values of 0 (off)
LEDState leds[NUMPIXELS] = {};

// Update the actual LED bar using the stored values
void updateLEDs() {
  for (int i = 0; i < NUMPIXELS; i++) {
    // Scale RGB values according to each LED's brightness
    int r = leds[i].r * leds[i].brightness / 255;
    int g = leds[i].g * leds[i].brightness / 255;
    int b = leds[i].b * leds[i].brightness / 255;

    strip.setPixelColor(i, strip.Color(r, g, b));
  }

  strip.show(); // Send updated colours to LED bar
}

void setup() {
  Serial.begin(115200);

  // Initialize Lab 1 hardware
  strip.begin();
  strip.clear();
  strip.show();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Start ESP32 in AP mode (max 2 devices)
  bool apStarted = WiFi.softAP(ssid, password, 1, 0, 2);

  if (!apStarted) {
    Serial.println("Failed to start Wi-Fi AP");
    return;
  }

  Serial.print("AP SSID: ");
  Serial.println(ssid);
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("AP MAC Address: ");
  Serial.println(WiFi.softAPmacAddress());

  // ----- Register HTTP request handlers
  // GET request: Land on the IoT dashboard webpage
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", LAB_PAGE);
  });

  // GET /api/distance: Take a new ultrasonic reading
  server.on("/api/distance", HTTP_GET, []() {
    unsigned int echoUs = sonar.ping_median(5);

    // Do not cache the distance response
    server.sendHeader("Cache-Control", "no-store");

    if (echoUs == 0) {
      // No echo detected
      server.send(200, "application/json", "{\"distance_cm\":null,\"valid\":false}");
    }
    else {
      float distance = sonar.convert_cm(echoUs);

      // Send distance to dashboard as JSON
      String response = "{\"distance_cm\":" + String(distance, 1) + ",\"valid\":true}";

      server.send(200, "application/json", response);
    }
  });

  // GET /api/leds : Return the current settings of all 8 LEDs
  server.on("/api/leds", HTTP_GET, []() {
    String response = "{\"leds\":[";

    for (int i = 0; i < NUMPIXELS; i++) {

      // Separate each LED object with a comma
      if (i > 0) response += ",";

      response += "{\"id\":" + String(i);
      response += ",\"r\":" + String(leds[i].r);
      response += ",\"g\":" + String(leds[i].g);
      response += ",\"b\":" + String(leds[i].b);
      response += ",\"brightness\":" + String(leds[i].brightness);
      response += "}";
    }

    response += "]}";

    server.send(200, "application/json", response);
  });

  // PUT request /api/leds/ : Change one LED's settings
  // onNotFound handles the different LED URLs (0 to 7)
  server.onNotFound([]() {
    // Check whether this is an LED PUT request
    if (server.method() != HTTP_PUT || !server.uri().startsWith("/api/leds/")) {
      server.send(404, "application/json", "{\"error\":\"Endpoint not found\"}");
      return;
    }

    // Extract LED number from URL, e.g. /api/leds/3
    String idString = server.uri().substring(10);
    int id = idString.toInt();

    // Only LED IDs 0 to 7 are allowed
    if (id < 0 || id >= NUMPIXELS || idString != String(id)) {
      server.send(404, "application/json", "{\"error\":\"Invalid LED ID\"}");
      return;
    }

    // Read JSON data sent by the dashboard
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));

    if (error || !doc.is<JsonObject>()) {
      server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }

    // Check that all 4 fields are integers from 0 to 255
    const char* fields[] = {"r", "g", "b", "brightness"};
    int values[4];

    for (int i = 0; i < 4; i++) {
      JsonVariant v = doc[fields[i]];

      if (!v.is<int>()) {
        server.send(400, "application/json", "{\"error\":\"Missing or invalid field\"}");
        return;
      }

      values[i] = v.as<int>();

      if (values[i] < 0 || values[i] > 255) {
        server.send(400, "application/json", "{\"error\":\"Value out of range\"}");
        return;
      }
    }

    // All values are valid, so save the selected LED's settings
    leds[id].r = values[0];
    leds[id].g = values[1];
    leds[id].b = values[2];
    leds[id].brightness = values[3];

    // Apply the stored settings to the physical LEDs
    updateLEDs();

    // Send updated LED state back to dashboard
    String response = "{\"id\":" + String(id);
    response += ",\"r\":" + String(leds[id].r);
    response += ",\"g\":" + String(leds[id].g);
    response += ",\"b\":" + String(leds[id].b);
    response += ",\"brightness\":" + String(leds[id].brightness);
    response += "}";

    server.send(200, "application/json", response);
  });

  // Start HTTP server after registering handlers
  server.begin();
  Serial.println("HTTP server started!");
}


void loop() {
  // Check and handle incoming HTTP requests
  server.handleClient();
}
