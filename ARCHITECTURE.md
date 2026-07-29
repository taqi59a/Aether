# 🏛️ AETHER System Architecture & Engineering Specification

> **Version**: 2.6.0  
> **Target Microcontroller**: ESP32-S3-DevKitC-1 (240MHz Xtensa Dual-Core, 8MB Flash, 320KB SRAM)  
> **Firmware Framework**: Arduino ESP32 Framework v3.x / PlatformIO  

---

## 1. Hardware Pinout & Peripheral Mapping

| Component | Pin / Interface | Function / Notes |
| :--- | :--- | :--- |
| **ST7789 TFT Display** | SPI (MOSI=11, SCLK=12, CS=10, DC=13, RST=14, BL=15) | 240x320 2.0" Color Display (SPI 40MHz) |
| **RC522 RFID Reader** | SPI / GPIO (SDA=9, RST=2, MISO=1) | 13.56MHz Mifare Card Reader |
| **DRV8833 Motor Driver** | GPIO (IN1=4, IN2=6, IN3=7, IN4=8) | Dual H-Bridge 5V Stepper Control |
| **TOF050C / VL6180X Sensor** | I2C (SDA=41, SCL=42, Addr=0x29) | Time-of-Flight Stock Distance Sensor |
| **Quadrature Rotary Encoder**| GPIO (ENC_A=17, ENC_B=18, ENC_SW=16) | Interrupt-driven quadrature input + push button |
| **K0 Kill Switch** | GPIO 0 (BOOT Button / Pull-up) | Instant Emergency Stop / Back Button |
| **Piezo Buzzer** | GPIO 5 | Non-blocking tone audio alerts |
| **WS2812 RGB LED** | GPIO 48 & 38 | Built-in RGB Telemetry LED (Breathing Rainbow in Standby) |

---

## 2. Firmware Features & Visual Aesthetics

1. **High-Contrast Standby Header**:
   - Header box is rendered in **solid bright white (`C_WH`)** with a cyan accent border.
   - Logo title `"AETHER"` and subtitle `"VENDING CARE #5552"` are printed in **100% SOLID CRISP BLACK TEXT (`C_BK`)** for 100% maximum readability over the QR code!

2. **Cyberpunk Quantum Light Strike & Shatter Screensaver**:
   - **Phase 0: Convergence (1.2s)**: Dynamic vector energy lines (Neon Blue `C_CY` and Gold `C_GL`) shoot from the 4 screen corners towards the center, striking together with an expanding energy core ring and audio chime.
   - **Phase 1: Formation & Orbit (2.5s)**: Out of the strike collision, bold glowing text **"AETHER"** forms in the center with 4 orbiting plasma energy satellites rotating in 3D around the logo. Holds for 2.5 seconds.
   - **Phase 2: Shatter & Dispersion (1.2s)**: The logo shatters into 12 glowing cyan and gold light beam particles that burst back to the 4 screen corners.
   - **Seamless Loop**: Dispersed particles loop seamlessly back into Phase 0, striking together to reform "AETHER" repeatedly!
   - **Instant Interaction Exit**: Any button press, knob turn, RFID card scan, or Web command exits the screensaver instantly back to Standby.

3. **Secret Stealth Mode Toggle (`2x K0 + 2x Knob Button` within 5s)**:
   - Turns off display backlight, RGB LED, and power indicators while keeping Wi-Fi, Web Server, MQTT, and Board background loops running 100% active. Repeat code to turn back on!

4. **Persistent NVS Data Retention**:
   - Admin Cards, Wi-Fi credentials, stock distance calibration, and theme settings are stored in dedicated ESP32 flash NVS sectors (`admin-store`, `motor-store`, `wifi-store`). Firmware updates over USB or OTA **never delete user data or Wi-Fi settings**.
