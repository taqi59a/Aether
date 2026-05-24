'use strict';
/**
 * app.js — Aether Frontend Shared Library
 * Loaded by index.html, admin.html, and company.html
 *
 * ─── DEPLOYMENT CONFIG ───────────────────────────────────────────────────────
 * Before deploying to cPanel, set window.AETHER_API_BASE in a <script> tag
 * BEFORE this file is loaded, e.g.:
 *
 *   <script>
 *     window.AETHER_API_BASE = 'https://your-app.azurewebsites.net';
 *   </script>
 *   <script src="app.js"></script>
 * ─────────────────────────────────────────────────────────────────────────────
 */

const API_BASE = (typeof window !== 'undefined' && window.AETHER_API_BASE)
  || 'http://localhost:3000';

// Derive WS URL from API_BASE automatically (http→ws, https→wss)
const WS_URL = (typeof window !== 'undefined' && window.AETHER_WS_URL)
  || API_BASE.replace(/^http/, 'ws');

// ─────────────────────────────────────────────────────────────────────────────
// STORAGE HELPERS
// ─────────────────────────────────────────────────────────────────────────────
function getToken()   { return localStorage.getItem('aether_token'); }
function setToken(t)  { localStorage.setItem('aether_token', t); }
function clearToken() { localStorage.removeItem('aether_token'); }

function getUser() {
  try { return JSON.parse(localStorage.getItem('aether_user')); }
  catch { return null; }
}
function setUser(u) { localStorage.setItem('aether_user', JSON.stringify(u)); }
function clearUser() { localStorage.removeItem('aether_user'); }

// ─────────────────────────────────────────────────────────────────────────────
// AUTH HELPERS
// ─────────────────────────────────────────────────────────────────────────────
/**
 * Call at the top of every protected page.
 * Redirects to login if no valid session, or to the correct dashboard if wrong role.
 */
function authGuard(requiredRole) {
  const token = getToken();
  const user  = getUser();
  if (!token || !user) {
    window.location.replace('index.html');
    return false;
  }
  if (requiredRole && user.role !== requiredRole) {
    window.location.replace(user.role === 'admin' ? 'admin.html' : 'company.html');
    return false;
  }
  return true;
}

function logout() {
  clearToken();
  clearUser();
  if (_ws) { _ws.onclose = null; _ws.close(); _ws = null; }
  window.location.replace('index.html');
}

// ─────────────────────────────────────────────────────────────────────────────
// API FETCH WRAPPER
// ─────────────────────────────────────────────────────────────────────────────
async function apiFetch(path, options = {}) {
  const token = getToken();
  const res = await fetch(`${API_BASE}${path}`, {
    ...options,
    headers: {
      'Content-Type': 'application/json',
      ...(token ? { Authorization: `Bearer ${token}` } : {}),
      ...(options.headers || {}),
    },
  });

  if (res.status === 401) { logout(); return null; }

  if (!res.ok) {
    const err = await res.json().catch(() => ({ error: `HTTP ${res.status}` }));
    throw new Error(err.error || `HTTP ${res.status}`);
  }

  return res.json();
}

// ─────────────────────────────────────────────────────────────────────────────
// LOGIN
// ─────────────────────────────────────────────────────────────────────────────
async function login(username, password) {
  try {
    const res = await fetch(`${API_BASE}/api/auth/login`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ username, password }),
    });
    const json = await res.json();
    if (!res.ok) return { success: false, error: json.error };
    setToken(json.token);
    setUser(json.user);
    return { success: true, user: json.user };
  } catch {
    return { success: false, error: 'Cannot reach server. Check your connection.' };
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// WEBSOCKET — connects once, auto-reconnects with exponential back-off
// ─────────────────────────────────────────────────────────────────────────────
let _ws            = null;
let _wsCallbacks   = {};
let _wsRetryTimer  = null;
let _wsRetryDelay  = 2000;
let _wsConnected   = false;

/**
 * @param {Object} callbacks
 *   onStatus(state)          — 'connecting' | 'connected' | 'disconnected' | 'error'
 *   onMachinesList(machines)  — initial full list
 *   onMachineUpdate(machine)  — single machine state change
 *   onAlert(alert)            — { level, serial, name, message }
 *   onAck(message)            — command acknowledged
 */
function connectDashboardWS(callbacks = {}) {
  _wsCallbacks = callbacks;
  _wsDoConnect();
}

function _wsDoConnect() {
  if (_wsRetryTimer) { clearTimeout(_wsRetryTimer); _wsRetryTimer = null; }
  const token = getToken();
  if (!token) return;

  _wsCallbacks.onStatus?.('connecting');

  _ws = new WebSocket(WS_URL);

  _ws.onopen = () => {
    _wsConnected   = true;
    _wsRetryDelay  = 2000;
    _ws.send(JSON.stringify({ type: 'auth', token }));
  };

  _ws.onmessage = ({ data }) => {
    let msg;
    try { msg = JSON.parse(data); } catch { return; }

    switch (msg.type) {
      case 'authenticated':
        _wsCallbacks.onStatus?.('connected', msg.user);
        break;
      case 'machines_list':
        _wsCallbacks.onMachinesList?.(msg.machines);
        break;
      case 'machine_update':
        _wsCallbacks.onMachineUpdate?.(msg.machine);
        break;
      case 'alert':
        _wsCallbacks.onAlert?.(msg);
        break;
      case 'ack':
        _wsCallbacks.onAck?.(msg.message);
        break;
      case 'error':
        console.warn('[WS] Server:', msg.message);
        if (/token|auth|expired/i.test(msg.message)) logout();
        break;
    }
  };

  _ws.onclose = () => {
    _wsConnected = false;
    _wsCallbacks.onStatus?.('disconnected');
    _wsRetryTimer = setTimeout(() => {
      _wsRetryDelay = Math.min(_wsRetryDelay * 1.5, 30000);
      _wsDoConnect();
    }, _wsRetryDelay);
  };

  _ws.onerror = () => {
    _wsCallbacks.onStatus?.('error');
  };
}

/** Send a message to the server (browser → server direction). */
function wsSend(msg) {
  if (_ws && _ws.readyState === WebSocket.OPEN) {
    _ws.send(JSON.stringify(msg));
    return true;
  }
  return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// FORMATTING UTILITIES
// ─────────────────────────────────────────────────────────────────────────────
function formatRelativeTime(dateStr) {
  if (!dateStr) return 'Never';
  const diff = Date.now() - new Date(dateStr + (dateStr.endsWith('Z') ? '' : 'Z')).getTime();
  if (isNaN(diff) || diff < 0)  return 'Just now';
  if (diff < 60000)             return 'Just now';
  if (diff < 3600000)           return `${Math.floor(diff / 60000)}m ago`;
  if (diff < 86400000)          return `${Math.floor(diff / 3600000)}h ago`;
  return `${Math.floor(diff / 86400000)}d ago`;
}

function stockColor(count) {
  if (count === null || count === undefined) return 'muted';
  if (count <= 5)  return 'c-red';
  if (count <= 10) return 'c-amber';
  return 'c-green';
}

function rssiColor(rssi) {
  if (rssi === null || rssi === undefined) return 'c-muted';
  if (rssi > -65) return 'c-green';
  if (rssi > -75) return 'c-amber';
  return 'c-red';
}

function rssiLabel(rssi) {
  if (rssi === null || rssi === undefined) return '—';
  if (rssi > -50) return 'Excellent';
  if (rssi > -65) return 'Good';
  if (rssi > -75) return 'Fair';
  return 'Weak';
}

function rssiBarCount(rssi) {
  if (rssi > -50) return 4;
  if (rssi > -65) return 3;
  if (rssi > -75) return 2;
  return 1;
}

function statusLabel(status) {
  return { online: 'Online', error: 'Error', offline: 'Offline' }[status] || 'Unknown';
}

// ─────────────────────────────────────────────────────────────────────────────
// SECURITY: HTML ESCAPE (prevents XSS when inserting server data into the DOM)
// ─────────────────────────────────────────────────────────────────────────────
function escHtml(str) {
  if (str === null || str === undefined) return '';
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#039;');
}

// ─────────────────────────────────────────────────────────────────────────────
// RENDER: MACHINE CARD
// ─────────────────────────────────────────────────────────────────────────────
/**
 * Returns an HTML string for a machine card.
 * @param {Object} machine  — machine row from the API
 * @param {boolean} isAdmin — shows Calibrate button when true
 */
function renderMachineCard(machine, isAdmin = false) {
  const status   = machine.status || 'offline';
  const stock    = machine.stock_count ?? 20;
  const stockPct = Math.min(100, Math.max(0, (stock / 20) * 100));
  const posMm    = parseFloat(machine.pos_mm) || 0;
  const posPct   = Math.min(100, Math.max(0, (posMm / 76) * 100));
  const sCls     = stockColor(stock);
  const rssi     = machine.wifi_rssi;
  const rCls     = rssiColor(rssi);
  const bars     = rssiBarCount(rssi);

  const sigBars  = [1, 2, 3, 4]
    .map(i => `<div class="sig-bar ${i <= bars ? 'lit ' + rCls : ''}"></div>`)
    .join('');

  const calibBtn = isAdmin
    ? `<button class="mc-btn btn-calibrate"
         onclick="openCalibrateModal('${escHtml(machine.id)}','${escHtml(machine.serial_number)}',${posMm})">
         <svg viewBox="0 0 24 24" width="13" height="13" fill="none"
              stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
           <path d="M12 20h9M16.5 3.5a2.12 2.12 0 0 1 3 3L7 19l-4 1 1-4Z"/>
         </svg>
         Calibrate
       </button>`
    : '';

  const companyTag = (isAdmin && machine.company_name)
    ? `<div class="mc-company">${escHtml(machine.company_name)}</div>`
    : '';

  const errorRow = machine.error_code
    ? `<div class="mc-metric">
         <span class="mc-lbl">Error</span>
         <span class="mc-val c-red mono">${escHtml(machine.error_code)}</span>
       </div>`
    : '';

  const rssiRow = (rssi !== null && rssi !== undefined)
    ? `<div class="mc-metric">
         <span class="mc-lbl">WiFi Signal</span>
         <div class="mc-val mc-rssi-wrap">
           <div class="sig-bars">${sigBars}</div>
           <span class="${rCls}">${rssi} dBm&nbsp;<span class="c-muted">${rssiLabel(rssi)}</span></span>
         </div>
       </div>`
    : '';

  return `
    <div class="mc" id="mc-${escHtml(machine.id)}" data-id="${escHtml(machine.id)}" data-status="${escHtml(status)}">
      <div class="mc-head">
        <div class="mc-status-row">
          <span class="status-dot ${escHtml(status)}"></span>
          <span class="mc-serial">${escHtml(machine.serial_number)}</span>
          <span class="status-badge ${escHtml(status)}">${statusLabel(status)}</span>
        </div>
        <span class="mc-fw c-muted">${escHtml(machine.firmware_version || 'v7.5')}</span>
      </div>

      <div class="mc-name">${escHtml(machine.name || '—')}</div>
      ${companyTag}

      <div class="mc-metrics">
        <div class="mc-metric">
          <span class="mc-lbl">Stock</span>
          <div class="mc-val mc-bar-wrap">
            <div class="bar-track">
              <div class="bar-fill ${sCls}" style="width:${stockPct}%"></div>
            </div>
            <span class="${sCls}">${stock}<span class="c-muted">/20</span></span>
          </div>
        </div>

        <div class="mc-metric">
          <span class="mc-lbl">Position</span>
          <div class="mc-val mc-bar-wrap">
            <div class="bar-track">
              <div class="bar-fill c-blue" style="width:${posPct}%"></div>
            </div>
            <span class="c-accent">${posMm.toFixed(1)}<span class="c-muted"> mm</span></span>
          </div>
        </div>

        <div class="mc-metric">
          <span class="mc-lbl">Motor State</span>
          <span class="mc-val ${machine.motor_state === 'idle' ? 'c-green' : 'c-amber'}">
            ${escHtml(machine.motor_state || 'idle')}
          </span>
        </div>

        ${rssiRow}
        ${errorRow}
      </div>

      <div class="mc-foot">
        <span class="mc-last c-muted">Last seen: ${formatRelativeTime(machine.last_seen)}</span>
        <div class="mc-actions">${calibBtn}</div>
      </div>
    </div>`;
}

/**
 * Update an existing card in-place without re-rendering the grid.
 * Falls back to full re-render if card not found.
 */
function updateMachineCard(machine, isAdmin, allMachinesMap) {
  const el = document.getElementById(`mc-${machine.id}`);
  if (!el) return;  // will be added on next full render
  const newHtml = renderMachineCard(machine, isAdmin);
  const wrapper = document.createElement('div');
  wrapper.innerHTML = newHtml;
  el.replaceWith(wrapper.firstElementChild);
}

// ─────────────────────────────────────────────────────────────────────────────
// RENDER: LOG TABLE ROW
// ─────────────────────────────────────────────────────────────────────────────
const LOG_BADGE_CLASS = {
  telemetry: 'badge-blue',
  command:   'badge-blue',
  calibrate: 'badge-purple',
  dispense:  'badge-green',
  error:     'badge-red',
  alert:     'badge-amber',
};

function renderLogRow(log) {
  const cls = LOG_BADGE_CLASS[log.event_type] || 'badge-muted';
  return `
    <tr>
      <td><span class="badge ${cls}">${escHtml(log.event_type)}</span></td>
      <td class="c-muted mono">${escHtml(log.serial_number || log.machine_id?.slice(0, 8) || '—')}</td>
      <td>${escHtml(log.machine_name || '—')}</td>
      <td class="log-msg">${escHtml(log.message)}</td>
      <td class="c-muted">${escHtml(log.triggered_by || '—')}</td>
      <td class="c-muted" title="${escHtml(log.timestamp)}">${formatRelativeTime(log.timestamp)}</td>
    </tr>`;
}

// ─────────────────────────────────────────────────────────────────────────────
// TOAST NOTIFICATIONS
// ─────────────────────────────────────────────────────────────────────────────
function showToast(message, type = 'info', duration = 4500) {
  const area = document.getElementById('toastArea');
  if (!area) return;

  const icons = { success: '✓', error: '✕', warning: '⚠', info: 'ℹ' };
  const t = document.createElement('div');
  t.className = `toast toast-${type}`;
  t.innerHTML = `<span class="toast-icon">${icons[type] || 'ℹ'}</span><span>${escHtml(message)}</span>`;
  area.appendChild(t);

  requestAnimationFrame(() => requestAnimationFrame(() => t.classList.add('show')));

  setTimeout(() => {
    t.classList.remove('show');
    t.addEventListener('transitionend', () => t.remove(), { once: true });
  }, duration);
}

// ─────────────────────────────────────────────────────────────────────────────
// STATS BAR HELPER
// ─────────────────────────────────────────────────────────────────────────────
function applyStats(stats) {
  if (!stats) return;
  const set = (id, val) => {
    const el = document.getElementById(id);
    if (el) el.textContent = val ?? '—';
  };
  set('statTotal',    stats.total);
  set('statOnline',   stats.online);
  set('statOffline',  stats.offline);
  set('statErrors',   stats.errors);
  set('statLowStock', stats.low_stock);
}
