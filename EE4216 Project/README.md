# EE4216 — ESP32 IoT Monitoring and Control System

An IoT project developed using ESP32-S3 boards, integrating sensor monitoring, local HTTP communication, MQTT cloud communication and remote LED control through ThingSpeak.

This repository contains my implementations of **Sensor Node 1 and the Gateway**, developed as part of NUS EE4216.

## System Overview

The system consists of two sensor nodes and a gateway. Both sensor nodes connect to the gateway's local Wi-Fi network, while the gateway connects to a phone hotspot for Internet access.

- **Sensor Node 1:** Measures distance using an HC-SR04 ultrasonic sensor and controls an 8-LED RGB bar.
- **Sensor Node 2:** Measures temperature, humidity and light intensity. Developed by my project partner.
- **Gateway:** Receives sensor readings, hosts a local web dashboard, publishes data to ThingSpeak and forwards remote LED commands.

The gateway operates in **Wi-Fi AP+STA mode**, allowing the local network to continue operating even when Internet access is unavailable.

## Communication Architecture

**Local Network — HTTP**

- Sensor nodes send readings to the gateway using HTTP POST requests containing JSON data.
- The gateway hosts a dashboard accessible through its AP IP address (`192.168.4.1`).
- The dashboard automatically retrieves updated readings through HTTP GET requests.
- The gateway forwards LED control commands to Sensor Node 1 using HTTP POST.

**Cloud Communication — MQTT**

- The gateway publishes sensor readings to ThingSpeak.
- It subscribes to remote LED control commands through ThingSpeak's MQTT broker.
- LED commands are validated and forwarded to Sensor Node 1.
- Execution feedback is published back to ThingSpeak.

MQTT communication uses TLS encryption through port 8883.

## Key Features

- Automatic distance-based RGB LED control and remote LED control.
- Sensor data validation, including handling invalid readings using JSON `null`.
- Dashboard updates every 2 seconds without manual refresh.
- Sensor-node status monitoring based on the most recent received report.
- Automatic Wi-Fi and MQTT reconnection.
- LED command validation and acknowledgment checking.
- MQTT publishing intervals to comply with ThingSpeak's update limits.

## Technologies Used

| Category | Technologies |
|---|---|
| Microcontroller | ESP32-S3 |
| Programming | C++ (Arduino framework) |
| Local Communication | Wi-Fi, HTTP, JSON |
| Cloud Communication | MQTT over TLS, ThingSpeak |
| Libraries | WiFi, WebServer, HTTPClient, ArduinoJson, PubSubClient, WiFiClientSecure, NewPing, Adafruit NeoPixel |
| Dashboard | HTML, CSS, JavaScript |

## Project Files

- `Gateway.ino` — Gateway Wi-Fi, HTTP server, MQTT communication, dashboard hosting and LED command forwarding.
- `dashboard.h` — HTML, CSS and JavaScript for the local dashboard.
- `SensorNode_1.ino` — Ultrasonic distance measurement, RGB LED control and HTTP communication.

## Notes

This project was developed for NUS EE4216 as a two-person group project. This repository focuses on my contributions to the system.

Sensor Node 2 was developed by my project partner and is not included in this repository.