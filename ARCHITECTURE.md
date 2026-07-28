# 🏗️ ESP32-S3 Master Hardware Architecture & Shared Bus Reference

## 1. System Overview & Performance Guarantees

This document defines the complete hardware pinout, shared bus topology, and circuit connections for the **Aether Care Women's Sanitary Vending Machine Controller** powered by the **ESP32-S3**.

### Performance & Bus Sharing Highlights
* **Shared Hardware SPI Bus (Maximum Performance)**: ST7789 240x320 IPS TFT Display and MFRC522 RFID Card Reader share `SCLK (GPIO 12)` & `MOSI (GPIO 13)` for ultra-fast, zero-lag rendering and card detection.
* **Shared Hardware I2C Bus (Expandable Sensor Network)**: TOF050C / VL6180X ToF Laser Stock Sensor sits on `SDA (GPIO 41)` & `SCL (GPIO 42)`. Any future I2C sensors (Temp/Humidity, OLED) reuse these exact same 2 wires!
* **Hardware EEP Jumper Short**: DRV8833 `EEP` / `nSLEEP` is hard-shorted directly to `3.3V / VDD`, freeing up `GPIO 9` for the RFID Chip Select (`SDA`).
* **Shared 3.3V Power & GND Rails**: Single unified power and ground plane for maximum electrical stability and minimal wiring bulk.

---

## 2. Bus Sharing Breakdown (Re-used Pins)

### 🔄 A. Shared Hardware SPI Bus (Display + RFID)
| Signal Name | ESP32-S3 Pin | Connected Modules |
| :--- | :---: | :--- |
| **SCLK / SCK** | **GPIO 12** | ST7789 Display SCLK **AND** MFRC522 RFID SCK |
| **MOSI / SDA** | **GPIO 13** | ST7789 Display MOSI **AND** MFRC522 RFID MOSI |

### 🔄 B. Shared Hardware I2C Bus (Laser Stock Sensor + Future Expansion)
| Signal Name | ESP32-S3 Pin | Connected Modules |
| :--- | :---: | :--- |
| **I2C SDA** | **GPIO 41** | TOF050C / VL6180X Sensor SDA *(Re-usable for future I2C modules)* |
| **I2C SCL** | **GPIO 42** | TOF050C / VL6180X Sensor SCL *(Re-usable for future I2C modules)* |

### ⚡ C. Shared Power & Ground Rails
| Power Rail | Voltage | Connected Modules |
| :--- | :---: | :--- |
| **VDD / 3.3V** | 3.3V | TFT VCC, RC522 VCC, Encoder VCC, DRV8833 VDD, DRV8833 **EEP**, TOF050C VIN |
| **GND** | 0V | Common Ground for ALL Modules |
| **VM / 5V** | 5V / 12V | DRV8833 VM (Motor Power VCC) |

---

## 3. Master ESP32-S3 Complete Pin Assignment Matrix

| ESP32-S3 Pin | Connected Component | Signal Name | Bus Type / Connection Rule |
| :---: | :--- | :--- | :--- |
| **3.3V** | All Logic Circuits | VDD (3.3V) | 🔴 **Shared 3.3V Power Rail** |
| **GND** | All Logic Circuits | GND (Common) | 🔴 **Shared Common Ground Rail** |
| **5V / VIN** | DRV8833 VM Pin | Motor Power | 🔴 **Dedicated Motor Supply** |
| **GPIO 1** | MFRC522 RFID Reader | MISO | Dedicated SPI Data In |
| **GPIO 2** | MFRC522 RFID Reader | RST | Dedicated RFID Reset |
| **GPIO 3** | EC11 Rotary Encoder | ENC_SW | Dedicated Push Button Input |
| **GPIO 5** | Piezo Buzzer Positive (+) | BUZZER | Dedicated Audio PWM |
| **GPIO 6** | DRV8833 Motor Driver | IN4 | Dedicated Stepper Coil B- |
| **GPIO 7** | DRV8833 Motor Driver | IN3 | Dedicated Stepper Coil B+ |
| **GPIO 8** | EC11 Rotary Encoder | ENC_B | Dedicated Quadrature Interrupt B |
| **GPIO 9** | MFRC522 RFID Reader | SDA / CS | Dedicated RFID Chip Select |
| **GPIO 10** | DRV8833 Motor Driver | IN1 | Dedicated Stepper Coil A+ |
| **GPIO 11** | DRV8833 Motor Driver | IN2 | Dedicated Stepper Coil A- |
| **GPIO 12** | **TFT Display + RC522 RFID** | SCLK / SCK | 🔄 **Shared Hardware SPI Clock** |
| **GPIO 13** | **TFT Display + RC522 RFID** | MOSI / SDA | 🔄 **Shared Hardware SPI Master Out** |
| **GPIO 14** | ST7789 TFT Display | RST | Dedicated Display Reset |
| **GPIO 15** | ST7789 TFT Display | DC | Dedicated Display Data/Cmd |
| **GPIO 16** | ST7789 TFT Display | CS | Dedicated Display Chip Select |
| **GPIO 17** | ST7789 TFT Display | BLK | Dedicated Backlight Control |
| **GPIO 18** | EC11 Rotary Encoder | ENC_A | Dedicated Quadrature Interrupt A |
| **GPIO 41** | **TOF050C Stock Sensor** | SDA | 🔄 **Shared Hardware I2C Data** |
| **GPIO 42** | **TOF050C Stock Sensor** | SCL | 🔄 **Shared Hardware I2C Clock** |
| **GPIO 46** | Push Button (KEY0) to GND | KEY0 | Dedicated Navigation Back |
| **GPIO 4, 21, 38-40, 43-45** | Reserved Expansion | GPIO / UART | 🟢 **LEAVE FREE FOR EXPANSION** |
