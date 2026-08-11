// Operator interface, served from flash as a single self-contained page.
//
// No external assets: the node must remain usable on an isolated VLAN with no
// route to a CDN, which is where an access-control edge device belongs.
#pragma once

#include <pgmspace.h>

static const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32-CAM · movement recorder</title>
<style>
:root{--bg:#0e1116;--panel:#161b22;--line:#272e38;--fg:#d7dde5;--dim:#8b96a5;
      --ok:#3fb950;--warn:#d29922;--bad:#f85149;--acc:#58a6ff}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--fg);
     font:14px/1.45 ui-monospace,SFMono-Regular,Consolas,monospace}
header{padding:10px 16px;border-bottom:1px solid var(--line);display:flex;
       gap:16px;align-items:baseline;flex-wrap:wrap}
h1{font-size:15px;margin:0;font-weight:600;letter-spacing:.02em}
.sub{color:var(--dim);font-size:12px}
main{display:grid;grid-template-columns:minmax(320px,1fr) 340px;gap:14px;padding:14px}
@media(max-width:860px){main{grid-template-columns:1fr}}
.panel{background:var(--panel);border:1px solid var(--line);border-radius:6px}
.panel h2{font-size:12px;text-transform:uppercase;letter-spacing:.08em;
          color:var(--dim);margin:0;padding:9px 12px;border-bottom:1px solid var(--line)}
.body{padding:12px}
#view{position:relative;line-height:0;background:#000}
#stream{width:100%;display:block}
#ov{position:absolute;inset:0;width:100%;height:100%}
.pills{display:flex;gap:8px;flex-wrap:wrap;padding:10px 12px;border-top:1px solid var(--line)}
.pill{border:1px solid var(--line);border-radius:999px;padding:2px 10px;font-size:12px;color:var(--dim)}
.pill b{color:var(--fg);font-weight:600}
.pill.hot{border-color:var(--bad);color:var(--bad)}
.row{display:flex;justify-content:space-between;align-items:center;gap:10px;margin:9px 0}
.row label{color:var(--dim);font-size:12px;flex:1}
.row output{width:44px;text-align:right;color:var(--acc)}
input[type=range]{flex:1.4;accent-color:var(--acc)}
select,input[type=text],input[type=password]{background:#0d1117;color:var(--fg);
  border:1px solid var(--line);border-radius:4px;padding:4px 6px;font:inherit;font-size:12px}
button{background:#21262d;color:var(--fg);border:1px solid var(--line);border-radius:4px;
       padding:4px 10px;font:inherit;font-size:12px;cursor:pointer}
button:hover{border-color:var(--acc)}
button.y{border-color:#2a5a33}button.n{border-color:#5a2a2a}
table{width:100%;border-collapse:collapse;font-size:12px}
th{text-align:left;color:var(--dim);font-weight:500;padding:4px 6px;border-bottom:1px solid var(--line)}
td{padding:4px 6px;border-bottom:1px solid #1d232c;white-space:nowrap}
.v-classified{color:var(--ok)}
.v-motion{color:var(--warn)}
.v-rejected-geometry,.v-rejected-model{color:var(--dim)}
fieldset{border:1px solid var(--line);border-radius:5px;margin:12px 0 0;padding:8px 10px}
legend{color:var(--dim);font-size:11px;text-transform:uppercase;letter-spacing:.07em}
a{color:var(--acc)}
small{color:var(--dim)}
#mlprev{image-rendering:pixelated;width:96px;height:96px;border:1px solid var(--line);background:#000}
</style>

<header>
  <h1>ESP32-CAM · movement recorder</h1>
  <span class="sub" id="hdr">connecting…</span>
</header>

<main>
  <section>
    <div class="panel">
      <h2>Live · motion overlay</h2>
      <div id="view">
        <img id="stream" alt="stream">
        <canvas id="ov" width="32" height="24"></canvas>
      </div>
      <div class="pills">
        <span class="pill">fps <b id="fps">–</b></span>
        <span class="pill">frames <b id="frames">–</b></span>
        <span class="pill">sampled <b id="samples">–</b></span>
        <span class="pill">area <b id="area">–</b></span>
        <span class="pill" id="state">idle</span>
        <span class="pill">stage1 <b id="s1">–</b></span>
        <span class="pill">stage2 <b id="s2">–</b></span>
        <span class="pill">decode <b id="dec">–</b>ms</span>
        <span class="pill">infer <b id="mlms">–</b>ms</span>
        <span class="pill">heap <b id="heap">–</b></span>
      </div>
    </div>

    <div class="panel" style="margin-top:14px">
      <h2>Events <small>— label them, then refit stage 1</small></h2>
      <div class="body" style="overflow-x:auto">
        <table>
          <thead><tr><th>#</th><th>time</th><th>verdict</th><th>label·model</th>
                     <th>s1</th><th>s2</th><th>box</th><th>ground truth</th></tr></thead>
          <tbody id="events"></tbody>
        </table>
        <p><a href="/api/dataset.csv" download>dataset.csv</a> ·
           <a href="/dav/" target="_blank">browse recordings</a> ·
           <a href="/capture" target="_blank">still</a>
           <small id="store"></small></p>
      </div>
    </div>
  </section>

  <section class="panel">
    <h2>Configuration</h2>
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
        <div class="row"><label for="mlUse">enabled</label>
          <input type="checkbox" id="mlUse"></div>
        <div class="row"><label for="mlProbability">min probability %</label>
          <input type="range" id="mlProbability" min="0" max="100"><output></output></div>
        <div class="row"><label>model</label><small id="mlstat">–</small></div>
        <div class="row"><label>input</label><small id="mlkind">–</small></div>
        <div class="row"><label>network sees</label>
          <img id="mlprev" alt="model input crop"></div>
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

      <fieldset><legend>network</legend>
        <div class="row"><label for="ssid">ssid</label><input type="text" id="ssid"></div>
        <div class="row"><label for="pass">passphrase</label><input type="password" id="pass"></div>
        <div class="row"><button id="wifi">save &amp; reconnect</button>
          <button id="reboot">reboot</button></div>
      </fieldset>
    </div>
  </section>
</main>

<script>
const $=s=>document.querySelector(s);
const KEYS=["cellDelta","motionPercent","sampleIdle","sampleActive","minFrames",
            "clearFrames","classifierPercent","mlProbability","jpegQuality",
            "lampBrightness","frameSize","mlUse","hMirror","vFlip","lampOnEvent",
            "saveEvents"];
const ctx=$("#ov").getContext("2d");
let GW=32,GH=24;

$("#stream").src="http://"+location.hostname+":81/stream";

function post(k,v){
  fetch("/api/config?"+k+"="+encodeURIComponent(v),{method:"POST"}).catch(()=>{});
}
function bind(){
  for(const k of KEYS){
    const el=$("#"+k); if(!el) continue;
    const out=el.parentElement.querySelector("output");
    el.addEventListener("input",()=>{if(out) out.value=el.value;});
    el.addEventListener("change",()=>{
      post(k, el.type==="checkbox"?(el.checked?1:0):el.value);
    });
  }
}
async function loadConfig(){
  const c=await (await fetch("/api/config")).json();
  for(const k of KEYS){
    const el=$("#"+k); if(!el||!(k in c)) continue;
    if(el.type==="checkbox") el.checked=!!c[k]; else el.value=c[k];
    const out=el.parentElement.querySelector("output");
    if(out) out.value=el.value;
  }
  $("#worigin").textContent=c.classifierOrigin;
  $("#mlstat").textContent=c.mlStatus;
  $("#mlkind").textContent=c.mlInput+" · "+c.mlKind;
}
function drawOverlay(st){
  const m=st.mask||"";
  ctx.clearRect(0,0,GW,GH);
  ctx.fillStyle="rgba(248,81,73,.45)";
  for(let i=0;i<m.length&&i<GW*GH;i++)
    if(m[i]==="1") ctx.fillRect(i%GW,(i/GW)|0,1,1);
  if(st.changed>0){
    ctx.strokeStyle="#58a6ff";ctx.lineWidth=.25;
    ctx.strokeRect(st.x0+.1,st.y0+.1,st.x1-st.x0+.8,st.y1-st.y0+.8);
  }
}
const kb=n=>n>1048576?((n/1048576)|0)+" MB":((n/1024)|0)+" KB";
async function tick(){
  try{
    const st=await (await fetch("/api/status")).json();
    GW=st.gw;GH=st.gh;
    const cv=$("#ov"); if(cv.width!=GW){cv.width=GW;cv.height=GH;}
    $("#hdr").textContent=st.ip+" · up "+st.uptime+"s · "+st.psram+" KB psram free";
    $("#fps").textContent=st.fps.toFixed(1);
    $("#frames").textContent=st.frames;
    $("#samples").textContent=st.samples;
    $("#area").textContent=(st.area*100).toFixed(1)+"%";
    $("#s1").textContent=st.s1<0?"–":st.s1.toFixed(2);
    $("#s2").textContent=st.s2<0?"–":st.s2.toFixed(2);
    $("#dec").textContent=st.decodeMs;
    $("#mlms").textContent=st.mlMs;
    $("#heap").textContent=(st.heap/1024|0)+"K";
    $("#store").textContent=" — "+st.store+", "+kb(st.storeUsed)+" of "+kb(st.storeTotal)+" used";
    const s=$("#state");
    s.textContent=st.eventOpen?("event open ·"+st.persistence):"idle";
    s.className="pill"+(st.eventOpen?" hot":"");
    drawOverlay(st);
  }catch(e){$("#hdr").textContent="link lost";}
}
async function loadEvents(){
  const evs=await (await fetch("/api/events")).json();
  $("#events").innerHTML=evs.map(e=>`<tr>
    <td>${e.path?`<a href="/dav${e.path}" target="_blank">${e.id}</a>`:e.id}</td>
    <td>${e.time}</td>
    <td class="v-${e.verdict}">${e.verdict}</td>
    <td>${e.mlLabel||"–"}</td>
    <td>${e.s1.toFixed(2)}</td><td>${e.s2<0?"–":e.s2.toFixed(2)}</td>
    <td>${e.x0},${e.y0}–${e.x1},${e.y1}</td>
    <td>${e.label<0?`<button class="y" data-i="${e.id}" data-l="1">yes</button>
                    <button class="n" data-i="${e.id}" data-l="0">no</button>`
                  :(e.label?"yes":"no")}</td></tr>`).join("");
}
document.addEventListener("click",async ev=>{
  const b=ev.target.closest("button[data-i]"); if(!b) return;
  await fetch(`/api/label?id=${b.dataset.i}&label=${b.dataset.l}`,{method:"POST"});
  loadEvents();
});
$("#refreshprev").onclick=()=>{$("#mlprev").src="/api/mlpreview.bmp?t="+Date.now();};
$("#wifi").onclick=async()=>{
  await fetch(`/api/wifi?ssid=${encodeURIComponent($("#ssid").value)}&pass=${encodeURIComponent($("#pass").value)}`,{method:"POST"});
  $("#hdr").textContent="credentials stored — rebooting";
};
$("#reboot").onclick=()=>fetch("/api/reboot",{method:"POST"});

bind();loadConfig();loadEvents();tick();
setInterval(tick,500);setInterval(loadEvents,3000);
</script>
)HTML";
