'use strict';
/**
 * server.js — Aether Cloud Backend
 * Target: Azure App Service (Node 18+)
 *
 * Handles:
 *  • REST API (auth, machines, logs, stats, calibrate)
 *  • WebSocket server on /ws — serves BOTH ESP32 devices (device API key)
 *    and browser dashboard clients (JWT). Bridges telemetry → live push.
 *  • Alert logic (console.log mock — swap sendAlertEmail body for nodemailer)
 */

// ─────────────────────────────────────────────────────────────────────────────
// 1. IMPORTS & CONFIG
// ─────────────────────────────────────────────────────────────────────────────
require('dotenv').config();

const express   = require('express');
const http      = require('http');
const WebSocket = require('ws');
const Database  = require('better-sqlite3');
const bcrypt    = require('bcryptjs');
const jwt       = require('jsonwebtoken');
const helmet    = require('helmet');
const rateLimit = require('express-rate-limit');
const cors      = require('cors');
const path      = require('path');

const PORT              = parseInt(process.env.PORT || '3000', 10);
const JWT_SECRET        = process.env.JWT_SECRET || 'aether-changeme-in-production';
const FRONTEND_ORIGIN   = process.env.FRONTEND_ORIGIN || '*';
const LOW_STOCK_THRESH  = 5;

if (JWT_SECRET === 'aether-changeme-in-production') {
  console.warn('[WARN] JWT_SECRET is using the default value. Set a strong secret in .env before deploying.');
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. DATABASE
// ─────────────────────────────────────────────────────────────────────────────
const DB_PATH = path.join(__dirname, 'aether.db');
let db;
try {
  db = new Database(DB_PATH);
} catch (err) {
  console.error(`[DB] Cannot open database at ${DB_PATH}. Run "node init_db.js" first.`);
  console.error(err.message);
  process.exit(1);
}
db.pragma('journal_mode = WAL');
db.pragma('foreign_keys = ON');

// ─────────────────────────────────────────────────────────────────────────────
// 3. EXPRESS + SECURITY MIDDLEWARE
// ─────────────────────────────────────────────────────────────────────────────
const app = express();

app.use(helmet({
  // CSP disabled on the API server — configure it on your cPanel frontend
  contentSecurityPolicy: false,
}));

app.use(cors({
  origin:          FRONTEND_ORIGIN,
  methods:         ['GET', 'POST', 'OPTIONS'],
  allowedHeaders:  ['Content-Type', 'Authorization'],
  credentials:     true,
}));

app.use(express.json({ limit: '50kb' }));

// Rate-limit login to 10 attempts per 15 minutes per IP
const loginLimiter = rateLimit({
  windowMs:       15 * 60 * 1000,
  max:            10,
  standardHeaders: true,
  legacyHeaders:  false,
  message:        { error: 'Too many login attempts. Please try again in 15 minutes.' },
});

// ─────────────────────────────────────────────────────────────────────────────
// 4. AUTH MIDDLEWARE
// ─────────────────────────────────────────────────────────────────────────────
/**
 * Verifies Bearer JWT. Optionally enforces a specific role.
 * Sets req.user = { id, username, role, company_id }
 */
function requireAuth(role = null) {
  return (req, res, next) => {
    const header = req.headers.authorization;
    if (!header || !header.startsWith('Bearer ')) {
      return res.status(401).json({ error: 'Unauthorized: missing token' });
    }
    try {
      const decoded = jwt.verify(header.slice(7), JWT_SECRET);
      req.user = decoded;
      if (role && decoded.role !== role) {
        return res.status(403).json({ error: 'Forbidden: insufficient permissions' });
      }
      next();
    } catch {
      return res.status(401).json({ error: 'Unauthorized: invalid or expired token' });
    }
  };
}

// ─────────────────────────────────────────────────────────────────────────────
// 5. ALERT LOGIC
// ─────────────────────────────────────────────────────────────────────────────
/**
 * Mock email sender.
 * Replace the console.log block with nodemailer / SendGrid / etc.
 */
function sendAlertEmail(machine, type) {
  const subject = type === 'error'
    ? `[AETHER ALERT] Machine Error: ${machine.serial_number}`
    : `[AETHER ALERT] Low Stock: ${machine.serial_number}`;

  const body = type === 'error'
    ? `Machine ${machine.serial_number} (${machine.name}) reporting error: ${machine.error_code}`
    : `Machine ${machine.serial_number} (${machine.name}) is low on stock — ${machine.stock_count} units remaining (threshold: ${LOW_STOCK_THRESH})`;

  // ── MOCK: swap this block with real email logic ──────────────────────────
  console.log('\n┌─────────────────────────────────────────────────────────────');
  console.log('│  📧  ALERT EMAIL  [MOCK — replace with nodemailer/SendGrid]');
  console.log(`│  To      : operations@company.com`);
  console.log(`│  Subject : ${subject}`);
  console.log(`│  Body    : ${body}`);
  console.log(`│  Time    : ${new Date().toISOString()}`);
  console.log('└─────────────────────────────────────────────────────────────\n');
  // ────────────────────────────────────────────────────────────────────────

  try {
    db.prepare(`
      INSERT INTO activity_logs (machine_id, event_type, message, triggered_by)
      VALUES (?, 'alert', ?, 'system')
    `).run(machine.id, body);
  } catch (e) {
    console.error('[ALERT] Failed to persist alert log:', e.message);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// 6. WEBSOCKET MANAGEMENT
// ─────────────────────────────────────────────────────────────────────────────
const httpServer = http.createServer(app);

// Single WS server — handles both ESP32 devices and browser clients
const wss = new WebSocket.Server({ server: httpServer });

// serial_number → WebSocket  (one active connection per physical device)
const deviceConnections = new Map();
// Set of { ws, user }  (all authenticated browser tabs)
const browserClients    = new Set();

// ── Helpers ──────────────────────────────────────────────────────────────────

function getMachinesForUser(user) {
  const sql = user.role === 'admin'
    ? `SELECT m.*, c.name AS company_name
       FROM machines m JOIN companies c ON m.company_id = c.id
       ORDER BY m.created_at ASC`
    : `SELECT m.*, c.name AS company_name
       FROM machines m JOIN companies c ON m.company_id = c.id
       WHERE m.company_id = ?
       ORDER BY m.created_at ASC`;
  return user.role === 'admin'
    ? db.prepare(sql).all()
    : db.prepare(sql).all(user.company_id);
}

function broadcastMachineUpdate(machineId) {
  const machine = db.prepare(`
    SELECT m.*, c.name AS company_name
    FROM machines m JOIN companies c ON m.company_id = c.id
    WHERE m.id = ?
  `).get(machineId);
  if (!machine) return;

  const payload = JSON.stringify({ type: 'machine_update', machine });

  for (const { ws, user } of browserClients) {
    if (ws.readyState !== WebSocket.OPEN) continue;
    if (user.role === 'admin' || user.company_id === machine.company_id) {
      ws.send(payload);
    }
  }
}

function broadcastAlert(machine, alertType, message) {
  const payload = JSON.stringify({
    type:       'alert',
    machine_id: machine.id,
    serial:     machine.serial_number,
    name:       machine.name,
    level:      alertType,
    message,
  });
  for (const { ws, user } of browserClients) {
    if (ws.readyState !== WebSocket.OPEN) continue;
    if (user.role === 'admin' || user.company_id === machine.company_id) {
      ws.send(payload);
    }
  }
}

/**
 * Persist one telemetry frame from an ESP32, trigger alerts if state changed,
 * then push the updated machine to all watching browsers.
 */
function processTelemetry(prevMachine, msg) {
  const newStatus = msg.error_code ? 'error' : 'online';

  db.prepare(`
    UPDATE machines SET
      status      = ?,
      stock_count = COALESCE(?, stock_count),
      pos_mm      = COALESCE(?, pos_mm),
      motor_state = COALESCE(?, motor_state),
      wifi_rssi   = ?,
      error_code  = ?,
      last_seen   = CURRENT_TIMESTAMP
    WHERE id = ?
  `).run(
    newStatus,
    msg.stock_count ?? null,
    msg.pos_mm      ?? null,
    msg.state       ?? null,
    msg.wifi_rssi   ?? null,
    msg.error_code  ?? null,
    prevMachine.id
  );

  db.prepare(`
    INSERT INTO activity_logs (machine_id, event_type, message, triggered_by)
    VALUES (?, 'telemetry', ?, 'device')
  `).run(
    prevMachine.id,
    `pos=${msg.pos_mm ?? '?'}mm state=${msg.state ?? '?'} stock=${msg.stock_count ?? '?'} rssi=${msg.wifi_rssi ?? '?'}dBm`
  );

  const updated = db.prepare('SELECT * FROM machines WHERE id = ?').get(prevMachine.id);

  // Trigger alerts only when state changes (avoids alert spam on every telemetry frame)
  const errorChanged  = updated.error_code !== prevMachine.error_code;
  const stockCrossed  = (prevMachine.stock_count > LOW_STOCK_THRESH)
                     && (updated.stock_count    <= LOW_STOCK_THRESH);

  if (errorChanged && updated.error_code) {
    const alertMsg = `Machine ${updated.serial_number} (${updated.name}) is reporting error: ${updated.error_code}`;
    sendAlertEmail(updated, 'error');
    broadcastAlert(updated, 'error', alertMsg);
  }
  if (stockCrossed) {
    const alertMsg = `Machine ${updated.serial_number} (${updated.name}) is low on stock (${updated.stock_count} units)`;
    sendAlertEmail(updated, 'low_stock');
    broadcastAlert(updated, 'low_stock', alertMsg);
  }

  broadcastMachineUpdate(prevMachine.id);
}

// ── WebSocket connection handler ──────────────────────────────────────────────
wss.on('connection', (ws, req) => {
  let clientType = null;  // 'device' | 'browser'
  let clientInfo = null;  // set on first-message auth

  // Close unauthenticated connections after 5 s
  const authTimeout = setTimeout(() => {
    if (!clientType) {
      ws.send(JSON.stringify({ type: 'error', message: 'Authentication timeout' }));
      ws.close(4001, 'Authentication timeout');
    }
  }, 5000);

  // Reject browsers from disallowed origins (ESP32 sends no Origin header — allow it)
  if (FRONTEND_ORIGIN !== '*') {
    const origin = req.headers.origin;
    if (origin && origin !== FRONTEND_ORIGIN) {
      clearTimeout(authTimeout);
      ws.close(4003, 'Origin not allowed');
      return;
    }
  }

  ws.on('message', (raw) => {
    let msg;
    try {
      msg = JSON.parse(raw.toString());
    } catch {
      ws.send(JSON.stringify({ type: 'error', message: 'Invalid JSON' }));
      return;
    }

    // ── FIRST MESSAGE: identify and authenticate the client ────────────────
    if (!clientType) {
      clearTimeout(authTimeout);

      // ── ESP32 device ──────────────────────────────────────────────────────
      if (msg.key && msg.serial) {
        const machine = db.prepare(
          'SELECT * FROM machines WHERE serial_number = ? AND device_api_key = ?'
        ).get(msg.serial, msg.key);

        if (!machine) {
          ws.send(JSON.stringify({ type: 'error', message: 'Invalid device credentials' }));
          ws.close(4001, 'Invalid credentials');
          return;
        }

        clientType = 'device';
        clientInfo = { serial: msg.serial, machineId: machine.id };
        deviceConnections.set(msg.serial, ws);

        ws.send(JSON.stringify({ type: 'ack', message: `Device ${msg.serial} authenticated` }));
        console.log(`[WS] Device connected: ${msg.serial}`);

        if (msg.type === 'telemetry') {
          processTelemetry(machine, msg);
        } else {
          // device_auth only — mark online, no telemetry data yet
          db.prepare("UPDATE machines SET status = 'online', last_seen = CURRENT_TIMESTAMP WHERE id = ?")
            .run(machine.id);
          broadcastMachineUpdate(machine.id);
        }
        return;
      }

      // ── Browser dashboard ─────────────────────────────────────────────────
      if (msg.type === 'auth' && msg.token) {
        try {
          const user    = jwt.verify(msg.token, JWT_SECRET);
          clientType    = 'browser';
          const ref     = { ws, user };
          clientInfo    = { user, ref };
          browserClients.add(ref);

          ws.send(JSON.stringify({
            type: 'authenticated',
            user: { id: user.id, username: user.username, role: user.role, company_id: user.company_id },
          }));

          // Immediately push the current machine list
          ws.send(JSON.stringify({
            type:     'machines_list',
            machines: getMachinesForUser(user),
          }));

          console.log(`[WS] Browser connected: ${user.username} (${user.role})`);
        } catch {
          ws.send(JSON.stringify({ type: 'error', message: 'Invalid or expired token' }));
          ws.close(4001, 'Invalid token');
        }
        return;
      }

      ws.send(JSON.stringify({ type: 'error', message: 'First message must contain {type:"auth",token} or device credentials {serial,key}' }));
      ws.close(4002, 'Bad handshake');
      return;
    }

    // ── SUBSEQUENT MESSAGES ────────────────────────────────────────────────

    if (clientType === 'device') {
      const machine = db.prepare('SELECT * FROM machines WHERE id = ?').get(clientInfo.machineId);
      if (machine) processTelemetry(machine, msg);
      return;
    }

    if (clientType === 'browser') {
      const { user } = clientInfo;

      if (msg.type === 'calibrate') {
        if (user.role !== 'admin') {
          ws.send(JSON.stringify({ type: 'error', message: 'Forbidden: admin only' }));
          return;
        }
        const machine = db.prepare('SELECT * FROM machines WHERE id = ?').get(msg.machine_id);
        if (!machine) {
          ws.send(JSON.stringify({ type: 'error', message: 'Machine not found' }));
          return;
        }
        const targetMm = Math.min(76, Math.max(0, parseInt(msg.target_mm) || 12));
        const deviceWs = deviceConnections.get(machine.serial_number);

        if (deviceWs && deviceWs.readyState === WebSocket.OPEN) {
          deviceWs.send(JSON.stringify({ type: 'command', action: 'calibrate', target_mm: targetMm }));
          ws.send(JSON.stringify({ type: 'ack', message: `Calibrate command sent to ${machine.serial_number}` }));
        } else {
          ws.send(JSON.stringify({ type: 'ack', message: `Command logged — ${machine.serial_number} is offline` }));
        }

        db.prepare(`
          INSERT INTO activity_logs (machine_id, event_type, message, triggered_by)
          VALUES (?, 'calibrate', ?, ?)
        `).run(machine.id, `Calibrate to ${targetMm}mm`, user.username);
      }
    }
  });

  ws.on('close', () => {
    clearTimeout(authTimeout);
    if (clientType === 'device' && clientInfo) {
      deviceConnections.delete(clientInfo.serial);
      db.prepare("UPDATE machines SET status = 'offline' WHERE serial_number = ?")
        .run(clientInfo.serial);
      console.log(`[WS] Device disconnected: ${clientInfo.serial}`);
      broadcastMachineUpdate(clientInfo.machineId);
    }
    if (clientType === 'browser' && clientInfo) {
      browserClients.delete(clientInfo.ref);
      console.log(`[WS] Browser disconnected: ${clientInfo.user.username}`);
    }
  });

  ws.on('error', (err) => {
    console.error('[WS] Client error:', err.message);
  });
});

// ─────────────────────────────────────────────────────────────────────────────
// 7. REST API ROUTES
// ─────────────────────────────────────────────────────────────────────────────

// ── POST /api/auth/login ─────────────────────────────────────────────────────
app.post('/api/auth/login', loginLimiter, (req, res) => {
  const { username, password } = req.body;

  if (!username || !password
      || typeof username !== 'string'
      || typeof password !== 'string') {
    return res.status(400).json({ error: 'Username and password are required' });
  }

  // Input validation — only allow safe username characters
  if (!/^[a-zA-Z0-9_]{1,50}$/.test(username)) {
    return res.status(401).json({ error: 'Invalid credentials' });
  }

  const user = db.prepare('SELECT * FROM users WHERE username = ?').get(username);

  if (!user) {
    // Constant-time dummy compare to prevent timing-based username enumeration
    bcrypt.compareSync('dummy', '$2a$12$AAAAAAAAAAAAAAAAAAAAAA.AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA');
    return res.status(401).json({ error: 'Invalid credentials' });
  }

  if (!bcrypt.compareSync(password, user.password_hash)) {
    return res.status(401).json({ error: 'Invalid credentials' });
  }

  const token = jwt.sign(
    { id: user.id, username: user.username, role: user.role, company_id: user.company_id },
    JWT_SECRET,
    { expiresIn: '8h' }
  );

  console.log(`[AUTH] Login: ${user.username} (${user.role})`);
  res.json({
    token,
    user: { id: user.id, username: user.username, role: user.role, company_id: user.company_id },
  });
});

// ── GET /api/auth/me ─────────────────────────────────────────────────────────
app.get('/api/auth/me', requireAuth(), (req, res) => {
  const user = db.prepare('SELECT id, username, role, company_id FROM users WHERE id = ?')
                 .get(req.user.id);
  if (!user) return res.status(404).json({ error: 'User not found' });
  res.json(user);
});

// ── GET /api/machines ────────────────────────────────────────────────────────
app.get('/api/machines', requireAuth(), (req, res) => {
  res.json(getMachinesForUser(req.user));
});

// ── GET /api/machines/:id ────────────────────────────────────────────────────
app.get('/api/machines/:id', requireAuth(), (req, res) => {
  const machine = db.prepare(`
    SELECT m.*, c.name AS company_name
    FROM machines m JOIN companies c ON m.company_id = c.id
    WHERE m.id = ?
  `).get(req.params.id);

  if (!machine) return res.status(404).json({ error: 'Machine not found' });

  if (req.user.role !== 'admin' && machine.company_id !== req.user.company_id) {
    return res.status(403).json({ error: 'Forbidden' });
  }

  const logs = db.prepare(
    'SELECT * FROM activity_logs WHERE machine_id = ? ORDER BY timestamp DESC LIMIT 20'
  ).all(req.params.id);

  res.json({ ...machine, logs });
});

// ── POST /api/machines/:id/calibrate ────────────────────────────────────────
app.post('/api/machines/:id/calibrate', requireAuth('admin'), (req, res) => {
  const machine = db.prepare('SELECT * FROM machines WHERE id = ?').get(req.params.id);
  if (!machine) return res.status(404).json({ error: 'Machine not found' });

  const targetMm  = Math.min(76, Math.max(0, parseInt(req.body.target_mm) || 12));
  const deviceWs  = deviceConnections.get(machine.serial_number);
  const isOnline  = deviceWs && deviceWs.readyState === WebSocket.OPEN;

  if (isOnline) {
    deviceWs.send(JSON.stringify({ type: 'command', action: 'calibrate', target_mm: targetMm }));
  }

  db.prepare(`
    INSERT INTO activity_logs (machine_id, event_type, message, triggered_by)
    VALUES (?, 'calibrate', ?, ?)
  `).run(machine.id, `Calibrate to ${targetMm}mm`, req.user.username);

  console.log(`[API] Calibrate ${machine.serial_number} → ${targetMm}mm by ${req.user.username} (device ${isOnline ? 'online' : 'offline'})`);

  res.json({
    success:       true,
    device_online: isOnline,
    message:       isOnline
      ? `Calibrate command sent to ${machine.serial_number}`
      : `Command logged — ${machine.serial_number} is currently offline`,
  });
});

// ── GET /api/logs ────────────────────────────────────────────────────────────
app.get('/api/logs', requireAuth(), (req, res) => {
  const limit     = Math.min(parseInt(req.query.limit) || 50, 200);
  const machineId = req.query.machine_id || null;

  const base = `
    SELECT l.*, m.serial_number, m.name AS machine_name
    FROM activity_logs l
    JOIN machines m ON l.machine_id = m.id
  `;

  let rows;
  if (req.user.role === 'admin') {
    rows = machineId
      ? db.prepare(`${base} WHERE l.machine_id = ? ORDER BY l.timestamp DESC LIMIT ?`).all(machineId, limit)
      : db.prepare(`${base} ORDER BY l.timestamp DESC LIMIT ?`).all(limit);
  } else {
    rows = machineId
      ? db.prepare(`${base} WHERE l.machine_id = ? AND m.company_id = ? ORDER BY l.timestamp DESC LIMIT ?`).all(machineId, req.user.company_id, limit)
      : db.prepare(`${base} WHERE m.company_id = ? ORDER BY l.timestamp DESC LIMIT ?`).all(req.user.company_id, limit);
  }

  res.json(rows);
});

// ── GET /api/stats ───────────────────────────────────────────────────────────
app.get('/api/stats', requireAuth(), (req, res) => {
  const isAdmin = req.user.role === 'admin';
  const cid     = req.user.company_id;

  const q = (sql, ...args) => db.prepare(sql).get(...args).c;

  if (isAdmin) {
    return res.json({
      total:     q('SELECT COUNT(*) AS c FROM machines'),
      online:    q("SELECT COUNT(*) AS c FROM machines WHERE status = 'online'"),
      offline:   q("SELECT COUNT(*) AS c FROM machines WHERE status = 'offline'"),
      errors:    q("SELECT COUNT(*) AS c FROM machines WHERE status = 'error'"),
      low_stock: q(`SELECT COUNT(*) AS c FROM machines WHERE stock_count <= ${LOW_STOCK_THRESH}`),
    });
  }

  return res.json({
    total:     q('SELECT COUNT(*) AS c FROM machines WHERE company_id = ?', cid),
    online:    q("SELECT COUNT(*) AS c FROM machines WHERE company_id = ? AND status = 'online'", cid),
    offline:   q("SELECT COUNT(*) AS c FROM machines WHERE company_id = ? AND status = 'offline'", cid),
    errors:    q("SELECT COUNT(*) AS c FROM machines WHERE company_id = ? AND status = 'error'", cid),
    low_stock: q(`SELECT COUNT(*) AS c FROM machines WHERE company_id = ? AND stock_count <= ${LOW_STOCK_THRESH}`, cid),
  });
});

// ─────────────────────────────────────────────────────────────────────────────
// 8. SERVER START
// ─────────────────────────────────────────────────────────────────────────────
httpServer.listen(PORT, () => {
  console.log('╔══════════════════════════════════════════════════════════╗');
  console.log('║  AETHER — CLOUD VENDING MACHINE BACKEND                  ║');
  console.log('╠══════════════════════════════════════════════════════════╣');
  console.log(`║  REST   →  http://localhost:${PORT}/api                    ║`);
  console.log(`║  WS     →  ws://localhost:${PORT}   (devices + browsers)   ║`);
  console.log(`║  DB     →  ${DB_PATH}`);
  console.log('╠══════════════════════════════════════════════════════════╣');
  console.log('║  WS Protocol:                                            ║');
  console.log('║   Device  first msg → { serial, key, type:"telemetry" } ║');
  console.log('║   Browser first msg → { type:"auth", token:"<JWT>" }    ║');
  console.log('╚══════════════════════════════════════════════════════════╝\n');
});

// Graceful shutdown
process.on('SIGTERM', () => {
  console.log('[SERVER] SIGTERM received — shutting down gracefully...');
  httpServer.close(() => { db.close(); process.exit(0); });
});
process.on('SIGINT', () => {
  httpServer.close(() => { db.close(); process.exit(0); });
});
