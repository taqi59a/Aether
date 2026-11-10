'use strict';
// Bilingual (FR + EN) HTML confirmation e-mail for Aether EdTech.
// Table layout + inline CSS for maximum e-mail client compatibility.

const SITE = process.env.SITE_URL || 'https://myaether.org';
const MAIL = process.env.NOTIFY_TO || 'info@myaether.org';

const C = {
  paper: '#f7f2e8', ink: '#17233b', soft: '#4a5468', blue: '#1f4f9c', deep: '#14376f',
  wash: '#e3eaf5', clay: '#b8472f', rule: '#d9d0bd', white: '#fffdf8',
};
const SERIF = "Georgia,'Times New Roman',serif";
const SANS = "'Helvetica Neue',Helvetica,Arial,sans-serif";

const esc = (s) => String(s).replace(/[&<>"']/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
const nl2br = (s) => esc(s).replace(/\r?\n/g, '<br>');
const clip = (s, n) => (s.length > n ? s.slice(0, n).trimEnd() + '…' : s);

const COPY = {
  fr: {
    tag: 'Français',
    hello: (n) => `Merci, ${n}.`,
    lead: 'Nous avons bien reçu votre demande et nous vous répondrons dès que possible.',
    sub: 'Une vraie personne de l’équipe Aether EdTech la lit en ce moment même. Rien à faire de votre côté.',
    nextH: 'Et maintenant ?',
    steps: [
      ['Reçu', 'Votre message est bien arrivé dans notre boîte.'],
      ['Lu par une personne', 'L’un de nos fondateurs prend connaissance de votre demande.'],
      ['Réponse personnelle', 'Vous recevez notre réponse depuis ' + MAIL + '.'],
    ],
    yourH: 'Votre message',
    topicL: 'Sujet',
    cta: 'Découvrir nos projets',
    ctaUrl: SITE + '/fr/projects.html',
    note: 'Une question urgente ? Répondez simplement à cet e-mail.',
  },
  en: {
    tag: 'English',
    hello: (n) => `Thank you, ${n}.`,
    lead: 'We have received your enquiry and will reply as soon as possible.',
    sub: 'A real person on the Aether EdTech team is reading it right now. There is nothing else you need to do.',
    nextH: 'What happens next',
    steps: [
      ['Received', 'Your message has safely reached our inbox.'],
      ['Read by a person', 'One of our founders reviews your enquiry.'],
      ['A personal reply', 'You will hear back from ' + MAIL + '.'],
    ],
    yourH: 'Your message',
    topicL: 'Topic',
    cta: 'Explore our projects',
    ctaUrl: SITE + '/en/projects.html',
    note: 'Something urgent? Simply reply to this e-mail.',
  },
};

function block(L, d, first) {
  const t = COPY[L];
  const steps = t.steps.map((s, i) => `
        <tr>
          <td width="44" valign="top" style="padding:0 0 18px 0;">
            <table role="presentation" cellpadding="0" cellspacing="0"><tr><td width="32" height="32" align="center" valign="middle" style="background:${C.clay};border-radius:16px;color:#ffffff;font:700 15px/32px ${SANS};">${i + 1}</td></tr></table>
          </td>
          <td valign="top" style="padding:0 0 18px 0;font-family:${SANS};">
            <div style="font:700 15px/1.3 ${SANS};color:${C.ink};">${esc(s[0])}</div>
            <div style="font:400 14px/1.5 ${SANS};color:${C.soft};padding-top:2px;">${esc(s[1])}</div>
          </td>
        </tr>`).join('');
  return `
    <tr><td style="padding:${first ? '40px' : '8px'} 40px 0 40px;">
      <span style="display:inline-block;background:${C.wash};color:${C.deep};font:700 11px/1 ${SANS};letter-spacing:1.6px;text-transform:uppercase;padding:7px 12px;border-radius:999px;">${t.tag}</span>
      <h1 style="margin:18px 0 10px 0;font:500 32px/1.15 ${SERIF};color:${C.ink};letter-spacing:-0.5px;">${esc(t.hello(d.name))}</h1>
      <p style="margin:0 0 12px 0;font:400 18px/1.55 ${SANS};color:${C.ink};">${t.lead}</p>
      <p style="margin:0 0 28px 0;font:400 15px/1.6 ${SANS};color:${C.soft};">${t.sub}</p>
    </td></tr>
    <tr><td style="padding:0 40px;">
      <div style="font:700 11px/1 ${SANS};letter-spacing:1.6px;text-transform:uppercase;color:${C.clay};padding:0 0 16px 0;border-top:1px solid ${C.ink};padding-top:20px;">${t.nextH}</div>
      <table role="presentation" width="100%" cellpadding="0" cellspacing="0">${steps}</table>
    </td></tr>
    <tr><td style="padding:6px 40px 0 40px;">
      <table role="presentation" width="100%" cellpadding="0" cellspacing="0" style="background:${C.paper};border-left:4px solid ${C.blue};">
        <tr><td style="padding:18px 22px;">
          <div style="font:700 11px/1 ${SANS};letter-spacing:1.6px;text-transform:uppercase;color:${C.soft};">${t.yourH}</div>
          <div style="font:600 14px/1.4 ${SANS};color:${C.ink};padding-top:10px;">${t.topicL}: <span style="font-weight:400;">${esc(d.topic)}</span></div>
          <div style="font:italic 400 15px/1.6 ${SERIF};color:${C.ink};padding-top:8px;">${nl2br(clip(d.message, 600))}</div>
        </td></tr>
      </table>
    </td></tr>
    <tr><td style="padding:28px 40px 0 40px;">
      <table role="presentation" cellpadding="0" cellspacing="0"><tr><td style="background:${C.blue};border-radius:999px;">
        <a href="${t.ctaUrl}" style="display:inline-block;padding:14px 26px;font:700 15px/1 ${SANS};color:#ffffff;text-decoration:none;">${t.cta} &rarr;</a>
      </td></tr></table>
      <p style="margin:18px 0 0 0;font:400 13px/1.5 ${SANS};color:${C.soft};">${t.note}</p>
    </td></tr>`;
}

function confirmation(d) {
  const order = d.lang === 'fr' ? ['fr', 'en'] : ['en', 'fr'];
  const subject = order[0] === 'fr'
    ? 'Nous avons bien reçu votre demande · We received your enquiry'
    : 'We received your enquiry · Nous avons bien reçu votre demande';
  const pre = order[0] === 'fr'
    ? 'Merci ! Nous avons bien reçu votre demande et reviendrons vers vous dès que possible. / We will reply as soon as possible.'
    : 'Thank you! We have received your enquiry and will reply as soon as possible. / Nous reviendrons vers vous dès que possible.';

  const html = `<!DOCTYPE html>
<html lang="${order[0]}"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="color-scheme" content="light"><meta name="supported-color-schemes" content="light">
<title>${esc(subject)}</title>
</head>
<body style="margin:0;padding:0;background:${C.paper};">
<div style="display:none;max-height:0;overflow:hidden;opacity:0;color:${C.paper};font-size:1px;line-height:1px;">${esc(pre)}&nbsp;&zwnj;&nbsp;&zwnj;&nbsp;&zwnj;&nbsp;&zwnj;&nbsp;&zwnj;</div>
<table role="presentation" width="100%" cellpadding="0" cellspacing="0" style="background:${C.paper};">
<tr><td align="center" style="padding:28px 12px;">
  <table role="presentation" width="600" cellpadding="0" cellspacing="0" style="width:100%;max-width:600px;background:${C.white};border:1px solid ${C.rule};border-radius:10px;overflow:hidden;">
    <tr><td style="background:${C.deep};padding:26px 40px;">
      <table role="presentation" cellpadding="0" cellspacing="0"><tr>
        <td valign="middle" style="padding-right:14px;"><img src="${SITE}/assets/img/logo.png" width="52" height="52" alt="" style="display:block;border-radius:26px;background:#ffffff;border:0;"></td>
        <td valign="middle">
          <div style="font:600 24px/1 ${SERIF};color:#ffffff;letter-spacing:-0.3px;">Aether EdTech</div>
          <div style="font:600 11px/1 ${SANS};letter-spacing:1.8px;text-transform:uppercase;color:#aebfdc;padding-top:7px;">ASBL &middot; Brussels</div>
        </td>
      </tr></table>
    </td></tr>
    <tr><td style="height:5px;line-height:5px;font-size:5px;background:${C.clay};">&nbsp;</td></tr>
    ${block(order[0], d, true)}
    <tr><td style="padding:34px 40px 0 40px;"><table role="presentation" width="100%" cellpadding="0" cellspacing="0"><tr><td style="border-top:2px dashed ${C.rule};font-size:0;line-height:0;">&nbsp;</td></tr></table></td></tr>
    ${block(order[1], d, false)}
    <tr><td style="padding:40px 40px 0 40px;font-size:0;line-height:0;">&nbsp;</td></tr>
    <tr><td style="background:${C.ink};padding:26px 40px;">
      <div style="font:600 14px/1.5 ${SERIF};color:#ffffff;">Aether EdTech ASBL</div>
      <div style="font:400 12px/1.7 ${SANS};color:#9fb0cf;padding-top:6px;">
        BCE/KBO 1045.010.593 &middot; Schaerbeek, Bruxelles / Brussels, Belgique / Belgium<br>
        <a href="${SITE}" style="color:#dbe5f7;">myaether.org</a> &middot; <a href="mailto:${MAIL}" style="color:#dbe5f7;">${MAIL}</a>
      </div>
      <div style="font:400 11px/1.6 ${SANS};color:#7f90b0;padding-top:14px;border-top:1px solid #33425f;margin-top:14px;">
        FR&nbsp;: Message de confirmation automatique. Vos données ne servent qu’à vous répondre &mdash; <a href="${SITE}/fr/privacy.html" style="color:#b9c8e3;">confidentialité</a>.<br>
        EN: Automatic confirmation. Your details are used only to reply to you &mdash; <a href="${SITE}/en/privacy.html" style="color:#b9c8e3;">privacy policy</a>.
      </div>
    </td></tr>
  </table>
</td></tr>
</table>
</body></html>`;

  const text = order.map((L) => {
    const t = COPY[L];
    return `${t.hello(d.name)}\n${t.lead}\n${t.sub}\n\n${t.nextH}\n` +
      t.steps.map((s, i) => `${i + 1}. ${s[0]} - ${s[1]}`).join('\n') +
      `\n\n${t.yourH}\n${t.topicL}: ${d.topic}\n${clip(d.message, 600)}\n\n${t.cta}: ${t.ctaUrl}\n${t.note}`;
  }).join('\n\n----------------------------------------\n\n') +
    `\n\nAether EdTech ASBL - BCE/KBO 1045.010.593 - Schaerbeek, Brussels\n${SITE} - ${MAIL}`;

  return { subject, html, text };
}

function notification(d, meta) {
  const row = (k, v) => `<tr><td style="padding:8px 14px 8px 0;font:600 13px ${SANS};color:${C.soft};vertical-align:top;white-space:nowrap;">${k}</td><td style="padding:8px 0;font:400 15px/1.5 ${SANS};color:${C.ink};">${v}</td></tr>`;
  const html = `<!DOCTYPE html><html><body style="margin:0;background:${C.paper};padding:24px;">
<table role="presentation" width="100%" style="max-width:600px;margin:0 auto;background:${C.white};border:1px solid ${C.rule};border-radius:8px;"><tr><td style="padding:28px 32px;">
<div style="font:700 11px ${SANS};letter-spacing:1.6px;text-transform:uppercase;color:${C.clay};">New website enquiry</div>
<h1 style="margin:10px 0 18px;font:500 26px ${SERIF};color:${C.ink};">${esc(d.topic)}</h1>
<table role="presentation" width="100%">
${row('Name', esc(d.name))}${row('Email', `<a href="mailto:${esc(d.email)}" style="color:${C.blue};">${esc(d.email)}</a>`)}${row('Site language', esc(d.lang.toUpperCase()))}${row('Received', esc(meta.when))}
</table>
<div style="margin-top:14px;padding:16px 18px;background:${C.paper};border-left:4px solid ${C.blue};font:400 15px/1.65 ${SANS};color:${C.ink};">${nl2br(d.message)}</div>
<p style="margin:18px 0 0;font:400 12px ${SANS};color:${C.soft};">Just hit reply: your answer goes straight to ${esc(d.name)}. An automatic FR/EN confirmation has already been sent to them.</p>
</td></tr></table></body></html>`;
  const text = `New website enquiry\nTopic: ${d.topic}\nName: ${d.name}\nEmail: ${d.email}\nLanguage: ${d.lang}\nReceived: ${meta.when}\n\n${d.message}\n`;
  return { subject: `[Website] ${d.topic} — ${d.name}`.replace(/[\r\n]+/g, ' ').slice(0, 200), html, text };
}

module.exports = { confirmation, notification };
