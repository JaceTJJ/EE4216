#ifndef DASHBOARD_H
#define DASHBOARD_H

// Simple webpage served by the gateway.
const char DASHBOARD_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>EE4216 IoT Dashboard</title>
  <style>
    body { font-family: Arial, sans-serif; background: #f4f6f8; color: #222; margin: 0; }
    main { max-width: 850px; margin: 35px auto; padding: 0 16px; }
    h1 { margin-bottom: 6px; }
    .subtitle { color: #666; margin-top: 0; }
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 16px; }
    .card { background: white; border: 1px solid #ddd; border-radius: 8px; padding: 20px; }
    .value { font-size: 30px; font-weight: bold; margin: 12px 0; }
    .status { font-weight: bold; }
    .online { color: #167a45; }
    .offline { color: #b33b32; }
    .note { color: #666; font-size: 13px; }
  </style>
</head>
<body>
<main>
  <h1>EE4216 IoT Dashboard</h1>
  <p class="subtitle">Live readings from the ESP32-S3 gateway</p>
  <div class="grid">
    <section class="card">
      <h2>Sensor Node 1</h2>
      <p>Ultrasonic sensor</p>
      <div id="distance" class="value">-- cm</div>
      <p>Status: <span id="node1" class="status">Checking...</span></p>
      <p class="note">Last received: <span id="node1time">--</span></p>
    </section>
    <section class="card">
      <h2>Sensor Node 2</h2>
      <p>DHT22 temperature, humidity and light sensor</p>
      <div id="temperature" class="value">-- °C</div>
      <div id="humidity" class="value">-- %</div>
      <div id="light" class="value">--</div>
      <p class="note">Light sensor (raw ADC: 0–4095)</p>
      <p>Status: <span id="node2" class="status">Checking...</span></p>
      <p class="note">Last received: <span id="node2time">--</span></p>
    </section>
    <section class="card">
      <h2>Gateway Status</h2>
      <p>Phone hotspot: <span id="wifi" class="status">Checking...</span></p>
      <p>ThingSpeak MQTT: <span id="mqtt" class="status">Checking...</span></p>
      <p class="note">The dashboard remains available on the local Wi-Fi when Internet is disconnected.</p>
    </section>
  </div>
  <p class="note" id="updated">Waiting for readings...</p>
</main>
<script>
  // Request the latest data from the gateway every 2 seconds.
  function showStatus(id, connected) {
    const element = document.getElementById(id);
    element.textContent = connected ? 'Online' : 'Offline';
    element.className = 'status ' + (connected ? 'online' : 'offline');
  }

  // Convert UTC time from the gateway to the browser's local time.
  function displayTime(value) {
    return value ? new Date(value).toLocaleString() : 'Time not synced yet';
  }

  async function updateDashboard() {
    try {
      const response = await fetch('/api/status', {cache: 'no-store'});
      if (!response.ok) throw new Error('HTTP ' + response.status);
      const data = await response.json();

      showStatus('node1', data.sensor01.online);
      showStatus('node2', data.sensor02.online);
      showStatus('wifi', data.internet_wifi_connected);
      showStatus('mqtt', data.mqtt_connected);

      // Keep the last measurement visible if a node goes offline.
      document.getElementById('distance').textContent =
        data.sensor01.distance_cm != null
          ? Number(data.sensor01.distance_cm).toFixed(1) + ' cm'
          : 'N/A';
      document.getElementById('temperature').textContent =
        data.sensor02.temperature_c != null
          ? Number(data.sensor02.temperature_c).toFixed(1) + ' °C'
          : 'N/A';
      document.getElementById('humidity').textContent =
        data.sensor02.humidity_percent != null
          ? Number(data.sensor02.humidity_percent).toFixed(1) + ' %'
          : 'N/A';
      document.getElementById('light').textContent =
        data.sensor02.light_raw != null
          ? Number(data.sensor02.light_raw).toFixed(0) + ' %'
          : 'N/A';
      document.getElementById('node1time').textContent = data.sensor01.received ? displayTime(data.sensor01.last_received) : '--';
      document.getElementById('node2time').textContent = data.sensor02.received ? displayTime(data.sensor02.last_received) : '--';
      document.getElementById('updated').textContent = 'Page checked: ' + new Date().toLocaleTimeString();
    } catch (error) {
      document.getElementById('updated').textContent = 'Cannot reach gateway: ' + error.message;
      showStatus('node1', false);
      showStatus('node2', false);
      showStatus('wifi', false);
      showStatus('mqtt', false);
    }
  }
  updateDashboard();
  setInterval(updateDashboard, 2000);
</script>
</body>
</html>
)rawliteral";

#endif
