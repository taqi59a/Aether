# 🌟 AETHER Vending Machine - Feature Wishlist & Roadmap

This document captures all future software enhancements, architectural upgrades, and feature requests for the **AETHER Women's Sanitary Vending Machine** platform.

---

## 1. 🌐 Cloud Telemetry & Remote Fleet Management
- [ ] **Over-The-Air (OTA) Wireless Firmware Updates**:
  - Remote firmware binary uploads via embedded Web UI / WiFi without needing a USB cable.
- [ ] **Instant Low-Stock & Empty Alerts**:
  - Push notifications via **Telegram Bot API**, **Email (SMTP)**, or **Custom Webhooks** when stock drops below threshold (e.g. `< 15%`).
- [ ] **Multi-Machine Central Fleet Dashboard**:
  - Cloud map interface showing telemetry, stock levels, card tap audit logs, and online status across all deployed machines.

---

## 2. 📇 Advanced RFID Access Control & Security
- [ ] **Admin Master Card Mode**:
  - Tapping a designated Admin RFID card instantly unlocks settings or triggers a manual stock refill reset without needing the encoder knob.
- [ ] **Daily / Weekly Scan Quotas**:
  - Enforce per-card usage limits (e.g., maximum 1 dispense per card UID per 24 hours) to prevent inventory abuse.
- [ ] **Card Blacklisting / Whitelisting**:
  - Block lost or stolen card UIDs via REST API or embedded Web Dashboard.
- [ ] **Time-Based Access Schedules**:
  - Restrict dispensing to specific operating hours (e.g., automatic night or weekend lockouts).

---

## 3. ⚙️ Machine Reliability & Self-Healing Hardware
- [ ] **Anti-Jam Motor Auto-Recovery**:
  - Auto-detect mechanical jams via step torque stalling and execute an automated back-and-forth wiggle routine to clear the jam automatically.
- [ ] **I2C Bus & RFID Auto-Recovery Watchdog**:
  - Background self-healing loop that resets the I2C or SPI bus if sensor communication drops.
- [ ] **Power-Loss State Preservation**:
  - Instantly save ongoing dispense/motor state to NVS flash if brownout occurs.

---

## 4. 📱 User Experience & Localization
- [ ] **Dynamic QR Code Payload Configuration**:
  - Change the QR code URL dynamically via NVS/API without recompiling code.
- [ ] **Multi-Language UI Support**:
  - Switch UI languages between English, French, and Dutch.
- [ ] **1-Click Refill Helper Wizard**:
  - On-screen guided workflow for route operators to home motor, zero distance sensor, and reset stats.

---
*Created on: 2026-07-29*
