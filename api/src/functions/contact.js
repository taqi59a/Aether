'use strict';
const { app } = require('@azure/functions');
const { EmailClient } = require('@azure/communication-email');
const { confirmation, notification } = require('../templates');

const ALLOWED = (process.env.ALLOWED_ORIGINS || 'https://myaether.org,https://www.myaether.org').split(',').map((s) => s.trim());
const NOTIFY_TO = process.env.NOTIFY_TO || 'info@myaether.org';
const LANGS = ['en', 'fr', 'nl', 'de'];
const EMAIL_RE = /^[^\s@<>"',;:()\[\]\\]+@[^\s@<>"',;:()\[\]\\]+\.[A-Za-z]{2,}$/;

// very small in-memory throttle (per instance): 5 sends / 10 min / IP
const hits = new Map();
function throttled(ip) {
  const now = Date.now(), win = 10 * 60 * 1000;
  const arr = (hits.get(ip) || []).filter((t) => now - t < win);
  arr.push(now); hits.set(ip, arr);
  if (hits.size > 5000) hits.clear();
  return arr.length > 5;
}

function cors(origin) {
  const ok = ALLOWED.includes(origin);
  return ok ? {
    'Access-Control-Allow-Origin': origin,
    'Access-Control-Allow-Methods': 'POST, OPTIONS',
    'Access-Control-Allow-Headers': 'Content-Type',
    'Access-Control-Max-Age': '86400',
    Vary: 'Origin',
  } : { Vary: 'Origin' };
}
const clean = (v, max) => String(v == null ? '' : v).replace(/[\u0000-\u0008\u000B\u000C\u000E-\u001F]/g, '').trim().slice(0, max);
const oneLine = (v, max) => clean(v, max).replace(/[\r\n\t]+/g, ' ');

app.http('contact', {
  methods: ['POST', 'OPTIONS'],
  authLevel: 'anonymous',
  route: 'contact',
  handler: async (req, ctx) => {
    const origin = req.headers.get('origin') || '';
    const headers = { 'Content-Type': 'application/json', 'Cache-Control': 'no-store', ...cors(origin) };
    if (req.method === 'OPTIONS') return { status: 204, headers };
    if (!ALLOWED.includes(origin)) return { status: 403, headers, jsonBody: { ok: false, error: 'origin' } };

    let b;
    try { b = await req.json(); } catch { return { status: 400, headers, jsonBody: { ok: false, error: 'json' } }; }

    // bots: honeypot filled, or form submitted impossibly fast
    const age = Date.now() - Number(b.t || 0);
    if (clean(b.website, 200) || !(age > 3000 && age < 86400000)) {
      return { status: 200, headers, jsonBody: { ok: true } }; // pretend success, send nothing
    }

    const d = {
      name: oneLine(b.name, 100),
      email: oneLine(b.email, 200),
      topic: oneLine(b.topic, 80) || 'Other',
      message: clean(b.message, 5000),
      lang: LANGS.includes(b.lang) ? b.lang : 'en',
    };
    if (!d.name || !EMAIL_RE.test(d.email) || d.message.length < 10) {
      return { status: 422, headers, jsonBody: { ok: false, error: 'invalid' } };
    }
    const ip = (req.headers.get('x-forwarded-for') || '').split(',')[0].trim() || 'unknown';
    if (throttled(ip)) return { status: 429, headers, jsonBody: { ok: false, error: 'rate' } };

    try {
      const client = new EmailClient(process.env.ACS_CONNECTION_STRING);
      const sender = process.env.SENDER_ADDRESS;
      const meta = { when: new Date().toLocaleString('en-GB', { timeZone: 'Europe/Brussels', dateStyle: 'full', timeStyle: 'short' }) + ' (Brussels)' };

      // 1) notify us (reply goes straight to the visitor)
      const n = notification(d, meta);
      await client.beginSend({
        senderAddress: sender,
        content: { subject: n.subject, plainText: n.text, html: n.html },
        recipients: { to: [{ address: NOTIFY_TO }] },
        replyTo: [{ address: d.email, displayName: d.name.replace(/[<>",;]/g, '') }],
      });

      // 2) automatic FR/EN confirmation to the visitor
      const c = confirmation(d);
      await client.beginSend({
        senderAddress: sender,
        content: { subject: c.subject, plainText: c.text, html: c.html },
        recipients: { to: [{ address: d.email }] },
        replyTo: [{ address: NOTIFY_TO, displayName: 'Aether EdTech' }],
      });
      // privacy: nothing about the visitor or message content is logged
      ctx.log('contact: sent');
      return { status: 200, headers, jsonBody: { ok: true } };
    } catch (e) {
      ctx.error('contact: send failed', e && e.code ? e.code : 'error');
      return { status: 502, headers, jsonBody: { ok: false, error: 'send' } };
    }
  },
});
