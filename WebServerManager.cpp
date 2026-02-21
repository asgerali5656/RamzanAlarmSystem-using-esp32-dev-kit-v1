#include "WebServerManager.h"
#include <WiFi.h>
#include <Preferences.h>
#include <Update.h> // ESP32 OTA Library
#include <esp_task_wdt.h> // Added for WDT handling during OTA
#include "NetworkManager.h"
#include "AlarmScheduler.h"
#include "BuzzerEngine.h"
#include "SystemState.h"
#include "DisplayManager.h"

// External references
extern RamzanNetworkManager networkManager;
extern AlarmScheduler alarmScheduler;
extern SystemState currentState;
extern DisplayManager displayManager; // Added DisplayManager
#include "ButtonEngine.h"
extern BuzzerEngine buzzerA;
extern BuzzerEngine buzzerB;
extern ButtonEngine btnHouseA;
extern ButtonEngine btnHouseB;
extern volatile unsigned long bootTime; 
extern void startTestMode(); 
extern void updatePrayerPattern(int count, int dur, int gap); 
extern void updateSehriPattern(int dur, int interval); 
extern Preferences prefs; 

WebServerManager::WebServerManager() : server(80) {}

void WebServerManager::init() {
    Serial.println("DEBUG: Init WebServer...");
    
    // --- Define Routes ---
    
    server.on("/", [this](){ handleRoot(); });
    server.on("/status", [this](){ handleStatus(); });
    
    // New Feature APIs
    server.on("/api/display", [this](){ handleDisplayJson(); });
    server.on("/api/message", [this](){ handleMessage(); });

    // Settings
    server.on("/settings", [this](){ handleSettings(); });
    server.on("/save-settings", [this](){ handleSaveSettings(); });

    // Web OTA Update
    // 1. GET /update -> Show Form
    server.on("/update", HTTP_GET, [this](){ handleUpdate(); });
    
    // 2. POST /update -> Process File Upload
    server.on("/update", HTTP_POST, [this](){
        // When upload finishes
        if (Update.hasError()) {
            server.send(200, "text/plain", "Update Failed. Restarting...");
        } else {
            server.send(200, "text/plain", "Update Success! Rebooting...");
            delay(1000);
            ESP.restart();
        }
    }, [this](){
        // During upload
        handleUpdateUpload();
    });

    server.on("/trigger-test", [this](){
        handleTest();
    });

    server.on("/api/reboot", [this](){
        server.send(200, "text/plain", "Rebooting...");
        delay(500);
        ESP.restart();
    });

    server.onNotFound([this](){
        server.send(404, "text/plain", "Not Found");
    });

    server.begin();
    Serial.println("WebServer Started on Port 80");
}

void WebServerManager::handleTest() {
    Serial.println("WEB: Triggering Test Mode");
    startTestMode();
    server.send(200, "text/plain", "Test Triggered");
}

void WebServerManager::handleClient() {
    server.handleClient();
}

void WebServerManager::handleRoot() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>Ramzan Alarm Hub</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;600&display=swap" rel="stylesheet">
  <style>
    :root {
      --primary: #00e5ff;
      --secondary: #7c4dff;
      --success: #00c853;
      --error: #ff3d00;
      --bg: #0f172a;
      --glass: rgba(255, 255, 255, 0.05);
      --glass-border: rgba(255, 255, 255, 0.1);
      --text: #e2e8f0;
    }
    
    body {
      font-family: 'Outfit', sans-serif;
      margin: 0;
      padding: 0;
      background: var(--bg);
      background-image: radial-gradient(circle at 10% 20%, rgba(124, 77, 255, 0.2) 0%, transparent 20%),
                        radial-gradient(circle at 90% 80%, rgba(0, 229, 255, 0.15) 0%, transparent 20%);
      color: var(--text);
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
    }

    /* Glassmorphism Logic */
    .glass-panel {
      background: var(--glass);
      backdrop-filter: blur(12px);
      -webkit-backdrop-filter: blur(12px);
      border: 1px solid var(--glass-border);
      border-radius: 16px;
      padding: 24px;
      margin: 16px;
      box-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.37);
      width: 90%;
      max-width: 600px;
    }

    h1 {
      font-weight: 600;
      background: linear-gradient(90deg, var(--primary), var(--secondary));
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
      margin-bottom: 8px;
    }

    h2 {
      font-size: 1.1rem;
      color: rgba(255,255,255,0.7);
      margin-top: 0;
      border-bottom: 1px solid var(--glass-border);
      padding-bottom: 8px;
    }

    /* Live Display Simulation */
    .lcd-display {
      background: #000;
      color: #00ff00;
      font-family: 'Courier New', monospace;
      padding: 15px;
      border-radius: 8px;
      border: 4px solid #333;
      box-shadow: inset 0 0 10px rgba(0, 255, 0, 0.2);
      margin: 20px 0;
      text-align: left;
      font-size: 1.2rem;
      line-height: 1.4;
      min-height: 60px;
      display: flex;
      flex-direction: column;
      justify-content: center;
      position: relative;
      overflow: hidden;
    }
    
    .lcd-line {
      white-space: pre;
      overflow: hidden;
    }

    /* Controls */
    .btn-group {
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
      justify-content: center;
      margin-top: 15px;
    }

    button, .btn-link {
      background: linear-gradient(135deg, rgba(255,255,255,0.1), rgba(255,255,255,0.05));
      border: 1px solid var(--glass-border);
      color: var(--primary);
      padding: 12px 24px;
      border-radius: 12px;
      cursor: pointer;
      font-family: inherit;
      font-weight: 600;
      text-decoration: none;
      transition: all 0.3s ease;
      display: inline-block;
      text-align: center;
    }

    button:hover, .btn-link:hover {
      background: rgba(0, 229, 255, 0.2);
      transform: translateY(-2px);
      box-shadow: 0 4px 12px rgba(0, 229, 255, 0.3);
    }

    button.secondary {
      color: var(--secondary);
    }
    button.secondary:hover {
      background: rgba(124, 77, 255, 0.2);
    }
    
    input[type=text] {
      width: 100%;
      background: rgba(0,0,0,0.3);
      border: 1px solid var(--glass-border);
      color: #fff;
      padding: 12px;
      border-radius: 8px;
      margin-bottom: 10px;
      box-sizing: border-box;
      font-family: 'Courier New', monospace;
    }

    .status-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 15px;
      text-align: left;
    }
    .status-item span {
      display: block;
      font-size: 0.8rem;
      color: #aaa;
    }
    .status-item strong {
      display: block;
      font-size: 1rem;
      color: #fff;
    }

    /* Schedule List */
    .schedule-list {
      list-style: none;
      padding: 0;
      text-align: left;
    }
    .schedule-item {
      padding: 8px 0;
      border-bottom: 1px solid rgba(255,255,255,0.05);
      display: flex;
      justify-content: space-between;
      font-size: 0.95rem;
    }
    .schedule-item:last-child { border: none; }
    .schedule-label { color: #aaa; }
    .schedule-time { color: var(--primary); font-weight: 600; }

    /* Switch Status Indicators */
    .switch-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 10px;
    }
    .switch-card {
      background: rgba(0,0,0,0.2);
      padding: 10px;
      border-radius: 8px;
      text-align: center;
      border: 1px solid rgba(255,255,255,0.05);
    }
    .switch-status {
      font-weight: 600;
      margin-top: 5px;
      display: block;
    }
    .status-on { color: var(--success); }
    .status-off { color: var(--error); transition: all 0.3s; }
    
    @keyframes pulse {
        0% { opacity: 1; transform: scale(1); }
        50% { opacity: 0.5; transform: scale(0.95); }
        100% { opacity: 1; transform: scale(1); }
    }

  </style>
  <script>
    function updateDisplayDetail() {
      fetch('/api/display').then(res => res.json()).then(data => {
        document.getElementById('lcd-l1').innerText = data.l1 || "";
        document.getElementById('lcd-l2').innerText = data.l2 || "";
      }).catch(e => console.error("Display fetch failed"));
    }

    function updateStatus() {
      fetch('/status').then(res => res.json()).then(data => {
        document.getElementById('sys-time').innerHTML = data.time + "<br><span style='font-size:0.7em'>" + data.date + "</span>";
        document.getElementById('next-alarm').innerText = data.nextAlarm + " (" + data.nextTime + ")";
        document.getElementById('uptime').innerText = data.uptime;
        if(data.lastDuration) document.getElementById('last-duration').innerText = data.lastDuration;
        
        // Update Switch Status
        updateSwitchUI('sw-a', data.swA, data.ringA);
        updateSwitchUI('sw-b', data.swB, data.ringB);
        
        // Update Schedule
        if(data.schedule) {
            let html = '';
            data.schedule.forEach(item => {
                html += `<li class="schedule-item">
                    <span class="schedule-label">${item.day} ${item.name}</span>
                    <span class="schedule-time">${item.time}</span>
                </li>`;
            });
            document.getElementById('sched-list').innerHTML = html;
        }

      }).catch(e => console.error("Status fetch failed"));
    }
    
    function updateSwitchUI(id, isOn, isRinging) {
        const el = document.getElementById(id);
        // Show actual physical state
        if(isOn) {
            el.innerText = "ON";
            el.className = "switch-status status-on";
        } else {
            el.innerText = "OFF"; 
            el.className = "switch-status status-off";
        }
        
        // Visual cue for ringing, but keep text as state
        if (isRinging) {
            el.style.animation = "pulse 1s infinite";
        } else {
            el.style.animation = "none";
        }
    }

    function sendMessage() {
      const l1 = document.getElementById('msg-l1').value;
      const l2 = document.getElementById('msg-l2').value;
      fetch(`/api/message?l1=${encodeURIComponent(l1)}&l2=${encodeURIComponent(l2)}`)
        .then(() => {
          alert("Message Sent to Display!");
          document.getElementById('msg-l1').value = "";
          document.getElementById('msg-l2').value = "";
          updateDisplayDetail(); // Immediate update
        });
    }

    function triggerTest() {
      fetch('/trigger-test').then(() => alert("Test Mode Triggered!"));
    }

    setInterval(updateDisplayDetail, 1000);
    setInterval(updateStatus, 3000);
  </script>
</head>
<body onload="updateDisplayDetail(); updateStatus();">

  <div class="glass-panel" style="text-align: center;">
    <h1>Ramzan Alarm Hub</h1>
    <div style="font-size: 0.9em;opacity: 0.7;">System Online • Protected</div>
  </div>
  
  <div class="glass-panel">
    <h2>House Switches Status</h2>
    <div class="switch-grid">
      <div class="switch-card">
        <div style="font-size:0.9em;opacity:0.7;">House A Button</div>
        <span id="sw-a" class="switch-status">Loading...</span>
      </div>
      <div class="switch-card">
        <div style="font-size:0.9em;opacity:0.7;">House B Button</div>
        <span id="sw-b" class="switch-status">Loading...</span>
      </div>
    </div>
    <div style="margin-top:10px;font-size:0.8em;opacity:0.6;text-align:center;">
      If Alarm rings, toggle switch to stop. <br>
      ON/OFF indicates physical switch position.
    </div>
  </div>

  <div class="glass-panel">
    <h2>Upcoming Schedule</h2>
    <ul id="sched-list" class="schedule-list">
        <li style="text-align:center;opacity:0.5;">Loading schedule...</li>
    </ul>
  </div>

  <div class="glass-panel">
    <h2>Live Device Preview</h2>
    <div class="lcd-display">
      <div class="lcd-line" id="lcd-l1">Connecting...</div>
      <div class="lcd-line" id="lcd-l2">Waiting for data...</div>
    </div>
    <div style="text-align: center; font-size: 0.8em; opacity: 0.6;">Updates every 1s</div>
  </div>

  <div class="glass-panel">
    <h2>Broadcast Message</h2>
    <p style="font-size: 0.9em; margin-bottom: 15px;">Override display for 5 seconds.</p>
    <input type="text" id="msg-l1" placeholder="Line 1 (Max 16 chars)" maxlength="16">
    <input type="text" id="msg-l2" placeholder="Line 2 (Max 16 chars)" maxlength="16">
    <button onclick="sendMessage()" style="width:100%">SEND TO DISPLAY</button>
  </div>

  <div class="glass-panel">
    <h2>System Status</h2>
    <div class="status-grid">
      <div class="status-item">
        <span>System Time</span>
        <strong id="sys-time">--:--</strong>
      </div>
      <div class="status-item">
        <span>Next Alarm</span>
        <strong id="next-alarm">--</strong>
      </div>
      <div class="status-item">
        <span>Uptime</span>
        <strong id="uptime">--</strong>
      </div>
      <div class="status-item">
        <span>Last Alarm Duration</span>
        <strong id="last-duration">--</strong>
      </div>
    </div>
  </div>

  <div class="glass-panel">
    <h2>System Controls & Maintenance</h2>
    <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 15px;">
      
      <div class="switch-card" style="text-align: left;">
        <div style="font-weight: 600; color: var(--primary); margin-bottom: 8px;">Alarm Testing</div>
        <p style="font-size: 0.8rem; opacity: 0.7; margin: 0 0 12px 0;">Trigger a full alarm cycle to verify house buzzers and switches.</p>
        <button onclick="triggerTest()" style="width: 100%; border-color: var(--secondary);">FIRE TEST ALARM</button>
      </div>

      <div class="switch-card" style="text-align: left;">
        <div style="font-weight: 600; color: var(--primary); margin-bottom: 8px;">Global Config</div>
        <p style="font-size: 0.8rem; opacity: 0.7; margin: 0 0 12px 0;">Adjust time offsets, beep patterns, and alarm behavior.</p>
        <a href="/settings" class="btn-link" style="width: 100%; box-sizing: border-box;">OPEN SETTINGS</a>
      </div>

      <div class="switch-card" style="text-align: left;">
        <div style="font-weight: 600; color: #ffab40; margin-bottom: 8px;">Firmware Update</div>
        <p style="font-size: 0.8rem; opacity: 0.7; margin: 0 0 12px 0;">Upload new system firmware (.bin) via wireless update.</p>
        <a href="/update" class="btn-link secondary" style="width: 100%; box-sizing: border-box; color: #ffab40;">UPDATE DEVICE</a>
      </div>

      <div class="switch-card" style="text-align: left;">
        <div style="font-weight: 600; color: #ff5252; margin-bottom: 8px;">Emergency Restart</div>
        <p style="font-size: 0.8rem; opacity: 0.7; margin: 0 0 12px 0;">Perform a cold reboot of the ESP32 controller.</p>
        <button onclick="if(confirm('Reboot device?')) fetch('/api/reboot')" class="secondary" style="width: 100%; color: #ff5252;">REBOOT NOW</button>
      </div>

    </div>
  </div>

</body>
</html>
)rawliteral";
    server.send(200, "text/html", html);
}

void WebServerManager::handleDisplayJson() {
    String json = "{";
    json += "\"l1\":\"" + displayManager.getCurrentLine1() + "\",";
    json += "\"l2\":\"" + displayManager.getCurrentLine2() + "\"";
    json += "}";
    server.send(200, "application/json", json);
}

void WebServerManager::handleMessage() {
    if (server.hasArg("l1") || server.hasArg("l2")) {
        String l1 = server.arg("l1");
        String l2 = server.arg("l2");
        displayManager.setOverrideMessage(l1, l2, 5000);
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Missing args");
    }
}

void WebServerManager::handleStatus() {
    String json = "{";
    json += "\"time\":\"" + networkManager.getFormattedTime() + "\",";
    json += "\"date\":\"" + networkManager.getFormattedDate() + "\",";
    json += "\"nextAlarm\":\"" + alarmScheduler.getNextAlarmName() + "\",";
    json += "\"nextTime\":\"" + alarmScheduler.getNextAlarmTime() + "\",";
    
    unsigned long upSec = millis() / 1000;
    int upH = upSec / 3600;
    int upM = (upSec % 3600) / 60;
    json += "\"uptime\":\"" + String(upH) + "h " + String(upM) + "m\",";
    
    // Last Alarm Duration
    json += "\"lastDuration\":\"" + alarmScheduler.getLastAlarmDuration() + "\",";
    
    // Switch Status (Active Low: LOW=ON, HIGH=OFF)
    // Actually, "ENABLED" means ready to ring.
    // Since our logic captures state on trigger, 'Status' here just shows physical position
    // Let's show: ON (GND) / OFF (OPEN)
    
    bool swA = (btnHouseA.getState() == LOW);
    bool swB = (btnHouseB.getState() == LOW);
    json += "\"swA\":" + String(swA ? "true" : "false") + ",";
    json += "\"swB\":" + String(swB ? "true" : "false") + ",";
    
    // Buzzer Ringing Status
    bool ringA = buzzerA.getBuzzerState(); // isRinging() checks pattern valid, getBuzzerState checks ON/OFF
    bool ringB = buzzerB.getBuzzerState(); 
    // Actually we want to know if pattern is active OR if it's currently making sound
    // Let's us isRinging() to show "Alarm Active" which is more useful than blinking "ON/OFF"
    
    json += "\"ringA\":" + String(buzzerA.isRinging() ? "true" : "false") + ",";
    json += "\"ringB\":" + String(buzzerB.isRinging() ? "true" : "false") + ",";
    
    // Schedule
    json += "\"schedule\":" + alarmScheduler.getUpcomingScheduleJson();
    
    json += "}";
    
    server.send(200, "application/json", json);
}

void WebServerManager::handleSettings() {
    int sOff = alarmScheduler.getSehriOffset();
    int iOff = alarmScheduler.getIftarOffset();
    
    // Read Current Prayer Settings from Prefs
    prefs.begin("ramzan", true); // ReadOnly
    int pCount = prefs.getInt("pCount", 2);
    int pDur = prefs.getInt("pDur", 300);
    int pGap = prefs.getInt("pGap", 300);
    int sDur = prefs.getInt("sDur", 5000);
    int sInt = prefs.getInt("sInt", 10000);
    int preOff = prefs.getInt("preOff", 60);
    bool sleep = prefs.getBool("sleep", false);
    prefs.end();

    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Alarm Settings</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;600&display=swap" rel="stylesheet">
  <style>
    body { font-family: 'Outfit', sans-serif; text-align: center; background-color: #0f172a; color: #e0e0e0; margin: 0; padding: 20px; }
    h1 { color: #00e5ff; }
    .card { background: rgba(255, 255, 255, 0.05); padding: 20px; border-radius: 16px; margin: 10px auto; max-width: 400px; border: 1px solid rgba(255, 255, 255, 0.1); }
    input[type=number] { padding: 10px; margin: 10px; width: 80px; font-size: 16px; text-align: center; background: rgba(0,0,0,0.3); border: 1px solid rgba(255,255,255,0.2); color: white; border-radius: 8px; }
    input[type=submit] { background: linear-gradient(135deg, rgba(255,255,255,0.1), rgba(255,255,255,0.05)); color: #00e5ff; padding: 15px 32px; border: 1px solid rgba(255,255,255,0.1); font-size: 16px; cursor: pointer; border-radius: 12px; margin-top: 20px; font-weight: 600; }
    input[type=submit]:hover { background: rgba(0, 229, 255, 0.2); }
    a { color: #7c4dff; text-decoration: none; display: block; margin-top: 20px; }
    h3 { margin-top: 20px; margin-bottom: 5px; color: #ccc; font-size: 1rem; border-bottom: 1px solid rgba(255,255,255,0.1); padding-bottom: 5px; }
    label { font-size: 0.9rem; color: #aaa; }
  </style>
</head>
<body>
  <h1>System Configuration</h1>
  <div class="card">
    <form action="/save-settings" method="GET">
      
      <h3>Time Offsets (Minutes)</h3>
      <label>Sehri Offset:</label>
      <input type="number" name="sehriOffset" value=")rawliteral" + String(sOff) + R"rawliteral(">
      <br>
      <label>Iftar Offset:</label>
      <input type="number" name="iftarOffset" value=")rawliteral" + String(iOff) + R"rawliteral(">
      <br>
      <label>Pre-Sehri Offset (Min Before):</label>
      <input type="number" name="preOff" value=")rawliteral" + String(preOff) + R"rawliteral(">
      
      <h3>Power & Display</h3>
      <label>Deep Sleep Mode:</label>
      <input type="checkbox" name="sleep" )rawliteral" + String(sleep ? "checked" : "") + R"rawliteral(">
      <p style="font-size: 0.7rem; color: #ff5252; margin: 5px 0 15px 0;">Warning: Web UI will be offline during sleep!</p>
      
      <h3>Prayer End Beep Pattern</h3>
      <label>Beep Count:</label>
      <input type="number" name="pCount" min="1" max="10" value=")rawliteral" + String(pCount) + R"rawliteral(">
      <br>
      <label>Beep Duration (ms):</label>
      <input type="number" name="pDur" step="50" min="50" value=")rawliteral" + String(pDur) + R"rawliteral(">
      <br>
      <label>Gap Duration (ms):</label>
      <input type="number" name="pGap" step="50" min="50" value=")rawliteral" + String(pGap) + R"rawliteral(">

      <h3>Sehri Alarm Pattern</h3>
      <label>Ring Duration (ms):</label>
      <input type="number" name="sDur" step="1000" min="1000" value=")rawliteral" + String(sDur) + R"rawliteral(">
      <br>
      <label>Repeat Interval (ms):</label>
      <input type="number" name="sInt" step="1000" min="1000" value=")rawliteral" + String(sInt) + R"rawliteral(">

      <br>
      <input type="submit" value="SAVE CHANGES">
    </form>
    <a href="/">< Back to Dashboard</a>
  </div>
</body>
</html>
)rawliteral";
    server.send(200, "text/html", html);
}

void WebServerManager::handleSaveSettings() {
    bool updated = false;
    
    // Offsets
    if (server.hasArg("sehriOffset") && server.hasArg("iftarOffset")) {
        int sOff = server.arg("sehriOffset").toInt();
        int iOff = server.arg("iftarOffset").toInt();
        int preOff = server.arg("preOff").toInt();
        bool sleep = server.hasArg("sleep");
        
        alarmScheduler.setOffsets(sOff, iOff);
        alarmScheduler.setPreSehriOffset(preOff);
        
        prefs.begin("ramzan", false);
        prefs.putInt("sOff", sOff);
        prefs.putInt("iOff", iOff);
        prefs.putInt("preOff", preOff);
        prefs.putBool("sleep", sleep);
        prefs.end();
        updated = true;
    }
    
    // Prayer Pattern
    if (server.hasArg("pCount") && server.hasArg("pDur") && server.hasArg("pGap")) {
        int pCount = server.arg("pCount").toInt();
        int pDur = server.arg("pDur").toInt();
        int pGap = server.arg("pGap").toInt();
        
        updatePrayerPattern(pCount, pDur, pGap);
        
        prefs.begin("ramzan", false);
        prefs.putInt("pCount", pCount);
        prefs.putInt("pDur", pDur);
        prefs.putInt("pGap", pGap);
        prefs.end();
        updated = true;
    }
    
    // Sehri Pattern
    if (server.hasArg("sDur") && server.hasArg("sInt")) {
        int sDur = server.arg("sDur").toInt();
        int sInt = server.arg("sInt").toInt();
        
        updateSehriPattern(sDur, sInt);
        
        prefs.begin("ramzan", false);
        prefs.putInt("sDur", sDur);
        prefs.putInt("sInt", sInt);
        prefs.end();
        updated = true;
    }

    if (updated) {
        server.sendHeader("Location", "/");
        server.send(303); 
    } else {
        server.send(400, "text/plain", "Missing Parameters");
    }
}

void WebServerManager::handleUpdate() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Firmware Update</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; text-align: center; background-color: #121212; color: #e0e0e0; margin: 0; padding: 20px; }
    h1 { color: #ff5722; }
    .card { background: #1e1e1e; padding: 20px; border-radius: 10px; margin: 10px auto; max-width: 400px; }
    input[type=file] { margin: 20px 0; color: #fff; }
    input[type=submit] { background-color: #ff5722; color: white; padding: 15px 32px; border: none; font-size: 16px; cursor: pointer; border-radius: 5px; }
    a { color: #00bcd4; text-decoration: none; display: block; margin-top: 20px; }
  </style>
</head>
<body>
  <h1>System Update</h1>
  <div class="card">
    <p>Upload a new firmware (.bin) file</p>
    <form method='POST' action='/update' enctype='multipart/form-data'>
      <input type='file' name='update'>
      <br>
      <input type='submit' value='Update Firmware'>
    </form>
    <a href="/">< Cancel</a>
  </div>
</body>
</html>
)rawliteral";
    server.send(200, "text/html", html);
}

void WebServerManager::handleUpdateUpload() {
    // Feed the watchdog to prevent timeouts during long upload/write operations
    esp_task_wdt_reset();

    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Update: %s\n", upload.filename.c_str());
        // Use totalSize if available for better stability
        size_t fileSize = (upload.totalSize > 0) ? upload.totalSize : UPDATE_SIZE_UNKNOWN;
        
        // Feed before potentially long erase operation
        esp_task_wdt_reset(); 
        
        if (!Update.begin(fileSize)) { 
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
        }
        // Feed after each chunk write
        esp_task_wdt_reset();
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { 
            Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
            Update.printError(Serial);
        }
        esp_task_wdt_reset();
    }
}
