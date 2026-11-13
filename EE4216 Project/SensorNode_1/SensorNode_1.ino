#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <NewPing.h>
#include <Adafruit_NeoPixel.h>

#define PIN 6
#define NUMPIXELS 8
#define TRIG_PIN 5
#define ECHO_PIN 18
#define MAX_DISTANCE 200

// Connect Sensor Node 1 to the gateway AP and its sensor API.
const char* ssid = "EE4216_Gateway";
const char* password = "password123gateway";
const char* gatewayURL = "http://192.168.4.1/api/sensor";

// Sensor, LED strip, and local HTTP server objects.
NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);
Adafruit_NeoPixel strip(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
WebServer server(80);

// Track periodic sensor readings, Wi-Fi retries, and LED control mode.
unsigned long lastReading = 0;
unsigned long lastWiFiTry = 0;
bool remoteControl = false;

// Accept and validate LED commands forwarded by the gateway.
void handleLED() {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (error || !doc["index"].is<int>() || !doc["r"].is<int>() ||
        !doc["g"].is<int>() || !doc["b"].is<int>()) {
        server.send(400, "application/json", "{\"error\":\"Invalid JSON fields\"}");
        return;
    }

    int index = doc["index"];
    int r = doc["r"];
    int g = doc["g"];
    int b = doc["b"];
    if (index < 0 || index >= NUMPIXELS || r < 0 || r > 255 ||
        g < 0 || g > 255 || b < 0 || b > 255) {
        server.send(400, "application/json", "{\"error\":\"Value out of range\"}");
        return;
    }

    // Keep remotely selected LED colors instead of automatic distance colors.
    remoteControl = true;
    strip.setPixelColor(index, strip.Color(r, g, b));
    strip.show();
    Serial.printf("LED %d updated to (%d,%d,%d)\n", index, r, g, b);
    server.send(200, "application/json", "{\"success\":true}");
}

// Measure distance, update local LEDs, and report to the gateway.
void readDistanceAndSend() {
    unsigned int echoUs = sonar.ping_median(5);
    float distance = 0;
    bool distanceValid = false;

    // Check whether a valid echo was received.
    if (echoUs > 0) {
        distance = sonar.convert_cm(echoUs);

        if (distance >= 2 && distance <= MAX_DISTANCE) {
            distanceValid = true;
        }
    }

    if (distanceValid) {
        Serial.printf("Distance: %.2f cm\n", distance);

        // Original Lab 1 distance-based LED display.
        if (!remoteControl) {
            int leds_to_turn_on;
            uint32_t color;

            // Select LED color based on distance.
            if (distance < 30)
                color = strip.Color(128, 0, 0);
            else if (distance <= 70)
                color = strip.Color(128, 128, 0);
            else
                color = strip.Color(0, 128, 0);

            // Illuminate more LEDs when the obstacle is closer.
            if (distance <= 10)
                leds_to_turn_on = 8;
            else if (distance >= 100)
                leds_to_turn_on = 1;
            else {
                float x = (distance - 10) / 90.0 * 7;
                leds_to_turn_on = 8 - round(x);
            }

            strip.clear();
            for (int i = 0; i < leds_to_turn_on; i++) {
                strip.setPixelColor(i, color);
            }
            strip.show();
        }
    }
    else {
        Serial.println("Invalid distance reading");

        if (!remoteControl) {
            strip.clear();
            strip.show();
        }
    }

    // Send the reading to the gateway, including null if invalid.
    // Send the latest reading as JSON via HTTP POST.
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(gatewayURL);
        http.setTimeout(2000);
        http.addHeader("Content-Type", "application/json");

        JsonDocument doc; // creates an empty JSON document named doc
        doc["node"] = "sensor01"; // create a key called 'node' and assigns a value of 'sensor01'

        if (distanceValid)
            doc["distance"] = distance; // create a key called 'distance' and assigns a distance value
        else
            doc["distance"] = nullptr;

        String body;
        serializeJson(doc, body);

        int code = http.POST(body);
        Serial.printf("Gateway HTTP response: %d\n", code);
        http.end();
    }
}

// Initialize the sensor pins, LED strip, Wi-Fi, and LED API.
void setup() {
    Serial.begin(115200);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    strip.begin();
    strip.setBrightness(10);
    strip.show();

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("Connecting to gateway Wi-Fi...");

    server.on("/api/led", HTTP_POST, handleLED);
    server.begin();
}

// Handle LED requests, Wi-Fi recovery, and five-second sensor updates.
void loop() {
    server.handleClient();

    // Retry Wi-Fi without blocking the main loop indefinitely.
    if (WiFi.status() != WL_CONNECTED && millis() - lastWiFiTry >= 3000) {
        lastWiFiTry = millis();
        WiFi.reconnect();
        Serial.println("Retrying Wi-Fi...");
    }

    if (millis() - lastReading >= 5000) {
        lastReading = millis();
        readDistanceAndSend();
    }
}
