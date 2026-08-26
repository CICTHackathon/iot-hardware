// Operator console, served from flash as a single self-contained page.
//
// GENERATED FROM console/index.html BY console/build.py - DO NOT EDIT.
// Edit the source, then re-run:  python console/build.py
//
// Both edge nodes embed this same file, so the interface a browser gets does
// not depend on which node it was pointed at. Nothing is fetched from a CDN:
// an access-control node belongs on an isolated VLAN, and a door that cannot be
// operated because a stylesheet failed to load is a door that is broken.
#pragma once

#include <pgmspace.h>

static const char CONSOLE_HTML[] PROGMEM = R"HTML(
<!doctype html>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Smart Gateway · console</title>
<style>
:root{--bg:#0e1116;--panel:#161b22;--line:#272e38;--fg:#d7dde5;--dim:#8b96a5;
      --ok:#3fb950;--warn:#d29922;--bad:#f85149;--acc:#58a6ff}
*{box-sizing:border-box}
/* A class selector outranks the user agent's [hidden] rule, and the tab
   sections carry .cols — without this the tabs all render at once. */
[hidden]{display:none!important}
body{margin:0;background:var(--bg);color:var(--fg);
     font:14px/1.45 ui-monospace,SFMono-Regular,Consolas,monospace}
header{padding:9px 16px;border-bottom:1px solid var(--line);display:flex;
       gap:14px;align-items:center;flex-wrap:wrap}
h1{font-size:15px;margin:0;font-weight:600;letter-spacing:.02em}
nav{display:flex;gap:4px}
nav button{background:none;border:1px solid transparent;color:var(--dim);
           border-radius:5px;padding:3px 11px;font:inherit;font-size:13px;cursor:pointer}
nav button:hover{color:var(--fg)}
nav button.on{background:var(--panel);border-color:var(--line);color:var(--fg)}
.reach{margin-left:auto;display:flex;gap:12px;align-items:center;font-size:12px;color:var(--dim)}
.dot{display:inline-block;width:8px;height:8px;border-radius:50%;background:#3a424e;
     margin-right:5px;vertical-align:middle}
.dot.up{background:var(--ok)}.dot.down{background:var(--bad)}
main{padding:14px}
.cols{display:grid;grid-template-columns:minmax(320px,1fr) 380px;gap:14px}
@media(max-width:960px){.cols{grid-template-columns:1fr}}
.panel{background:var(--panel);border:1px solid var(--line);border-radius:6px;margin-bottom:14px}
.panel h2{font-size:12px;text-transform:uppercase;letter-spacing:.08em;color:var(--dim);
          margin:0;padding:9px 12px;border-bottom:1px solid var(--line)}
.panel h2 small{text-transform:none;letter-spacing:0}
.body{padding:12px}
.scroll{max-height:46vh;overflow:auto}
.pills{display:flex;gap:8px;flex-wrap:wrap;padding:10px 12px;border-top:1px solid var(--line)}
.pills.plain{border-top:none}
.pill{border:1px solid var(--line);border-radius:999px;padding:2px 10px;font-size:12px;color:var(--dim)}
.pill b{color:var(--fg);font-weight:600}
.pill.ok{border-color:#2a5a33;color:var(--ok)}
.pill.bad{border-color:#5a2a2a;color:var(--bad)}
.pill.warn{border-color:#5a4a1a;color:var(--warn)}
.row{display:flex;justify-content:space-between;align-items:center;gap:10px;margin:9px 0}
.row label{color:var(--dim);font-size:12px;flex:1}
.row output{width:46px;text-align:right;color:var(--acc)}
input,select{background:#0d1117;color:var(--fg);border:1px solid var(--line);border-radius:4px;
             padding:4px 6px;font:inherit;font-size:12px;min-width:0}
input[type=range]{flex:1.4;accent-color:var(--acc);padding:0}
input[type=checkbox]{accent-color:var(--acc)}
button{background:#21262d;color:var(--fg);border:1px solid var(--line);border-radius:4px;
       padding:4px 10px;font:inherit;font-size:12px;cursor:pointer}
button:hover{border-color:var(--acc)}
button.y{border-color:#2a5a33;color:var(--ok)}
button.n{border-color:#5a2a2a;color:var(--bad)}
button:disabled{opacity:.4;cursor:default;border-color:var(--line)}
table{width:100%;border-collapse:collapse;font-size:12px}
th{text-align:left;color:var(--dim);font-weight:500;padding:4px 6px;border-bottom:1px solid var(--line)}
td{padding:4px 6px;border-bottom:1px solid #1d232c;white-space:nowrap}
fieldset{border:1px solid var(--line);border-radius:5px;margin:12px 0 0;padding:8px 10px}
legend{color:var(--dim);font-size:11px;text-transform:uppercase;letter-spacing:.07em}
#view{position:relative;line-height:0;background:#000;border-radius:5px 5px 0 0;overflow:hidden}
#stream{width:100%;display:block}
#ov{position:absolute;inset:0;width:100%;height:100%}
#mlprev{image-rendering:pixelated;width:96px;height:96px;border:1px solid var(--line);background:#000}
.wait{border:1px solid var(--warn);border-radius:6px;padding:10px;margin-bottom:10px}
.wait.idle{border-color:var(--line);color:var(--dim)}
.e-permit,.v-classified,.c1{color:var(--ok)}
.e-deny{color:var(--bad)}
.e-indeterminate,.v-motion,.c2{color:var(--warn)}
.v-rejected_s1,.v-rejected_s2{color:var(--dim)}
.c3{color:var(--acc)}
.miss{color:var(--dim);font-style:italic}
small,.hint{color:var(--dim)}
a{color:var(--acc)}
code{color:var(--acc)}
.banner{background:#2d2210;border:1px solid #5a4a1a;color:var(--warn);
        border-radius:6px;padding:9px 12px;margin-bottom:14px;font-size:12px}
</style>
<header>
  <h1>Smart Gateway</h1>
  <nav id="tabs">
    <button data-t="door" class="on">Door</button>
    <button data-t="camera">Camera</button>
    <button data-t="timeline">Timeline</button>
    <button data-t="setup">Setup</button>
  </nav>
  <span class="reach">
    <span><span class="dot" id="dGw"></span><span id="nGw">gateway</span></span>
    <span><span class="dot" id="dCam"></span><span id="nCam">camera</span></span>
  </span>
</header>
<main>
<div id="warn"></div>
<!-- ------------------------------------------------------------- door --- -->
<section id="tab-door" class="cols">
  <div>
    <div class="panel">
      <h2>Enforcement node</h2>
      <div class="body">
        <div id="wait" class="wait idle">No transaction in flight.</div>
        <div class="pills plain" id="gwPills"></div>
        <div class="row" style="margin-top:12px">
          <label>Manual actuation — recorded as an operator override</label>
          <button id="unlock" class="y">unlock</button>
          <button id="lock" class="n">lock</button>
        </div>
      </div>
    </div>
    <div class="panel">
      <h2>Audit ring <small>— newest first</small></h2>
      <div class="body scroll">
        <table><thead><tr><th>#</th><th>when</th><th>uid</th><th>state</th>
        <th>effect</th><th>reason</th><th>note</th></tr></thead>
        <tbody id="gwLog"></tbody></table>
      </div>
    </div>
  </div>
  <div>
    <div class="panel">
      <h2>Cached authorisation set</h2>
      <div class="body">
        <small>Consulted only while the decision tier is unreachable. A miss denies.</small>
        <table style="margin-top:8px"><tbody id="acl"></tbody></table>
        <div class="row"><label>UID</label><input id="aUid" size="12" placeholder="0x2334a703"></div>
        <div class="row"><label>sha256("uid:pin")</label><input id="aPin" size="12" placeholder="optional, 64 hex"></div>
        <div class="row"><label>TTL (s)</label><input id="aTtl" size="6" value="3600">
          <button id="aAdd">add</button><button id="aClr" class="n">clear</button></div>
        <div class="row"><label>Read a card into the UID field</label>
          <button id="enrol">enrol</button></div>
      </div>
    </div>
  </div>
</section>
<!-- ----------------------------------------------------------- camera --- -->
<section id="tab-camera" class="cols" hidden>
  <div>
    <div class="panel">
      <h2>Live · motion overlay</h2>
      <div id="view">
        <img id="stream" alt="stream">
        <canvas id="ov" width="32" height="24"></canvas>
      </div>
      <div class="pills" id="camPills"></div>
    </div>
    <div class="panel">
      <h2>Detections <small>— label them, then refit stage 1</small></h2>
      <div class="body scroll">
        <table><thead><tr><th>#</th><th>time</th><th>verdict</th><th>model</th>
        <th>s1</th><th>s2</th><th>box</th><th>ground truth</th></tr></thead>
        <tbody id="camLog"></tbody></table>
      </div>
      <div class="pills"><span id="camLinks" class="hint"></span></div>
    </div>
  </div>
  <div class="panel">
    <h2>Detector</h2>
    <div class="body">
      <fieldset><legend>stage 0 · frame differencing</legend>
        <div class="row"><label for="cellDelta">cell delta</label>
          <input type="range" id="cellDelta" min="4" max="80"><output></output></div>
        <div class="row"><label for="motionPercent">motion, % of cells</label>
          <input type="range" id="motionPercent" min="1" max="50"><output></output></div>
        <div class="row"><label for="sampleIdle">sample 1 in N, idle</label>
          <input type="range" id="sampleIdle" min="1" max="20"><output></output></div>
        <div class="row"><label for="sampleActive">sample 1 in N, active</label>
          <input type="range" id="sampleActive" min="1" max="30"><output></output></div>
        <div class="row"><label for="minFrames">min frames to open</label>
          <input type="range" id="minFrames" min="1" max="20"><output></output></div>
        <div class="row"><label for="clearFrames">quiet frames to close</label>
          <input type="range" id="clearFrames" min="1" max="60"><output></output></div>
      </fieldset>
      <fieldset><legend>stage 1 · geometry classifier</legend>
        <div class="row"><label for="classifierPercent">accept above %</label>
          <input type="range" id="classifierPercent" min="0" max="100"><output></output></div>
        <div class="row"><label>weights</label><small id="worigin">–</small></div>
      </fieldset>
      <fieldset><legend>stage 2 · edge impulse model</legend>
        <div class="row"><label for="mlUse">enabled</label><input type="checkbox" id="mlUse"></div>
        <div class="row"><label for="mlProbability">min probability %</label>
          <input type="range" id="mlProbability" min="0" max="100"><output></output></div>
        <div class="row"><label>model</label><small id="mlstat">–</small></div>
        <div class="row"><label>input</label><small id="mlkind">–</small></div>
        <div class="row"><label>network sees</label><img id="mlprev" alt="model input crop"></div>
        <div class="row"><button id="refreshprev">refresh crop</button></div>
      </fieldset>
      <fieldset><legend>sensor &amp; retention</legend>
        <div class="row"><label for="frameSize">frame size</label>
          <select id="frameSize">
            <option value="5">QVGA 320×240</option>
            <option value="7">HVGA 480×320</option>
            <option value="8">VGA 640×480</option>
            <option value="9">SVGA 800×600</option>
            <option value="10">XGA 1024×768</option>
            <option value="12">SXGA 1280×1024</option>
            <option value="13">UXGA 1600×1200</option>
          </select></div>
        <div class="row"><label for="jpegQuality">jpeg quality (low = better)</label>
          <input type="range" id="jpegQuality" min="10" max="50"><output></output></div>
        <div class="row"><label for="hMirror">h mirror</label><input type="checkbox" id="hMirror"></div>
        <div class="row"><label for="vFlip">v flip</label><input type="checkbox" id="vFlip"></div>
        <div class="row"><label for="saveEvents">save events to store</label>
          <input type="checkbox" id="saveEvents"></div>
        <div class="row"><label for="lampOnEvent">lamp on event</label>
          <input type="checkbox" id="lampOnEvent"></div>
        <div class="row"><label for="lampBrightness">lamp brightness</label>
          <input type="range" id="lampBrightness" min="0" max="255"><output></output></div>
      </fieldset>
    </div>
  </div>
</section>
<!-- --------------------------------------------------------- timeline --- -->
<section id="tab-timeline" hidden>
  <div class="panel">
    <h2>Movement semantics <small>— credential events joined to motion events</small></h2>
    <div class="body">
      <p class="hint" style="margin-top:0">
        A transit is only nominal when both nodes saw it. The two other classes are
        the ones worth looking at: motion with no credential is a tailgate, a propped
        door or an intrusion; a grant with no motion is credential probing, or someone
        who was let in and did not come through.
      </p>
      <div class="row" style="max-width:420px">
        <label for="win">Correlation window, ± seconds</label>
        <input type="range" id="win" min="2" max="60" value="15"><output>15</output>
      </div>
      <div class="pills plain" id="tlPills"></div>
    </div>
    <div class="body scroll" style="border-top:1px solid var(--line)">
      <table><thead><tr><th>when</th><th>class</th><th>credential</th>
      <th>motion</th><th>evidence</th></tr></thead>
      <tbody id="tl"></tbody></table>
    </div>
  </div>
</section>
<!-- ------------------------------------------------------------ setup --- -->
<section id="tab-setup" class="cols" hidden>
  <div>
    <div class="panel">
      <h2>Nodes</h2>
      <div class="body">
        <div class="row"><label for="gwUrl">Enforcement node</label>
          <input id="gwUrl" size="22" placeholder="http://192.168.1.40"></div>
        <div class="row"><label for="gwTok">API token <small>(printed once on its console)</small></label>
          <input id="gwTok" type="password" size="22"></div>
        <div class="row"><label for="camUrl">Movement recorder</label>
          <input id="camUrl" size="22" placeholder="http://192.168.1.41"></div>
        <div class="row"><label></label><button id="saveNodes">save</button>
          <button id="forget" class="n">forget</button></div>
        <small>Stored in this browser only. The recorder has no token of its own —
        it is reachable by anything on its network, which is why it belongs on an
        isolated VLAN.</small>
      </div>
    </div>
    <div class="panel">
      <h2>Enforcement node settings</h2>
      <div class="body" id="gwCfg"></div>
    </div>
  </div>
  <div>
    <div class="panel">
      <h2>Networks</h2>
      <div class="body">
        <fieldset><legend>enforcement node</legend>
          <div class="row"><label for="gwSsid">ssid</label><input id="gwSsid"></div>
          <div class="row"><label for="gwPass">passphrase</label><input id="gwPass" type="password"></div>
          <div class="row"><label></label><button id="gwWifi">store</button>
            <button id="gwReboot" class="n">reboot</button></div>
        </fieldset>
        <fieldset><legend>movement recorder</legend>
          <div class="row"><label for="camSsid">ssid</label><input id="camSsid"></div>
          <div class="row"><label for="camPass">passphrase</label><input id="camPass" type="password"></div>
          <div class="row"><label></label><button id="camWifi">store &amp; reconnect</button>
            <button id="camReboot" class="n">reboot</button></div>
        </fieldset>
      </div>
    </div>
  </div>
</section>
</main>
<script>
const $ = s => document.querySelector(s);
const esc = s => String(s == null ? '' : s).replace(/[<>&"]/g,
  c => ({'<':'&lt;','>':'&gt;','&':'&amp;','"':'&quot;'}[c]));
// ---------------------------------------------------------------- nodes ---
// Addresses live in localStorage. The one that served this page is filled in
// automatically, so a fresh node needs no configuration to be usable.
const N = {
  gw:  {url: localStorage.getItem('gwUrl')  || '', tok: localStorage.getItem('gwTok') || '', up: null},
  cam: {url: localStorage.getItem('camUrl') || '', up: null},
};
async function probeOrigin() {
  if (location.protocol === 'file:') return;
  const here = location.origin;
  if (N.gw.url === here || N.cam.url === here) return;
  try {
    const r = await fetch(here + '/api/status', {cache: 'no-store'});
    if (r.status === 401) { N.gw.url = N.gw.url || here; return; }   // gateway, token needed
    const j = await r.json();
    if ('door' in j) N.gw.url = N.gw.url || here;
    else if ('fps' in j) N.cam.url = N.cam.url || here;
  } catch (e) { /* served from somewhere that is neither node */ }
}
async function api(node, path, opt = {}) {
  const n = N[node];
  if (!n.url) throw new Error('no address for the ' + node + ' node');
  const o = {cache: 'no-store', headers: {}, ...opt};
  if (node === 'gw' && N.gw.tok) o.headers['X-Api-Token'] = N.gw.tok;
  if (o.json !== undefined) {
    o.method = 'POST';
    o.headers['Content-Type'] = 'application/json';
    o.body = JSON.stringify(o.json);
    delete o.json;
  }
  const r = await fetch(n.url + path, o);
  const t = await r.text();
  let j = {};
  try { j = JSON.parse(t); } catch (e) {}
  if (!r.ok) throw new Error(j.error || (r.status + ' ' + r.statusText));
  return j;
}
function reach(node, ok) {
  N[node].up = ok;
  $(node === 'gw' ? '#dGw' : '#dCam').className = 'dot ' + (ok ? 'up' : 'down');
}
function banner(msg) { $('#warn').innerHTML = msg ? `<div class="banner">${msg}</div>` : ''; }
// ----------------------------------------------------------------- tabs ---
for (const b of document.querySelectorAll('#tabs button')) {
  b.onclick = () => {
    for (const o of document.querySelectorAll('#tabs button')) o.classList.toggle('on', o === b);
    for (const t of ['door', 'camera', 'timeline', 'setup'])
      $('#tab-' + t).hidden = t !== b.dataset.t;
    if (b.dataset.t === 'camera') startStream();
    if (b.dataset.t === 'timeline') renderTimeline();
    if (b.dataset.t === 'setup') { loadGwConfig(); }
  };
}
const pill = (k, v, cls = '') => `<span class="pill ${cls}">${k} <b>${esc(v)}</b></span>`;
const when = e => e.epoch ? new Date(e.epoch * 1000).toLocaleTimeString() : '+' + e.uptime + 's';
// ----------------------------------------------------------- door: state ---
let seenTxn = '', gwLast = 0, gwEvents = [], gwStatus = null;
async function gwTick() {
  let s;
  try { s = await api('gw', '/api/status'); reach('gw', true); }
  catch (e) {
    reach('gw', false);
    $('#gwPills').innerHTML = pill('enforcement node', e.message, 'bad');
    return;
  }
  gwStatus = s;
  $('#nGw').textContent = s.doorId;
  $('#gwPills').innerHTML =
      pill('state', s.state, s.state === 'grant' ? 'ok' : s.state === 'deny' ? 'bad' : '') +
      pill('door', s.door, s.door === 'locked' ? '' : 'warn') +
      pill('angle', s.angle) + pill('uid', s.uid || '—') +
      pill('pin', '*'.repeat(s.pinDigits) || '—') +
      pill('clock', s.clockSet ? 'set' : 'unset', s.clockSet ? '' : 'warn') +
      pill('transactions', s.transactions) + pill('grants', s.grants, 'ok') +
      pill('denials', s.denials, 'bad') +
      pill('degraded', s.degraded, s.degraded ? 'warn' : '') +
      pill('pdp', s.pdp.configured ? s.pdp.lastCode + ' / ' + s.pdp.lastMs + 'ms' : 'not set',
           s.pdp.configured ? '' : 'warn') +
      pill('cached', s.acl) + pill('lcd', s.lcd ? 'ok' : 'absent', s.lcd ? '' : 'bad');
  const w = $('#wait');
  if (s.state === 'await_decision') {
    if (s.txn !== seenTxn) {
      seenTxn = s.txn;
      w.className = 'wait';
      w.innerHTML =
        `<b>Decision requested</b> — txn <code>${esc(s.txn)}</code>, uid <code>${esc(s.uid)}</code>,
         PIN ${s.pinDigits ? 'presented' : 'not presented'}
         <div class="row"><label>${s.signedDecisions
            ? 'This node requires a signed decision; these buttons will be refused.'
            : 'Unsigned decisions are accepted and audited as such.'}</label>
           <button class="y" id="dPermit">permit</button>
           <button class="n" id="dDeny">deny</button></div>`;
      $('#dPermit').onclick = () => decide('permit');
      $('#dDeny').onclick = () => decide('deny');
    }
  } else if (seenTxn || w.className !== 'wait idle') {
    seenTxn = '';
    w.className = 'wait idle';
    w.textContent = 'No transaction in flight.';
  }
  const ev = await api('gw', '/api/events?since=' + gwLast);
  if (ev.events && ev.events.length) {
    gwLast = ev.last;
    gwEvents = ev.events.concat(gwEvents).slice(0, 200);
    $('#gwLog').innerHTML = gwEvents.map(e => `<tr>
      <td>${e.id}</td><td>${when(e)}</td><td>${esc(e.uid) || '—'}</td>
      <td>${esc(e.state)}</td>
      <td class="e-${e.effect}">${e.effect}${e.unsigned && e.effect === 'permit' ? ' (unsigned)' : ''}</td>
      <td>${esc(e.reason)}</td><td>${esc(e.note)}</td></tr>`).join('');
  }
}
async function decide(effect) {
  try { await api('gw', '/api/decision', {json: {txn: seenTxn, effect, reason: 'operator'}}); }
  catch (e) { alert(e.message); }
  seenTxn = '';
  gwTick();
}
$('#unlock').onclick = async () => {
  if (!confirm('Open the door with no credential behind it? This is audited as an override.')) return;
  try { await api('gw', '/api/unlock', {json: {who: 'console'}}); } catch (e) { alert(e.message); }
};
$('#lock').onclick = () => api('gw', '/api/lock', {json: {who: 'console'}}).catch(e => alert(e.message));
// ------------------------------------------------------------ door: acl ---
async function loadAcl() {
  let a;
  try { a = await api('gw', '/api/acl'); } catch (e) { return; }
  $('#acl').innerHTML = a.entries.map(e =>
    `<tr><td>${esc(e.uid)}</td><td>${e.pin ? 'card+PIN' : 'card only'}</td>
     <td>${e.ttl ? e.ttl + 's' : 'no expiry'}</td>
     <td><button data-acl="${esc(e.uid)}">remove</button></td></tr>`).join('') ||
    '<tr><td colspan="4"><small>empty — a partition denies everyone</small></td></tr>';
  for (const b of $('#acl').querySelectorAll('[data-acl]'))
    b.onclick = async () => { await api('gw', '/api/acl', {json: {remove: b.dataset.acl}}); loadAcl(); };
}
$('#aAdd').onclick = async () => {
  const body = {uid: $('#aUid').value.trim(), ttl: $('#aTtl').value.trim()};
  const p = $('#aPin').value.trim();
  if (p) body.pin = p;
  try { await api('gw', '/api/acl', {json: body}); $('#aUid').value = ''; $('#aPin').value = ''; loadAcl(); }
  catch (e) { alert(e.message); }
};
$('#aClr').onclick = async () => {
  if (!confirm('Clear the cached set? The door will deny everyone during a partition.')) return;
  await api('gw', '/api/acl', {json: {clear: 1}});
  loadAcl();
};
$('#enrol').onclick = async () => {
  await api('gw', '/api/enrol?ms=20000', {json: {}});
  const b = $('#enrol');
  b.disabled = true; b.textContent = 'present a card…';
  const deadline = Date.now() + 21000;
  const poll = setInterval(async () => {
    const r = await api('gw', '/api/enrol');
    if (r.uid) $('#aUid').value = r.uid;
    if (r.uid || !r.armed || Date.now() > deadline) {
      clearInterval(poll);
      b.disabled = false; b.textContent = 'enrol';
    }
  }, 600);
};
// --------------------------------------------------------------- camera ---
const CAM_KEYS = ['cellDelta','motionPercent','sampleIdle','sampleActive','minFrames',
                  'clearFrames','classifierPercent','mlProbability','jpegQuality',
                  'lampBrightness','frameSize','mlUse','hMirror','vFlip','lampOnEvent','saveEvents'];
const ctx = $('#ov').getContext('2d');
let GW = 32, GH = 24, camEvents = [], streamOn = false, camStatus = null;
function camPost(k, v) {
  fetch(N.cam.url + '/api/config?' + k + '=' + encodeURIComponent(v), {method: 'POST'}).catch(() => {});
}
function bindCam() {
  for (const k of CAM_KEYS) {
    const el = $('#' + k);
    if (!el) continue;
    const out = el.parentElement.querySelector('output');
    el.addEventListener('input', () => { if (out) out.value = el.value; });
    el.addEventListener('change', () => camPost(k, el.type === 'checkbox' ? (el.checked ? 1 : 0) : el.value));
  }
}
function startStream() {
  if (streamOn || !N.cam.url) return;
  // The stream is a second server on port 81 so a stalled viewer cannot take
  // the management interface with it. An <img> is not subject to CORS, so this
  // works from any origin without the node knowing about it.
  const u = new URL(N.cam.url);
  u.port = 81; u.pathname = '/stream';
  $('#stream').src = u.toString();
  streamOn = true;
}
async function loadCamConfig() {
  let c;
  try { c = await api('cam', '/api/config'); } catch (e) { return; }
  for (const k of CAM_KEYS) {
    const el = $('#' + k);
    if (!el || !(k in c)) continue;
    if (el.type === 'checkbox') el.checked = !!c[k]; else el.value = c[k];
    const out = el.parentElement.querySelector('output');
    if (out) out.value = el.value;
  }
  $('#worigin').textContent = c.classifierOrigin;
  $('#mlstat').textContent = c.mlStatus;
  $('#mlkind').textContent = c.mlInput + ' · ' + c.mlKind;
}
function drawOverlay(st) {
  const m = st.mask || '';
  ctx.clearRect(0, 0, GW, GH);
  ctx.fillStyle = 'rgba(248,81,73,.45)';
  for (let i = 0; i < m.length && i < GW * GH; i++)
    if (m[i] === '1') ctx.fillRect(i % GW, (i / GW) | 0, 1, 1);
  if (st.changed > 0) {
    ctx.strokeStyle = '#58a6ff'; ctx.lineWidth = .25;
    ctx.strokeRect(st.x0 + .1, st.y0 + .1, st.x1 - st.x0 + .8, st.y1 - st.y0 + .8);
  }
}
const kb = n => n > 1048576 ? ((n / 1048576) | 0) + ' MB' : ((n / 1024) | 0) + ' KB';
async function camTick() {
  let st;
  try { st = await api('cam', '/api/status'); reach('cam', true); }
  catch (e) {
    reach('cam', false);
    $('#camPills').innerHTML = pill('movement recorder', e.message, 'bad');
    return;
  }
  camStatus = st;
  GW = st.gw; GH = st.gh;
  const cv = $('#ov');
  if (cv.width != GW) { cv.width = GW; cv.height = GH; }
  $('#nCam').textContent = 'camera ' + st.ip;
  $('#camPills').innerHTML =
      pill('state', st.eventOpen ? 'event open · ' + st.persistence : 'idle', st.eventOpen ? 'bad' : '') +
      pill('fps', st.fps.toFixed(1)) + pill('frames', st.frames) + pill('sampled', st.samples) +
      pill('area', (st.area * 100).toFixed(1) + '%') +
      pill('stage1', st.s1 < 0 ? '–' : st.s1.toFixed(2)) +
      pill('stage2', st.s2 < 0 ? '–' : st.s2.toFixed(2)) +
      pill('decode', st.decodeMs + 'ms') + pill('infer', st.mlMs + 'ms') +
      pill('psram', st.psram + 'K') +
      pill('clock', st.clockSet ? 'set' : 'unset', st.clockSet ? '' : 'warn');
  $('#camLinks').innerHTML =
      `<a href="${N.cam.url}/api/dataset.csv" download>dataset.csv</a> ·
       <a href="${N.cam.url}/dav/" target="_blank">recordings</a> ·
       <a href="${N.cam.url}/capture" target="_blank">still</a>
       — ${st.store}, ${kb(st.storeUsed)} of ${kb(st.storeTotal)} used`;
  drawOverlay(st);
}
async function loadCamEvents() {
  try { camEvents = await api('cam', '/api/events'); } catch (e) { return; }
  $('#camLog').innerHTML = camEvents.map(e => `<tr>
    <td>${e.path ? `<a href="${N.cam.url}/dav${esc(e.path)}" target="_blank">${e.id}</a>` : e.id}</td>
    <td>${esc(e.time)}</td><td class="v-${e.verdict}">${esc(e.verdict)}</td>
    <td>${esc(e.mlLabel) || '–'}</td>
    <td>${e.s1.toFixed(2)}</td><td>${e.s2 < 0 ? '–' : e.s2.toFixed(2)}</td>
    <td>${e.x0},${e.y0}–${e.x1},${e.y1}</td>
    <td>${e.label < 0 ? `<button class="y" data-i="${e.id}" data-l="1">yes</button>
                        <button class="n" data-i="${e.id}" data-l="0">no</button>`
                      : (e.label ? 'yes' : 'no')}</td></tr>`).join('');
}
document.addEventListener('click', async ev => {
  const b = ev.target.closest('button[data-i]');
  if (!b) return;
  await fetch(`${N.cam.url}/api/label?id=${b.dataset.i}&label=${b.dataset.l}`, {method: 'POST'});
  loadCamEvents();
});
$('#refreshprev').onclick = () => { $('#mlprev').src = N.cam.url + '/api/mlpreview.bmp?t=' + Date.now(); };
// ------------------------------------------------------------- timeline ---
// The two nodes share no wire, so the epoch is the only thing that can join a
// credential event to a motion event. Both nodes report whether their clock is
// actually set; when either is not, the table says so instead of lining up two
// unrelated stopwatches and calling it evidence.
//
// The check is on node status rather than on the events, so an unsynchronised
// bench is named before anything has been recorded, rather than looking like a
// door nobody has walked through yet.
$('#win').addEventListener('input', e => {
  e.target.parentElement.querySelector('output').value = e.target.value;
  renderTimeline();
});
function renderTimeline() {
  const win = +$('#win').value;
  // A node that has not answered yet is not a node with a bad clock: an
  // unreachable half greys out, it does not accuse the other one of drifting.
  const noClock = [];
  if (gwStatus && !gwStatus.clockSet) noClock.push('the enforcement node');
  if (camStatus && !camStatus.clockSet) noClock.push('the recorder');
  if (noClock.length) {
    $('#tlPills').innerHTML = pill('correlation', 'unavailable — a clock is unset', 'warn');
    $('#tl').innerHTML =
      `<tr><td colspan="5" class="miss">No time on ${esc(noClock.join(' and '))}.
       Both nodes need NTP, or a joined timeline is two stopwatches started at
       different moments.</td></tr>`;
    return;
  }
  // Records written before NTP landed carry no epoch. They are dropped from the
  // join and counted, rather than blanking a table that is now correlating
  // perfectly well — they age out of both rings on their own.
  const credAll = gwEvents.filter(e => e.effect === 'permit' || e.effect === 'deny');
  const cred = credAll.filter(e => e.epoch).map(e => ({t: e.epoch, e}));
  const motion = camEvents.filter(e => e.epoch).map(e => ({t: e.epoch, e}));
  const unstamped = (credAll.length - cred.length) + (camEvents.length - motion.length);
  const usedMotion = new Set();
  const rows = [];
  let c1 = 0, c2 = 0, c3 = 0;
  for (const c of cred) {
    const m = motion.find(x => !usedMotion.has(x.e.id) && Math.abs(x.t - c.t) <= win);
    if (m) usedMotion.add(m.e.id);
    const permit = c.e.effect === 'permit';
    let cls, label;
    if (m && permit)  { cls = 'c1'; label = '1 · attested transit'; c1++; }
    else if (permit)  { cls = 'c3'; label = '3 · unconsummated grant'; c3++; }
    else              { cls = '';   label = '— denial'; }
    rows.push({t: c.t, cls, label, cred: c.e, motion: m && m.e});
  }
  for (const m of motion) {
    if (usedMotion.has(m.e.id)) continue;
    rows.push({t: m.t, cls: 'c2', label: '2 · unattested motion', cred: null, motion: m.e});
    c2++;
  }
  rows.sort((a, b) => b.t - a.t);
  $('#tlPills').innerHTML =
      pill('attested transits', c1, 'ok') + pill('unattested motion', c2, c2 ? 'warn' : '') +
      pill('unconsummated grants', c3, c3 ? 'warn' : '') + pill('window', '±' + win + 's') +
      (unstamped ? pill('not joinable', unstamped + ' before NTP', 'warn') : '');
  $('#tl').innerHTML = rows.map(r => `<tr>
    <td>${new Date(r.t * 1000).toLocaleTimeString()}</td>
    <td class="${r.cls}">${r.label}</td>
    <td>${r.cred ? `<span class="e-${r.cred.effect}">${r.cred.effect}</span> ${esc(r.cred.uid)}`
                 : '<span class="miss">none</span>'}</td>
    <td>${r.motion ? esc(r.motion.verdict) + (r.motion.mlLabel ? ' · ' + esc(r.motion.mlLabel) : '')
                   : '<span class="miss">none</span>'}</td>
    <td>${r.motion && r.motion.path
            ? `<a href="${N.cam.url}/dav${esc(r.motion.path)}" target="_blank">clip</a>` : '—'}</td>
    </tr>`).join('') || '<tr><td colspan="5" class="miss">nothing recorded yet</td></tr>';
}
// ---------------------------------------------------------------- setup ---
const GW_FIELDS = [
  ['doorId', 'Door identifier', 'text'],
  ['openAngle', 'Servo angle, latch clear', 'number'],
  ['closedAngle', 'Servo angle, latch shot', 'number'],
  ['openHoldMs', 'Hold open (ms)', 'number'],
  ['pinTimeoutMs', 'T_pin (ms)', 'number'],
  ['pdpTimeoutMs', 'T_pdp (ms)', 'number'],
  ['pinAttempts', 'Attempts before lockout', 'number'],
  ['requirePin', 'Require a PIN', 'bool'],
  ['degradedAllow', 'Use the cached set when offline', 'bool'],
  ['buzzerEnabled', 'Buzzer', 'bool'],
  ['pdpUrl', 'PDP endpoint', 'text'],
  ['pdpToken', 'PDP bearer token (write-only)', 'text'],
  ['decisionKey', 'Decision HMAC key (write-only)', 'text'],
  ['apiToken', 'API token (write-only)', 'text'],
];
async function loadGwConfig() {
  let c;
  try { c = await api('gw', '/api/config'); }
  catch (e) { $('#gwCfg').innerHTML = `<small>${esc(e.message)}</small>`; return; }
  $('#gwCfg').innerHTML = GW_FIELDS.map(([k, label, type]) => {
    const v = c[k];
    const input = type === 'bool'
      ? `<select data-k="${k}"><option value="1"${v ? ' selected' : ''}>on</option>
         <option value="0"${v ? '' : ' selected'}>off</option></select>`
      : `<input data-k="${k}" type="${type}" size="10" value="${v === 'set' ? '' : esc(v)}"
         placeholder="${v === 'set' ? 'set — type to replace' : ''}">`;
    return `<div class="row"><label>${label}</label>${input}</div>`;
  }).join('') + '<div class="row"><label></label><button id="cSave">apply</button></div>';
  // One request, one transaction. Applying a field at a time meant swapping the
  // two servo angles had to pass through a pair the node is right to refuse,
  // and a rejection halfway down the form left the rest of it unapplied.
  $('#cSave').onclick = async () => {
    const body = {};
    for (const el of $('#gwCfg').querySelectorAll('[data-k]')) {
      if (el.tagName === 'INPUT' && el.value === '') continue;    // untouched secret
      body[el.dataset.k] = el.value;
    }
    try { await api('gw', '/api/config', {json: body}); }
    catch (e) { alert('settings not applied — ' + e.message); }
    loadGwConfig();
  };
}
$('#saveNodes').onclick = () => {
  N.gw.url = $('#gwUrl').value.trim().replace(/\/$/, '');
  N.cam.url = $('#camUrl').value.trim().replace(/\/$/, '');
  N.gw.tok = $('#gwTok').value.trim();
  localStorage.setItem('gwUrl', N.gw.url);
  localStorage.setItem('camUrl', N.cam.url);
  localStorage.setItem('gwTok', N.gw.tok);
  streamOn = false;
  boot();
};
$('#forget').onclick = () => { localStorage.clear(); location.reload(); };
$('#gwWifi').onclick = () => api('gw', '/api/wifi',
    {json: {ssid: $('#gwSsid').value, pass: $('#gwPass').value}})
  .then(() => alert('stored — reboot the node to join')).catch(e => alert(e.message));
$('#gwReboot').onclick = () => api('gw', '/api/reboot', {json: {}})
  .catch(e => alert(e.message));
$('#camWifi').onclick = () => fetch(`${N.cam.url}/api/wifi?ssid=` +
    `${encodeURIComponent($('#camSsid').value)}&pass=${encodeURIComponent($('#camPass').value)}`,
    {method: 'POST'}).then(() => alert('stored — the recorder is restarting'));
$('#camReboot').onclick = () => fetch(N.cam.url + '/api/reboot', {method: 'POST'});
// ----------------------------------------------------------------- boot ---
async function boot() {
  await probeOrigin();
  $('#gwUrl').value = N.gw.url;
  $('#camUrl').value = N.cam.url;
  $('#gwTok').value = N.gw.tok;
  const missing = [];
  if (!N.gw.url) missing.push('enforcement node');
  if (!N.cam.url) missing.push('movement recorder');
  banner(missing.length
    ? `No address for the ${missing.join(' or the ')}. Set it under
       <b>Setup → Nodes</b>; this page works with either one alone.`
    : '');
  if (N.gw.url) { gwTick(); loadAcl(); }
  if (N.cam.url) { loadCamConfig(); camTick(); loadCamEvents(); }
}
bindCam();
boot();
setInterval(() => { if (N.gw.url) gwTick(); }, 700);
setInterval(() => { if (N.cam.url) camTick(); }, 700);
setInterval(() => { if (N.cam.url) loadCamEvents(); }, 3000);
setInterval(() => { if (!$('#tab-timeline').hidden) renderTimeline(); }, 3000);
</script>
)HTML";
