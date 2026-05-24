'use strict';
/**
 * init_db.js — Run once to create the SQLite schema and seed dummy data.
 * Usage: node init_db.js
 */

const Database = require('better-sqlite3');
const bcrypt   = require('bcryptjs');
const { v4: uuidv4 } = require('uuid');
const path     = require('path');

const DB_PATH  = path.join(__dirname, 'aether.db');
const db       = new Database(DB_PATH);

db.pragma('journal_mode = WAL');
db.pragma('foreign_keys = ON');

// ─────────────────────────────────────────────────────────────────────────────
// SCHEMA
// ─────────────────────────────────────────────────────────────────────────────
db.exec(`
  CREATE TABLE IF NOT EXISTS companies (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    name       TEXT    NOT NULL,
    email      TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
  );

  CREATE TABLE IF NOT EXISTS users (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    username      TEXT    UNIQUE NOT NULL,
    password_hash TEXT    NOT NULL,
    role          TEXT    NOT NULL CHECK(role IN ('admin', 'company')),
    company_id    INTEGER REFERENCES companies(id),
    created_at    DATETIME DEFAULT CURRENT_TIMESTAMP
  );

  CREATE TABLE IF NOT EXISTS machines (
    id               TEXT    PRIMARY KEY,
    serial_number    TEXT    UNIQUE NOT NULL,
    name             TEXT,
    company_id       INTEGER NOT NULL REFERENCES companies(id),
    device_api_key   TEXT    UNIQUE NOT NULL,
    status           TEXT    DEFAULT 'offline' CHECK(status IN ('online','offline','error')),
    stock_count      INTEGER DEFAULT 20,
    pos_mm           REAL    DEFAULT 12,
    motor_state      TEXT    DEFAULT 'idle',
    wifi_rssi        INTEGER,
    error_code       TEXT,
    firmware_version TEXT    DEFAULT 'v7.5',
    last_seen        DATETIME,
    created_at       DATETIME DEFAULT CURRENT_TIMESTAMP
  );

  CREATE TABLE IF NOT EXISTS activity_logs (
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    machine_id   TEXT    REFERENCES machines(id),
    event_type   TEXT    NOT NULL,
    message      TEXT    NOT NULL,
    triggered_by TEXT,
    timestamp    DATETIME DEFAULT CURRENT_TIMESTAMP
  );
`);
console.log('[DB] Tables created (or already exist)');

// ─────────────────────────────────────────────────────────────────────────────
// GUARD: only seed if DB is empty
// ─────────────────────────────────────────────────────────────────────────────
const alreadySeeded = db.prepare('SELECT COUNT(*) AS c FROM companies').get().c > 0;
if (alreadySeeded) {
  console.log('[DB] Already seeded — skipping. Delete aether.db to re-seed.\n');
  db.close();
  process.exit(0);
}

// ─────────────────────────────────────────────────────────────────────────────
// SEED: COMPANIES
// ─────────────────────────────────────────────────────────────────────────────
const insertCompany = db.prepare('INSERT INTO companies (name, email) VALUES (?, ?)');
const { lastInsertRowid: demoId  } = insertCompany.run('Demo Corp',   'admin@democorp.com');
const { lastInsertRowid: betaId  } = insertCompany.run('Beta Clinic', 'info@betaclinic.com');
console.log(`[DB] Companies seeded (Demo Corp id=${demoId}, Beta Clinic id=${betaId})`);

// ─────────────────────────────────────────────────────────────────────────────
// SEED: USERS  (admin + company, both password = "12345")
// ─────────────────────────────────────────────────────────────────────────────
const SALT_ROUNDS = 12;
console.log('[DB] Hashing passwords (bcrypt, 12 rounds) — this takes a moment...');
const adminHash   = bcrypt.hashSync('12345', SALT_ROUNDS);
const companyHash = bcrypt.hashSync('12345', SALT_ROUNDS);

const insertUser = db.prepare(
  'INSERT INTO users (username, password_hash, role, company_id) VALUES (?, ?, ?, ?)'
);
insertUser.run('admin',   adminHash,   'admin',   null);
insertUser.run('company', companyHash, 'company', demoId);
console.log('[DB] Users seeded  →  admin/12345 (admin)  |  company/12345 (company → Demo Corp)');

// ─────────────────────────────────────────────────────────────────────────────
// SEED: MACHINES
// ─────────────────────────────────────────────────────────────────────────────
const m1 = uuidv4();
const m2 = uuidv4();
const m3 = uuidv4();

const insertMachine = db.prepare(`
  INSERT INTO machines
    (id, serial_number, name, company_id, device_api_key,
     status, stock_count, pos_mm, motor_state, wifi_rssi, error_code, firmware_version, last_seen)
  VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, datetime('now', ?))
`);

//                     id   serial     name                cid      key              status    stock  pos   state    rssi    err           fw       last_seen offset
insertMachine.run(m1, 'SVM-001', 'Lobby Unit A',    demoId, 'key_svm_001', 'online',  18, 12.0, 'idle',  -52, null,         'v7.5', '-3 minutes');
insertMachine.run(m2, 'SVM-002', 'Restroom Unit B', demoId, 'key_svm_002', 'error',    3, 41.0, 'idle',  -68, 'MOTOR_JAM', 'v7.5', '-12 minutes');
insertMachine.run(m3, 'SVM-003', 'Clinic Unit C',   betaId, 'key_svm_003', 'offline', 20, 12.0, 'idle',  null, null,        'v7.4', '-2 hours');

console.log(`[DB] Machines seeded:
     SVM-001 → Demo Corp  | Online  | Stock 18 | key_svm_001
     SVM-002 → Demo Corp  | Error   | Stock  3 | key_svm_002  (MOTOR_JAM, low stock)
     SVM-003 → Beta Clinic| Offline | Stock 20 | key_svm_003`);

// ─────────────────────────────────────────────────────────────────────────────
// SEED: ACTIVITY LOGS
// ─────────────────────────────────────────────────────────────────────────────
const insertLog = db.prepare(
  "INSERT INTO activity_logs (machine_id, event_type, message, triggered_by, timestamp) VALUES (?, ?, ?, ?, datetime('now', ?))"
);

// SVM-001 logs
insertLog.run(m1, 'dispense',  'Pad dispensed successfully via RFID tap',          'device', '-1 hour');
insertLog.run(m1, 'dispense',  'Pad dispensed successfully via RFID tap',          'device', '-3 hours');
insertLog.run(m1, 'calibrate', 'Calibrate to 12mm requested by admin',             'admin',  '-2 days');
insertLog.run(m1, 'telemetry', 'pos=12mm state=idle stock=18 rssi=-52dBm',        'device', '-3 minutes');

// SVM-002 logs
insertLog.run(m2, 'error',    'Motor jam detected at position 41mm',               'device', '-12 minutes');
insertLog.run(m2, 'alert',    'ERROR ALERT: Machine SVM-002 reporting error: MOTOR_JAM', 'system', '-12 minutes');
insertLog.run(m2, 'alert',    'LOW STOCK ALERT: Machine SVM-002 has only 3 units', 'system', '-12 minutes');
insertLog.run(m2, 'dispense', 'Pad dispensed successfully',                        'device', '-1 day');

// SVM-003 logs
insertLog.run(m3, 'telemetry', 'pos=12mm state=idle stock=20 rssi=-61dBm',        'device', '-2 hours');
insertLog.run(m3, 'telemetry', 'Device went offline — last heartbeat',             'system', '-2 hours');

console.log('[DB] Activity logs seeded');

db.close();

console.log(`
✅  Database initialized: ${DB_PATH}
────────────────────────────────────────────
  Login accounts:
    admin   / 12345  (admin role)
    company / 12345  (company role → Demo Corp)

  Device API keys (for ESP32 firmware):
    SVM-001 → key_svm_001
    SVM-002 → key_svm_002
    SVM-003 → key_svm_003

  Next step: node server.js
────────────────────────────────────────────`);
