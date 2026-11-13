#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <time.h>
#include "secrets.h"
#include "root_certs.h"
#include "dashboard.h"

// Local HTTP server and secure MQTT client.
WebServer server(80);
WiFiClientSecure secureClient;
PubSubClient mqtt(secureClient);

// MQTT configurations (8883 is port used for encrypted TLS connection)
const char *mqttHost = "mqtt3.thingspeak.com";
const int mqttPort = 8883;
const char *sensorTopic = "channels/3574449/publish";
const char *ledTopic = "channels/3574454/subscribe";
const char *feedbackTopic = "channels/3574454/publish/fields/field4";

// Latest sensor readings and whether each value is valid.
float distance = 0;
float temperature = 0;
float humidity = 0;
int light = 0;

bool temperatureValid = false;
bool humidityValid = false;
bool lightValid = false;
bool distanceValid = false;

// Track when each sensor node last reported data.
unsigned long node1LastSeen = 0;
unsigned long node2LastSeen = 0;
bool node1Received = false; // Time of last accepted sensor report, use millis()
bool node2Received = false; // Whether the gateway has ever received data from Node 1
bool node1New = false; // Whether there is new data to publish to ThingSpeak
bool node2New = false; 
time_t node1ReceivedAt = 0;
time_t node2ReceivedAt = 0;
IPAddress node1IP; // SensorNode 1's IP, used for forwarding LED commands via HTTP

// Timers and state flags for Wi-Fi, MQTT, and remote LED commands.
unsigned long lastWiFiTry = 0;
bool timeSyncStarted = false;
unsigned long lastMQTTTry = 0;
unsigned long lastPublish = 0;
unsigned long lastLedChannelWrite = 0;
int lastCommandEntry = 0;
int commandEntry = 0;
int commandIndex = 0;
int commandColor = 0;
bool commandPending = false;
bool feedbackPending = false;
int feedbackValue = 0;

// Receive and validate HTTP sensor data from either local node.
void handleSensor() // Runs when sensor node sends POST /api/sensor, defined in server.on in setup()
{
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (error || !doc["node"].is<const char *>()) {
        server.send(400, "application/json", "{\"error\":\"Invalid JSON or node\"}");
        return;
    }

    String node = doc["node"].as<String>();

     // Sensor 1 sends ultrasonic distance readings.
    if (node == "sensor01"){

        // Distance must be a number or null.
        if (doc["distance"].isUnbound() ||
            (!doc["distance"].is<float>() &&
             !doc["distance"].is<int>() &&
             !doc["distance"].isNull()))
        {
            server.send(400, "application/json", "{\"error\":\"Invalid distance\"}");
            return;
        }

        distanceValid = !doc["distance"].isNull();

        if (distanceValid) {
            float d = doc["distance"].as<float>();

            if (!isfinite(d) || d < 2 || d > 200) {
                server.send(400, "application/json", "{\"error\":\"Distance out of range\"}");
                return;
            }

            distance = d;
            Serial.printf("Sensor 1 distance: %.2f cm\n", distance);
        }
        else {
            Serial.println("Sensor 1 distance unavailable");
        }

        // Update node status even if the reading is null.
        node1IP = server.client().remoteIP();
        node1LastSeen = millis();
        node1Received = true;
        node1New = true;
        node1ReceivedAt = time(nullptr);
    }

    // Sensor 2 sends temperature, humidity, and light readings.
    else if (node == "sensor02") {

        // Check that all fields are present.
        if (!doc["temperature"].is<float>() &&
            !doc["temperature"].is<int>() &&
            !doc["temperature"].isNull()) {
            server.send(400, "application/json", "{\"error\":\"Invalid temperature\"}");
            return;
        }

        if (!doc["humidity"].is<float>() &&
            !doc["humidity"].is<int>() &&
            !doc["humidity"].isNull()) {
            server.send(400, "application/json", "{\"error\":\"Invalid humidity\"}");
            return;
        }

        if (!doc["light"].is<int>() &&
            !doc["light"].isNull()) {
            server.send(400, "application/json", "{\"error\":\"Invalid light\"}");
            return;
        }

        float t = doc["temperature"].as<float>();
        float h = doc["humidity"].as<float>();
        int l = doc["light"].as<int>();

        // Check numerical readings only.
        if ((!doc["temperature"].isNull() && (!isfinite(t) || t < -40 || t > 80)) ||
            (!doc["humidity"].isNull() && (!isfinite(h) || h < 0 || h > 100)) ||
            (!doc["light"].isNull() && (l < 0 || l > 100))) {
            server.send(400, "application/json", "{\"error\":\"Out of range\"}");
            return;
        }

        temperatureValid = !doc["temperature"].isNull();
        humidityValid = !doc["humidity"].isNull();
        lightValid = !doc["light"].isNull();

        if (temperatureValid) temperature = t;
        if (humidityValid) humidity = h;
        if (lightValid) light = l;

        // The node is online even if a sensor reading is null.
        node2LastSeen = millis();
        node2Received = true;
        node2New = true;
        node2ReceivedAt = time(nullptr);

        Serial.println("Sensor 2 readings received");
    }
    else {
        server.send(400, "application/json", "{\"error\":\"Unknown node\"}");
        return;
    }
    server.send(200, "application/json", "{\"success\":true}");
}

// Format the gateway's UTC system time for the dashboard.
String timeText(time_t value)
{
    if (value < 1600000000) return ""; // Clock has not synced yet.
    struct tm utc;
    gmtime_r(&value, &utc);
    char text[25];
    strftime(text, sizeof(text), "%Y-%m-%dT%H:%M:%SZ", &utc);
    return String(text);
}

// Return latest readings and connectivity status to the dashboard.
void handleStatus() // Supply data to dashboard
{
    JsonDocument doc;

    doc["sensor01"]["received"] = node1Received;
    doc["sensor01"]["online"] = node1Received && millis() - node1LastSeen < 9000;

    if (node1Received) {
        if (distanceValid)
            doc["sensor01"]["distance_cm"] = distance;
        else
            doc["sensor01"]["distance_cm"] = nullptr;

        doc["sensor01"]["last_received"] = timeText(node1ReceivedAt);
    }

    doc["sensor02"]["received"] = node2Received;
    doc["sensor02"]["online"] = node2Received && millis() - node2LastSeen < 9000;

    if (node2Received) {
        // Display null for invalid sensor readings.
        if (temperatureValid)
            doc["sensor02"]["temperature_c"] = temperature;
        else
            doc["sensor02"]["temperature_c"] = nullptr;

        if (humidityValid)
            doc["sensor02"]["humidity_percent"] = humidity;
        else
            doc["sensor02"]["humidity_percent"] = nullptr;

        if (lightValid)
            doc["sensor02"]["light_raw"] = light;
        else
            doc["sensor02"]["light_raw"] = nullptr;

        doc["sensor02"]["last_received"] = timeText(node2ReceivedAt);
    }

    doc["internet_wifi_connected"] = WiFi.status() == WL_CONNECTED;
    doc["mqtt_connected"] = mqtt.connected();
    doc["time_synced"] = time(nullptr) > 1600000000;

    String response;
    serializeJson(doc, response);
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", response);
}

// ThingSpeak sends a JSON feed entry when the LED channel changes.
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    if (error)
        return;
    if (doc["field1"].isNull() || doc["field2"].isNull())
        return;
    
    // Decode the cloud command and ignore old or invalid entries.
    int entry = doc["entry_id"] | 0;
    int index = String(doc["field1"].as<const char *>()).toInt();
    int color = String(doc["field2"].as<const char *>()).toInt();

    if (entry <= lastCommandEntry || entry <= 0)
        return;
    if (index < 0 || index > 7 || color < 0 || color > 3) {
        Serial.println("Invalid LED command");
        lastCommandEntry = entry;
        feedbackValue = -1;
        feedbackPending = true;
        lastLedChannelWrite = millis();
        return;
    }

    commandEntry = entry;
    commandIndex = index;
    commandColor = color;
    commandPending = true;

    lastLedChannelWrite = millis();

    Serial.printf("New LED command: index %d, color %d\n", index, color);
}

// Forward a cloud LED command to Sensor Node 1 over HTTP.
void sendLEDCommand()
{
    commandPending = false;
    lastCommandEntry = commandEntry;
    int r = 0, g = 0, b = 0;
    if (commandColor == 1) r = 255;
    if (commandColor == 2) g = 255;
    if (commandColor == 3) b = 255;

    // Do not send commands when Sensor Node 1 has stopped reporting.
    if (!node1Received || millis() - node1LastSeen >= 15000) {
        Serial.println("LED command failed: sensor 1 offline");
        feedbackValue = -1;
        feedbackPending = true;
        return;
    }

    JsonDocument doc;
    doc["index"] = commandIndex;
    doc["r"] = r;
    doc["g"] = g;
    doc["b"] = b;
    String body;
    serializeJson(doc, body);

    HTTPClient http;
    String url = "http://" + node1IP.toString() + "/api/led";

    http.begin(url);
    http.setTimeout(2000);
    http.addHeader("Content-Type", "application/json");

    int code = http.POST(body);

    String reply = http.getString();
    http.end();

    // Confirm the HTTP response before preparing MQTT feedback.
    JsonDocument ack;

    bool validAck = deserializeJson(ack, reply) == DeserializationError::Ok;
    bool success = code == 200 && validAck && (ack["success"] == true);

    feedbackValue = success ? 1 : -1;
    feedbackPending = true;

    Serial.printf("LED HTTP result: %d, success: %s\n", code, success ? "yes" : "no");
}

// Connect to the MQTT broker and subscribe to LED commands.
void connectMQTT()
{
    if (WiFi.status() != WL_CONNECTED || mqtt.connected())
        return;
    if (millis() - lastMQTTTry < 10000)
        return;
    lastMQTTTry = millis();
    Serial.println("Connecting to ThingSpeak MQTT...");

    if (mqtt.connect(mqttClientID, mqttUsername, mqttPassword)) {
        Serial.println("MQTT connected");
        if (mqtt.subscribe(ledTopic))
            Serial.println("Subscribed to LED channel");
    }
    else
        Serial.printf("MQTT connection failed, state: %d\n", mqtt.state());
}

// Periodically publish newly received sensor measurements to ThingSpeak.
void publishSensors()
{
    if (!mqtt.connected() || (!node1New && !node2New))
        return;
    if (millis() - lastPublish < 20000)
        return; // ThingSpeak free update interval.
    lastPublish = millis();

    // Publish only measurements received since the previous successful publish.
    String data = "";

    if (node1New && distanceValid)
        data += "field1=" + String(distance, 1);

    if (node2New) {
        if (temperatureValid) {
            if (data.length())
                data += "&";
            data += "field2=" + String(temperature, 1);
        }
        if (humidityValid) {
            if (data.length())
                data += "&";
            data += "field3=" + String(humidity, 1);
        }
        if (lightValid) {
            if (data.length())
                data += "&";
            data += "field4=" + String(light);
        }
    }
    if (mqtt.publish(sensorTopic, data.c_str())) {
        Serial.println("MQTT sensor data sent: " + data);
        node1New = false;
        node2New = false;
    }
    else
        Serial.println("MQTT sensor publish failed");
}

// Publish the LED execution result back to ThingSpeak.
void publishFeedback()
{
    if (!feedbackPending || !mqtt.connected())
        return;

    // The LED channel also has a 15-second minimum update interval.
    if (millis() - lastLedChannelWrite < 16000)
        return;
    String value = String(feedbackValue);

    if (mqtt.publish(feedbackTopic, value.c_str())) {
        Serial.println("MQTT feedback sent: " + value);
        feedbackPending = false;
        lastLedChannelWrite = millis();
    }
}

// Configure gateway AP+STA Wi-Fi, secure MQTT, and HTTP endpoints.
void setup()
{
    Serial.begin(115200);
    delay(500);
    WiFi.mode(WIFI_AP_STA);

    Serial.print("Gateway STA IP: ");
    Serial.println(WiFi.localIP());

    // Create local AP
    WiFi.softAP(apSSID, apPassword);
    Serial.print("Gateway AP IP: ");
    Serial.println(WiFi.softAPIP());

    // Connect to Wifi in STA mode
    WiFi.begin(wifiSSID, wifiPassword);
    Serial.println("Connecting gateway to phone hotspot...");

    // Check the broker certificate rather than skipping TLS verification.

    // Confdigure TLS
    secureClient.setCACert(mqttRootCerts); // configures cert verification for TLS connection
    secureClient.setTimeout(5);

    // Configure MQTT
    mqtt.setServer(mqttHost, mqttPort); // specifies ThingSpeak's MQTT broker
    mqtt.setCallback(mqttCallback); // registers the function to handle incoming MQTT messages
    mqtt.setBufferSize(512);
    mqtt.setSocketTimeout(5);

    // Register sensor upload, dashboard status, and landing-page routes.
    // Register HTTP endpoints
    server.on("/api/sensor", HTTP_POST, handleSensor);
    server.on("/api/status", HTTP_GET, handleStatus);
    server.on("/", HTTP_GET, []()
            { server.send_P(200, "text/html", DASHBOARD_PAGE); });

    server.begin(); // Start the server
    Serial.println("HTTP server started");
}

// Keep the local server responsive and service cloud communication.
void loop()
{
    server.handleClient();

    // Retry upstream Wi-Fi periodically without an endless wait loop.
    if (WiFi.status() != WL_CONNECTED && millis() - lastWiFiTry >= 15000) {
        lastWiFiTry = millis();
        WiFi.reconnect();
        Serial.println("Retrying phone hotspot...");
    }

    // Get system time from NTP when Internet Wi-Fi is available.
    if (WiFi.status() == WL_CONNECTED && !timeSyncStarted) {
        Serial.print("Gateway STA IP: ");
        Serial.println(WiFi.localIP());

        configTime(0, 0, "pool.ntp.org", "time.google.com"); // UTC
        timeSyncStarted = true;
        Serial.println("NTP time synchronization started");
    }

    // A valid clock is also needed for TLS certificate checks.
    if (time(nullptr) > 1600000000)
        connectMQTT();
    // Process incoming MQTT messages and any pending outgoing updates.
    if (mqtt.connected())
        mqtt.loop();
    if (commandPending)
        sendLEDCommand();

    publishSensors();
    publishFeedback();
}
