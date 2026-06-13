# Hardware Components Configuration (ESP32-C3 Super Mini)

This document maps all the hardware components and pin assignments used in the Aether Vending Machine project.

---

## 1. Microcontroller
* **Board:** ESP32-C3 Super Mini
* **SoC:** Espressif ESP32-C3 (Single-core RISC-V @ 160MHz, built-in Wi-Fi and BLE)

---

## 2. Display Module (OLED)
* **Component:** 0.91" SSD1306 OLED Display (128x32 px)
* **Protocol:** I2C (Wire) running at 800 kHz
* **Pinout:**
  * **SCL (I2C Clock):** `GPIO 20`
  * **SDA (I2C Data):** `GPIO 21`
  * **Reset:** `None` (Software reset / -1)

---

## 3. RFID Sensor Module (RC522)
* **Component:** MFRC522 RFID Card Reader
* **Protocol:** Hardware SPI
* **Pinout:**
  * **SDA / SS (Chip Select):** `GPIO 0`
  * **SCK (SPI Clock):** `GPIO 3`
  * **MOSI (SPI Master Out Slave In):** `GPIO 1`
  * **MISO (SPI Master In Slave Out):** `GPIO 2`
  * **RST (Reset):** `GPIO 10`

---

## 4. Motor Driver & Stepper Motor
* **Component:** Stepper Motor (4800 half-steps / 60mm travel) driven by a **DRV8833** Dual H-Bridge Driver
* **Pinout:**
  * **AIN1 (A-phase input 1):** `GPIO 5`
  * **AIN2 (A-phase input 2):** `GPIO 6`
  * **BIN1 (B-phase input 1):** `GPIO 7`
  * **BIN2 (B-phase input 2):** `GPIO 8`

---

## 5. Acoustic Indicator (Buzzer)
* **Component:** Passive Piezo Buzzer
* **Pinout:**
  * **Buzzer Control Pin:** `GPIO 4`
