# Aether — System Architecture

Cloud-connected multi-tenant sanitary vending machine management platform.

---

## High-Level Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│  ESP32 C3 Devices (Field)                                           │
│  WebSocket Client → sends telemetry, receives commands              │
└────────────────────────┬────────────────────────────────────────────┘
                         │ WSS (443)
                         ▼
┌─────────────────────────────────────────────────────────────────────┐
│  Azure App Service — Backend (Node.js)                              │
│  ├── Express REST API   (/api/*)                                    │
│  ├── WebSocket Server   (ws://)  ← same HTTP server                │
│  └── SQLite Database    (aether.db, WAL mode)                      │
└────────────────────────┬────────────────────────────────────────────┘
                         │ HTTPS REST + WSS
                         ▼
┌─────────────────────────────────────────────────────────────────────┐
│  cPanel Static Hosting — Frontend (HTML/CSS/JS)                     │
│  ├── index.html   (login)                                           │
│  ├── admin.html   (admin dashboard)                                 │
│  ├── company.html (company dashboard)                               │
│  └── app.js       (shared library)                                  │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Repository Structure

```
Aether/
├── backend/
│   ├── server.js          # Main Express + WebSocket server
│   ├── init_db.js         # One-time DB init & seed script
│   ├── package.json       # Dependencies & npm scripts
│   ├── .env.example       # Environment variable template
│   └── aether.db          # SQLite database (generated, not committed)
│
├── frontend/
│   ├── app.js             # Shared JS library (auth, WS, helpers, renderers)
│   ├── index.html         # Login page
│   ├── admin.html         # Admin dashboard (full access)
│   └── company.html       # Company dashboard (read-only, scoped)
│
└── ARCHITECTURE.md        # This file
```

---

## Backend — `backend/server.js`

### Tech Stack
| Package | Version | Purpose |
|---------|---------|---------|
| express | 4.x | REST API framework |
| ws | 8.x | WebSocket server |
| better-sqlite3 | latest | Synchronous SQLite (no native build issues) |
| bcryptjs | 2.4.3 | Password hashing (no native bindings) |
| jsonwebtoken | 9.x | JWT auth (HS256, 8h expiry) |
| helmet | latest | Security headers |
| express-rate-limit | latest | Login brute-force protection |
| cors | latest | Cross-origin requests |
| uuid | 9.x | Machine UUID generation |
| dotenv | latest | Environment variable loading |

### REST API Endpoints

| Method | Route | Auth | Description |
|--------|-------|------|-------------|
| POST | `/api/auth/login` | None (rate-limited) | Login → returns JWT |
| GET | `/api/auth/me` | JWT | Current user info |
| GET | `/api/machines` | JWT | List machines (filtered by role) |
| GET | `/api/machines/:id` | JWT | Machine detail + last 20 logs |
| POST | `/api/machines/:id/calibrate` | Admin JWT | Push calibrate command to device |
| GET | `/api/logs` | JWT | Activity log (filtered, `?machine_id=&limit=`) |
| GET | `/api/stats` | JWT | Counters (total/online/offline/errors/low_stock) |

### WebSocket Protocol

Single WS endpoint on the same HTTP server. Device vs browser distinguished by **first message content**:

```
Device (ESP32) first message:
  { "serial": "SVM-001", "key": "key_svm_001", "type": "telemetry", ... }

Browser first message:
  { "type": "auth", "token": "<JWT>" }
```

**Auth timeout:** 5 seconds — unauthenticated connections are closed.

**Telemetry fields (ESP32 → server):**
```json
{
  "serial": "SVM-001",
  "key": "key_svm_001",
  "type": "telemetry",
  "status": "online",
  "stock_count": 15,
  "pos_mm": 12,
  "motor_state": "idle",
  "wifi_rssi": -58,
  "error_code": null,
  "firmware_version": "v7.5"
}
```

**Server → browser message types:**
| Type | Description |
|------|-------------|
| `authenticated` | WS auth success |
| `machines_list` | Full machine list on connect |
| `machine_update` | Single machine state change |
| `alert` | Error or low-stock alert |
| `ack` | Calibration command acknowledged |
| `error` | Auth failure or bad message |

### Alert Logic
- Fires **only on state change** (not every telemetry frame)
- `error_code` changes to non-null → error alert
- `stock_count` crosses ≤ 5 threshold → low stock alert
- `LOW_STOCK_THRESH = 5`

### Database Schema

```sql
companies (id, name, created_at)

users (id, username, password_hash, role, company_id→companies, created_at)
  role: 'admin' | 'company'

machines (
  id UUID, serial_number, name, company_id→companies,
  device_api_key, status, stock_count, pos_mm,
  motor_state, wifi_rssi, error_code, firmware_version,
  last_seen, created_at
)
  status: 'online' | 'offline' | 'error'

activity_logs (id, machine_id→machines, event_type, message, triggered_by, created_at)
```

### Role-Based Access Control

| Role | Machines visible | Calibrate | Logs |
|------|-----------------|-----------|------|
| admin | All companies | ✅ | All |
| company | Own company only | ❌ | Own only |

---

## Frontend — `frontend/`

### `app.js` — Shared Library (global functions)

| Function | Description |
|----------|-------------|
| `authGuard(role)` | Redirect if not logged in or wrong role |
| `logout()` | Clear storage, close WS, redirect to login |
| `login(user, pass)` | POST to /api/auth/login, store JWT + user |
| `apiFetch(path, opts)` | Fetch wrapper with JWT header, auto-logout on 401 |
| `connectDashboardWS(callbacks)` | WS with exponential backoff reconnect (2s→30s) |
| `wsSend(msg)` | Send JSON to open WS |
| `renderMachineCard(machine, isAdmin)` | Full card HTML string |
| `updateMachineCard(machine, isAdmin)` | In-place DOM update (id=`mc-{machine.id}`) |
| `renderLogRow(log)` | Table row HTML |
| `applyStats(stats)` | Update stat card values by DOM id |
| `showToast(msg, type, ms)` | Toast notification (success/error/warning/info) |
| `stockColor(count)` | Returns `c-red` / `c-amber` / `c-green` |
| `rssiColor/rssiLabel/rssiBarCount` | WiFi signal strength helpers |
| `escHtml(str)` | XSS prevention |
| `formatRelativeTime(dateStr)` | "Just now" / "Xm ago" / "Xh ago" |
| `getToken/setToken/clearToken` | JWT localStorage helpers |
| `getUser/setUser/clearUser` | User object localStorage helpers |

### Pages

| File | Role Guard | Features |
|------|-----------|----------|
| `index.html` | None (redirects if logged in) | Login form, demo hint, role-based redirect after login |
| `admin.html` | `admin` | Full machine grid, stats, logs, calibrate modal |
| `company.html` | `company` | Read-only machine grid, stats, logs, no calibrate |

### Design System (Color Palette)

| Token | Value | Use |
|-------|-------|-----|
| `--bg` | `#0f1117` | Page background |
| `--card` | `#1a2030` | Card/panel background |
| `--border` | `#252f42` | Default borders |
| `--accent` | `#7dd3fc` | Serial numbers, highlights |
| `--blue` | `#2563eb` | Action buttons |
| `--green` | `#4ade80` | Online status, success (glow: `0 0 7px #4ade80`) |
| `--red` | `#ef4444` | Errors, alerts |
| `--amber` | `#f59e0b` | Warnings, low stock |
| `--text` | `#e2e8f0` | Primary text |
| `--muted` | `#64748b` | Secondary text |

---

## ESP32 Firmware Changes Required

Your existing local WebSocket prototype needs these changes to connect to the cloud:

### 1. Switch to WSS (secure WebSocket)
```cpp
// Add library: arduinoWebSockets by Markus Sattler
#include <WebSocketsClient.h>

WebSocketsClient ws;

// Replace local connection with:
ws.beginSSL("your-app.azurewebsites.net", 443, "/");
ws.setReconnectInterval(5000);
```

### 2. First message must include auth fields
```cpp
void sendTelemetry() {
  StaticJsonDocument<256> doc;
  doc["serial"]           = "SVM-001";         // must match DB
  doc["key"]              = "key_svm_001";      // must match DB
  doc["type"]             = "telemetry";
  doc["status"]           = "online";
  doc["stock_count"]      = stockCount;
  doc["pos_mm"]           = currentPosMm;
  doc["motor_state"]      = "idle";
  doc["wifi_rssi"]        = WiFi.RSSI();
  doc["error_code"]       = nullptr;            // or "MOTOR_JAM" etc
  doc["firmware_version"] = "v7.5";

  String out;
  serializeJson(doc, out);
  ws.sendTXT(out);
}
```

### 3. Handle incoming calibrate command
```cpp
void onWsEvent(WStype_t type, uint8_t* payload, size_t length) {
  if (type == WStype_TEXT) {
    StaticJsonDocument<128> doc;
    deserializeJson(doc, payload, length);
    if (doc["type"] == "calibrate") {
      int targetMm = doc["pos_mm"];
      moveMotorTo(targetMm);
    }
  }
}
```

### Device API Keys (from seed data)
| Serial | Key |
|--------|-----|
| SVM-001 | `key_svm_001` |
| SVM-002 | `key_svm_002` |
| SVM-003 | `key_svm_003` |

---

## Deployment

### Backend → Azure App Service

1. Create an Azure App Service (Node 18+ runtime, Linux)
2. Deploy the `backend/` folder (zip deploy or GitHub Actions)
3. Set **Application Settings** (Environment Variables):
   ```
   JWT_SECRET=<long-random-string-32+-chars>
   FRONTEND_ORIGIN=https://yourdomain.com
   PORT=8080
   ```
4. Via Kudu console or SSH, run **once**: `node init_db.js`
5. Start command: `npm start` (runs `node server.js`)

> **Note:** SQLite `aether.db` is stored on Azure's ephemeral disk. For production, migrate to Azure SQL or Postgres and swap `better-sqlite3` for `pg` or `mssql`.

### Frontend → cPanel Static Hosting

1. Upload all 4 files from `frontend/` to `public_html/` via File Manager or FTP
2. In each HTML file, uncomment and set the API base URL:
   ```html
   <!-- Near bottom of each HTML file: -->
   <script>window.AETHER_API_BASE = 'https://your-app.azurewebsites.net';</script>
   ```
3. `index.html` should be at your root URL (e.g. `https://yourdomain.com/`)

### Environment Variables Reference

| Variable | Required | Description |
|----------|----------|-------------|
| `JWT_SECRET` | ✅ | Strong random secret for JWT signing |
| `FRONTEND_ORIGIN` | ✅ | Exact origin of frontend (CORS) |
| `PORT` | Optional | Server port (default 3000, Azure uses 8080) |
| `SMTP_*` | Optional | Email alerts (currently console.log mock) |

---

## Security Notes

- Login endpoint is rate-limited (10 requests / 15 min per IP)
- Timing-safe auth: dummy bcrypt compare runs even for unknown usernames
- All user input passed to HTML is `escHtml()`-sanitized (XSS prevention)
- CORS restricted to `FRONTEND_ORIGIN` only
- `helmet` sets standard security headers
- Device API keys are plain strings — consider rotating them per-device in production
- SQLite WAL mode enabled for concurrent read performance
