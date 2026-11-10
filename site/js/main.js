(function () {
  var btn = document.querySelector('.menu-btn');
  var nav = document.getElementById('nav');
  if (!btn || !nav) return;
  btn.addEventListener('click', function () {
    var open = nav.classList.toggle('open');
    btn.setAttribute('aria-expanded', open ? 'true' : 'false');
  });
  document.addEventListener('keydown', function (e) {
    if (e.key === 'Escape' && nav.classList.contains('open')) {
      nav.classList.remove('open');
      btn.setAttribute('aria-expanded', 'false');
      btn.focus();
    }
  });
})();

/* Contact form: posts to our EU-hosted Azure Function, falls back to mailto without JS */
(function () {
  var form = document.getElementById('contact-form');
  if (!form) return;
  var API = 'https://aether-contact-api.azurewebsites.net/api/contact';
  var started = Date.now();
  var btn = form.querySelector('button[type=submit]');
  var box = form.querySelector('.form-msg');
  function show(html, ok) {
    box.hidden = false; box.className = 'form-msg ' + (ok ? 'ok' : 'err'); box.innerHTML = html;
    box.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
  }
  form.addEventListener('submit', function (e) {
    e.preventDefault();
    if (!form.checkValidity()) { form.reportValidity(); return; }
    btn.disabled = true; btn.textContent = form.dataset.sending;
    var body = {
      name: form.name.value, email: form.email.value, topic: form.topic.value,
      message: form.message.value, website: form.website.value,
      lang: form.dataset.lang, t: started
    };
    fetch(API, { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body), referrerPolicy: 'no-referrer' })
      .then(function (r) { return r.json().then(function (j) { return { ok: r.ok && j.ok }; }); })
      .then(function (r) {
        if (r.ok) { show(document.getElementById('msg-ok').innerHTML, true); form.reset(); started = Date.now(); btn.style.display = 'none'; }
        else { throw new Error('send'); }
      })
      .catch(function () { show(document.getElementById('msg-err').innerHTML, false); })
      .then(function () { btn.disabled = false; btn.textContent = form.dataset.btn; });
  });
})();
