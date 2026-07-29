# 🛡️ Complete System Restore & Code Backup Document
**Date of Backup**: July 29, 2026  
**System Target**: ESP32-S3 Sanitary Vending Machine Controller  
**Hardware Board**: ESP32-S3-DevKitC-1  
**Status**: 100% Fully Functional, Flashed & Empirically Verified  

---

## 📋 Table of Contents
1. [System Restore Instructions](#1-system-restore-instructions)
2. [Hardware Pinout & Circuit Schematic](#2-hardware-pinout--circuit-schematic)
3. [Architecture Overview](#3-architecture-overview)
4. [File 1: platformio.ini (Build Config)](#file-1-platformioini)
5. [File 2: src/html_page.h (Web Interface)](#file-2-srchtml_pageh)
6. [File 3: src/main.cpp (Master Firmware)](#file-3-srcmaincpp)

---

## 1. System Restore Instructions

If you ever need to restore your ESP32-S3 controller from scratch, follow these simple steps:

### Prerequisites
* **PlatformIO CLI / Core** installed (or Python with PlatformIO package).
* **ESP32-S3 DevKitC-1** board connected via USB.

### Restoration Steps
1. Create a new directory named `screen_testing_v1`.
2. Save the build configuration file to `platformio.ini`.
3. Create a folder named `src`.
4. Save the web dashboard code to `src/html_page.h`.
5. Save the master firmware code to `src/main.cpp`.
6. Open terminal in the project directory and run the compile & upload command:

```powershell
C:\Python314\python.exe -m platformio run -t upload
```

---

## 2. Hardware Pinout & Circuit Schematic

| ESP32-S3 Pin | Connected Component | Signal / Line Name | Notes |
| :---: | :--- | :--- | :--- |
| **3.3V** | TFT, RC522, Encoder, DRV8833 VDD & **EEP** | VDD (3.3V Logic Rail) | Main 3.3V Power |
| **GND** | TFT, RC522, DRV8833, Encoder, Buttons, Buzzer | GND (Common Ground) | Main System Ground |
| **5V / VIN** | DRV8833 VM Pin | Motor VCC (5V/12V) | Actuator Motor Power |
| **GPIO 1** | MFRC522 RFID Reader | MISO | Hardware SPI Data In |
| **GPIO 2** | MFRC522 RFID Reader | RST | RFID Reader Reset |
| **GPIO 4** | DRV8833 Motor Driver | IN1 | Stepper Coil Phase 1 |
| **GPIO 5** | Piezo Buzzer Positive (+) | BUZZER | Audio Chime Tone |
| **GPIO 6** | DRV8833 Motor Driver | IN2 | Stepper Coil Phase 2 |
| **GPIO 7** | DRV8833 Motor Driver | IN3 | Stepper Coil Phase 3 |
| **GPIO 8** | EC11 Rotary Encoder | ENC_B | Quadrature Phase B |
| **GPIO 9** | MFRC522 RFID Reader | SDA / CS | RFID Chip Select |
| **GPIO 10** | ST7789 TFT Display | CS | Display Chip Select |
| **GPIO 11** | **SHARED**: ST7789 TFT & RC522 RFID | MOSI / SDA | Shared SPI Master Out |
| **GPIO 12** | **SHARED**: ST7789 TFT & RC522 RFID | SCLK / SCK | Shared SPI Clock |
| **GPIO 13** | ST7789 TFT Display | DC | Display Data / Command |
| **GPIO 14** | ST7789 TFT Display | RST | Display Reset |
| **GPIO 15** | ST7789 TFT Display | BLK | Display Backlight PWM |
| **GPIO 16** | EC11 Rotary Encoder | ENC_SW | Knob Push Button |
| **GPIO 17** | EC11 Rotary Encoder | ENC_A | Quadrature Phase A |
| **GPIO 18** | DRV8833 Motor Driver | IN4 | Stepper Coil Phase 4 |
| **GPIO 38 & 48** | Built-in WS2812 RGB LED | RGB_LED | Indicator LED |
| **GPIO 41** | TOF050C Sensor | SDA | I2C Data (0x29) |
| **GPIO 42** | TOF050C Sensor | SCL | I2C Clock |
| **GPIO 0 / KEY0** | BOOT Switch to GND | KEY0 / K0 | Back Button / Double-click Kill Switch |

> ⚡ **CRITICAL HARDWARE JUMPER**: The DRV8833 `EEP` / `nSLEEP` pin MUST be shorted directly to **3.3V / VDD** using a small jumper wire. This keeps the motor driver enabled and frees up `GPIO 9` for the RFID Chip Select.

---

## 3. Architecture Overview

- **Zero-Flicker SPI Engine**: Displays graphics via in-place slot updating and single-pass static frame initialization (`drawCareDispenseFrame`).
- **5V DRV8833 Motor Driver Physics**: Stepper motor runs with 2-phase full stepping (`startSpeed = 2600us`, `maxSpeed = 1800us`, `rampSteps = 80`).
- **Unified Motion Dispatcher**: Both RFID Card Scan dispensing and Web/Knob control use `executeGlobalMotorMove()`, ensuring 100% smooth, identical motor execution without SPI pulse delays.
- **NVS Multi-Admin Storage**: Stores up to 5 Admin Cards in non-volatile flash under `"admin-store"`. Tapping any admin card toggles menu unlock mode and disables 10-second idle auto-exit.

---

## 4. File 1: platformio.ini

Save the code below as `platformio.ini`:

```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
upload_speed = 115200
upload_flags =
    --no-stub
build_flags =
    -D ARDUINO_USB_CDC_ON_BOOT=1
    -D ARDUINO_USB_MODE=1
lib_deps =
    adafruit/Adafruit GFX Library @ ^1.11.9
    adafruit/Adafruit ST7735 and ST7789 Library @ ^1.10.4
    ricmoo/QRCode @ ^0.0.1
    knolleary/PubSubClient @ ^2.8
    miguelbalboa/MFRC522 @ ^1.4.10
    adafruit/Adafruit_VL6180X @ ^1.4.3
```
