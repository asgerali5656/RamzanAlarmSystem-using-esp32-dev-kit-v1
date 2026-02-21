# Ramzan Alarm System 🌙

An ESP32-based smart alarm system designed to automate timetabled alerts for Sehri, Iftar, and daily prayer times during Ramzan. The system features a local web dashboard for remote management, auto-off buzzer sequences, deep-sleep power saving, wireless OTA (Over-The-Air) updates, and real-time LCD display integration for seamless daily scheduling and automation.

---

## 🚀 Features

- **Automated Alarm Patterns:** Custom sequence for Sehri and Iftar (5 seconds continuous continuous ON, followed by 5 short rapid rings, then auto-off). No manual intervention needed to stop the alarm.
- **Smart Deep Sleep:** Intelligently calculates the time to the next alarm. If the next alarm is more than 3 hours away, the ESP32 enters deep sleep to save power, waking up exactly 30 minutes before the next scheduled event. Manual wake via navigation button is supported.
- **Web Dashboard:** Access a beautiful glass-morphism web interface locally (Port 80) to view:
  - Live mock LCD display
  - Upcoming schedules
  - Real-time switch status
  - Uptime and connectivity metrics
- **Over-The-Air (OTA) Updates:** Flash new firmware wirelessly over WiFi via the web dashboard—no need to connect the ESP32 to a PC.
- **Offline Reliability:** Stores the full month's timetable locally. Time is synchronized via NTP when connected to WiFi, ensuring accuracy even if internet access drops temporarily.
- **Watchdog Timer (WDT) Protection:** Guards against system freezes and network hanging.

---

## 🛠️ Hardware Requirements

- **ESP32 Development Board** (e.g., DOIT ESP32 DEVKIT V1)
- **Active Buzzers or 5V Relay Modules** (x2 for House A & House B)
- **LCD Display** (16x2 or 20x4 parallel format, 4-bit mode)
- **Push Buttons / Toggle Switches** (x4 for House A, House B, Navigation, and Pre-Sehri enable)
- Jumper wires and breadboard/PCB

---

## 🔌 Circuit Diagram & Pinout

Below is the mapping for connecting your peripherals to the ESP32.

### Buzzers / Relays
 *(Note: Configure `RELAY_ACTIVE_LOW` in `Config.h` depending on whether your relays trigger on a high or low signal).*
| Component | ESP32 Pin | Description |
| :--- | :--- | :--- |
| **Buzzer/Relay House A** | `GPIO 25` | Output for House A alarm |
| **Buzzer/Relay House B** | `GPIO 26` | Output for House B alarm |

### Buttons & Switches
| Component | ESP32 Pin | Description |
| :--- | :--- | :--- |
| **House A Button** | `GPIO 16` | (Optional manual override input) Connect to GND |
| **House B Button** | `GPIO 17` | (Optional manual override input) Connect to GND |
| **Pre-Sehri Switch** | `GPIO 13` | Toggle switch to enable 1-hour prior alerts |
| **Navigation Button**| `GPIO 4`  | Toggles LCD screen / Wakes from Deep Sleep |

### LCD Display (4-bit Mode)
| LCD Pin | ESP32 Pin |
| :--- | :--- |
| **RS** | `GPIO 21` |
| **EN** | `GPIO 22` |
| **D4** | `GPIO 23` |
| **D5** | `GPIO 19` |
| **D6** | `GPIO 18` |
| **D7** | `GPIO 5`  |

*(Remember to wire LCD VCC to 5V/3.3V as per your screen model, GND to GND, and use a potentiometer on `V0` for contrast).*

---

## 💻 Software Setup

### Prerequisites
1. Install **Arduino IDE**.
2. Add ESP32 board support via the Boards Manager.
3. No complex external libraries required (uses native ESP32 `WiFi.h`, `Preferences.h`, `Update.h`, `esp_task_wdt.h`).

### Configuration
1. Open `Config.h`.
2. Update your **WiFi Credentials**:
   ```cpp
   #define WIFI_SSID     "your_wifi_name"
   #define WIFI_PASSWORD "your_wifi_password"
   ```
3. Set your **Timezone Offset**:
   ```cpp
   // Adjust for your location. Example: India is UTC +5:30 (19800 seconds)
   #define GMT_OFFSET_SEC  19800 
   ```

### Installation
1. Connect your ESP32 to your computer.
2. Select your board (e.g., `DOIT ESP32 DEVKIT V1`) and your COM port in Arduino IDE.
3. Open `RamzanAlarm.ino`.
4. Click **Upload**.

---

## 📱 Using the Web Dashboard

1. Once the ESP32 connects to WiFi, the assigned IP address will scroll across the physical LCD.
2. Open your web browser and navigate to `http://<ESP32_IP_ADDRESS>`.
3. From the dashboard, you can monitor the upcoming alarms, configure global timing offsets, trigger a test alarm, and upload new `.bin` updates over-the-air.

## 📄 License
This codebase is open-source and free to modify for community use. Ramzan Mubarak!
