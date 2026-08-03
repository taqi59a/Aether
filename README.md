# ⚡ Aether Sanitary Vending Machine Firmware

High-performance, zero-flicker embedded firmware for the **Aether Sanitary Care Vending Machine (#5552)** built on the **ESP32-S3 Dual-Core MCU**.

---

## 🌟 Key Features

- **3D Cyber Matrix & Particle Starfield Screensaver**: Zero-flicker 50Hz animation engine featuring metallic gold "AETHER" branding and orbiting plasma shield nodes.
- **50Hz Breathing Rainbow RGB NeoPixel**: Smooth 360° spectrum LED color rotation during Standby and Screensaver modes.
- **Precision Time-of-Flight (ToF) Stock Sensing**: Hardware 16-bit register engine for ST VL6180X / TOF050C sensors with 7-sample median noise filtering and up to 500mm stack measurement.
- **Permanent NVS Zero-Stock Calibration**: One-touch zero-stock depth calibration stored permanently in Non-Volatile Storage (NVS) Flash memory.
- **Timing-Based Rate-of-Change Vend Engine**: Algorithmic vend status verification ($\Delta D$) with automatic jam detection and clearing pulse retry logic.
- **Dual Security Authentication**: Pattern-lock rotary encoder password and MFRC522 Admin RFID card authorization.
- **Stealth Mode & Mobile Web Control**: Integrated async web admin dashboard and HiveMQ MQTT cloud telemetry broker.

---

## 🛠️ Hardware Specification

| Component | Pin / Bus Interface | Specs / Detail |
| :--- | :--- | :--- |
| **MCU** | ESP32-S3 | 240MHz Dual Core, 8MB Flash |
| **TFT Display** | ST7789 (240x320) | Software SPI (CS: 16, DC: 15, RST: 14, BLK: 17, SCLK: 12, MOSI: 13) |
| **ToF Sensor** | ST VL6180X / TOF050C | I2C (SDA: 41, SCL: 42) @ 0x29 |
| **RFID Reader** | MFRC522 | Hardware SPI (SS: 9, RST: 2, MISO: 1, SCK: 12, MOSI: 13) |
| **Motor Driver** | DRV8833 Dual H-Bridge | Stepper Pins (IN1: 6, IN2: 7, IN3: 10, IN4: 11, EEP: 9) |
| **RGB LED** | Built-in WS2812 NeoPixel | GPIO 48 / GPIO 38 |
| **Controls** | Rotary Encoder & Key 0 | ENC_A: 18, ENC_B: 8, ENC_SW: 3, KEY0: 46 |

---

## 🚀 Getting Started

### Prerequisites
- [PlatformIO Core / IDE](https://platformio.org/)
- ESP32 Development Board Support

### Build & Flash Firmware
```bash
# Clone repository
git clone https://github.com/taqi59a/Aether.git
cd Aether

# Build firmware
platformio run

# Flash to ESP32-S3 over USB
platformio run -t upload
```

---

## 📡 Cloud & Web Integration
- **Web Dashboard**: Access local IP or connect to cloud portal at `broker.hivemq.com` (WSS).
- **MQTT Command Topic**: `aether/vending/5552/cmd`
- **MQTT Status Topic**: `aether/vending/5552/status`

---

## 📄 License
Designed for Aether Tech. All rights reserved.
