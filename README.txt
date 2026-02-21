# Ramzan Alarm System - Arduino Sketch

This is the Arduino-ready version of the code.

## How to Flash

1.  **Open Arduino IDE**.
2.  Go to `File > Open` and navigate to this folder: `RamzanAlarm`.
3.  Select `RamzanAlarm.ino`.
4.  **Connect your ESP32**.
5.  Select your board (e.g., `DOIT ESP32 DEVKIT V1`) and Port.
6.  Click **Compile** (Checkmark) to verify.
7.  Click **Upload** (Arrow).

## Pin Configuration (Updated)

| Component | Pin (GPIO) | Notes |
| :--- | :--- | :--- |
| **Relay A (House A)** | **25** | Corrected from your request. |
| **Relay B (House B)** | **26** | Corrected. |
| **Wake Button A** | **16** | Corrected. |
| **Wake Button B** | **17** | Corrected. |
| **Pre-Sehri Switch** | **13** | **I assigned this pin.** Connect your main switch here. |
| **Navigation Button**| **4** | Defined for future use. |
| **LCD** | 21,22... | Defined for future display logic. |

## Usage (Phase 1 Testing)

Open Serial Monitor at `115200` baud.
- **Debug Mode**: The system now prints "DEBUG" messages whenever a switch or button changes state. Use this to verify your wiring.
- Send `1`: Trigger Pre-Sehri Alarm (Stop by toggling Switch on Pin 13).
- Send `2`: Trigger Sehri Alarm (Stop by pressing House Button A/B individually).
- Send `4`: Trigger Prayer Beep.
