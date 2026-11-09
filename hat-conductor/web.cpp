#include "web.h"
#include "config.h"
#include "settings.h"
#include "radio.h"
#include "mode.h"
#include <WebServer.h>

static WebServer server(80);
static AudioFrame lastFrame = {};

static const char PAGE[] = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Conductor Hat</title>
<style>
:root{color-scheme:dark}
*{box-sizing:border-box}
body{margin:0 auto;padding:0 16px 16px;max-width:480px;font-family:system-ui,sans-serif;background:#111;color:#eee}
header{position:sticky;top:0;z-index:2;display:flex;align-items:center;gap:10px;padding:12px 0;background:#111}
header h1{flex:1;font-size:1.3rem;margin:0}
#ver{font-size:.8rem;color:#888;font-variant-numeric:tabular-nums}
#menuBtn{flex:none;width:40px;height:40px;padding:0;border:0;border-radius:8px;background:#222;color:#eee;font-size:1.4rem;line-height:40px}
nav{position:absolute;top:56px;left:0;min-width:180px;background:#222;border-radius:10px;box-shadow:0 8px 24px #000a;overflow:hidden}
nav[hidden]{display:none}
nav a{display:block;padding:14px 16px;color:#eee;text-decoration:none;font-size:1rem}
nav a.on{background:#333;color:#fc6}
section[hidden]{display:none}
h2{font-size:1rem;margin:24px 0 0;color:#ccc}
.cards{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;transition:opacity .3s}
.off{opacity:.35}
.card{background:#1d1d1d;border-radius:10px;padding:12px 6px;text-align:center}
.card b{display:block;font-size:1.6rem;font-variant-numeric:tabular-nums}
.card span{font-size:.75rem;color:#aaa}
.tap{cursor:pointer;user-select:none;-webkit-tap-highlight-color:transparent}
.tap:active{background:#2a2a2a}
.card span.tapHint::after{content:' \21C4';color:#fc6}.meter{height:10px;background:#1d1d1d;border-radius:5px;margin:12px 0 0;overflow:hidden}
.meter div{height:100%;width:0;background:linear-gradient(90deg,#d22,#f90,#2c2,#fff);transition:width .2s}
label{display:flex;justify-content:space-between;margin:18px 0 6px;font-size:.95rem}
label output{color:#fc6;font-variant-numeric:tabular-nums}
input[type=range]{width:100%;height:28px;margin:0}
.ends{display:flex;justify-content:space-between;font-size:.75rem;color:#999;font-variant-numeric:tabular-nums}
.saved{color:#888;font-size:.8rem;margin-left:6px}
.hint{font-size:.75rem;color:#777;margin-top:4px}
.btns{display:flex;gap:8px;margin-top:24px}
.btns button{flex:1;padding:14px;border:0;border-radius:8px;font-size:1rem;background:#c33;color:#fff}
.btns button.alt{background:#333}
#msg{margin-top:12px;font-size:.85rem;color:#aaa;min-height:1.2em}
#hatSummary{font-size:.9rem;color:#aaa;margin:4px 0 12px}
table{width:100%;border-collapse:collapse;font-variant-numeric:tabular-nums;transition:opacity .3s}
th{text-align:left;font-size:.75rem;font-weight:normal;color:#888;padding:0 6px 6px}
td{padding:10px 6px;border-top:1px solid #222;font-size:.95rem}
td.id{font-family:ui-monospace,Consolas,monospace}
.dot{display:inline-block;width:8px;height:8px;border-radius:50%;margin-right:6px;background:#555}
.dot.live{background:#2c2}
tr.gone td{color:#777}
.good{color:#2c2}.fair{color:#fc6}.weak{color:#f55}
.mismatch{color:#f90}
.empty{color:#777;text-align:center;padding:24px 0}
.chan{align-items:center;margin-top:8px}
select{padding:8px 10px;border:0;border-radius:8px;background:#222;color:#fc6;font-size:1rem}
#chanMsg{font-size:.85rem;color:#fc6;min-height:1.2em;margin:6px 0 16px}
.mode{display:block;width:100%;text-align:left;padding:14px 16px;margin:10px 0 0;border:2px solid transparent;border-radius:10px;background:#1d1d1d;color:#eee;font-size:1.05rem}
.mode span{display:block;font-size:.8rem;color:#999;margin-top:2px}
.mode.on{border-color:#fc6;background:#262014}
.mode.on b::after{content:'  \25CF';color:#fc6}
#modeMsg{margin-top:12px;font-size:.85rem;color:#aaa;min-height:1.2em}
</style>
</head>
<body>
<header>
<button id="menuBtn" aria-label="Menu" aria-expanded="false" aria-controls="menu">&#9776;</button>
<h1 id="title">Tuning</h1>
<span id="ver"></span>
<nav id="menu" hidden>
<a href="#tuning" data-view="tuning">Tuning</a>
<a href="#hats" data-view="hats">Hats</a>
<a href="#modes" data-view="modes">Modes</a>
</nav>
</header>

<section id="v-tuning">
<div class="cards" id="cards">
<div class="card"><b id="rx">&ndash;</b><span>receivers</span></div>
<div class="card"><b id="db">&ndash;</b><span>dBFS</span></div>
<div class="card tap" id="hzCard" role="button" tabindex="0" aria-label="Toggle between frequency and note"><b id="hz">&ndash;</b><span id="hzUnit">Hz</span></div>
</div>
<div class="meter"><div id="lvl"></div></div>
<h2>Settings</h2>
<form id="f">
<label for="gate">Noise floor <output id="gate_o"></output></label>
<input type="range" name="gate" id="gate">
<div class="hint">Sounds quieter than this show nothing.</div>
<label for="minFreq">Minimum frequency <output id="minFreq_o"></output></label>
<input type="range" name="minFreq" id="minFreq">
<div class="hint">Everything below this is filtered out and ignored for color.</div>
<label for="release">Bar decay <output id="release_o"></output></label>
<input type="range" name="release" id="release">
<div class="hint">How fast the bar falls. A full bar is 255.</div>
<label for="agcDecay">Gain recovery decay <output id="agcDecay_o"></output></label>
<input type="range" name="agcDecay" id="agcDecay">
<div class="hint">How fast the meter regains sensitivity after loud passages.</div>
<div class="btns"><button type="submit">Save</button><button type="button" class="alt" id="reset">Reset to defaults</button></div>
<div id="msg"></div>
</form>
</section>

<section id="v-hats" hidden>
<label class="chan" for="chan">Broadcast channel <select id="chan"></select></label>
<div class="hint">Moves the conductor and every receiver. Channels 1, 6 and 11 don't overlap each other. Your phone drops off for a few seconds and must rejoin.</div>
<div id="chanMsg"></div>
<div id="hatSummary">&nbsp;</div>
<table id="hatTable">
<thead><tr><th>ID</th><th>Signal</th><th>Version</th><th>Last seen</th></tr></thead>
<tbody id="hatRows"></tbody>
</table>
</section>

<section id="v-modes" hidden>
<div class="hint">Applies to every receiver at once. Not saved: every hat starts in Reactive after a reboot.</div>
<button class="mode" data-mode="0"><b>Reactive</b><span>Receivers mirror the conductor's level and pitch color.</span></button>
<button class="mode" data-mode="1"><b>Sleep</b><span>All LEDs off. Hats ignore the music until another mode is chosen.</span></button>
<button class="mode" data-mode="2"><b>Disco</b><span>Bars still follow the music, but each hat jumps to a random color every second.</span></button>
<button class="mode" data-mode="3"><b>Ranged</b><span>Only the first LED lights, showing the hat's signal strength: red is weak, green is strong.</span></button>
<button class="mode" data-mode="4"><b>Metronome</b><span>Every hat flashes on the beat instead of following the music. White marks the first beat of each bar of 4.</span></button>
<label for="bpm">Tempo <output id="bpm_o"></output></label>
<input type="range" id="bpm" step="1">
<div class="ends"><span id="bpm_min"></span><span id="bpm_max"></span></div>
<div id="modeMsg"></div>
</section>

<script>
const $=id=>document.getElementById(id);
const VIEWS={tuning:'Tuning',hats:'Hats',modes:'Modes'};
let view='tuning',conductorVersion=null;

function setView(v){
  if(!VIEWS[v])v='tuning';
  view=v;
  for(const k in VIEWS)$('v-'+k).hidden=k!==v;
  $('title').textContent=VIEWS[v];
  document.querySelectorAll('nav a').forEach(a=>a.classList.toggle('on',a.dataset.view===v));
  closeMenu();
  pollNow();
}
function closeMenu(){$('menu').hidden=true;$('menuBtn').setAttribute('aria-expanded','false')}
$('menuBtn').addEventListener('click',e=>{
  e.stopPropagation();
  const open=$('menu').hidden;
  $('menu').hidden=!open;
  $('menuBtn').setAttribute('aria-expanded',String(open));
});
document.addEventListener('click',e=>{if(!$('menu').contains(e.target))closeMenu()});
window.addEventListener('hashchange',()=>setView(location.hash.slice(1)));
function setVersion(v){conductorVersion=v;$('ver').textContent='v'+v}

const U={gate:' dB',minFreq:' Hz',release:' /s',agcDecay:' dB/s'};
const saved={};
function show(k){
  const v=$(k).value;
  $(k+'_o').innerHTML=v+U[k]+(Number(v)!==saved[k]?'<span class="saved">(saved '+saved[k]+')</span>':'');
}
function load(s){for(const k in U){
  const e=$(k),l=s.limits[k];
  e.min=l[0];e.max=l[1];e.step=l[2];e.value=s[k];saved[k]=Number(e.value);
  $(k+'_min').textContent=l[0]+U[k];$(k+'_max').textContent=l[1]+U[k];
  show(k);
}}
for(const k in U){
  const d=document.createElement('div');d.className='ends';
  d.innerHTML='<span id="'+k+'_min"></span><span id="'+k+'_max"></span>';
  $(k).after(d);
  $(k).addEventListener('input',()=>show(k));
}
async function post(url,body,ok){
  try{const r=await fetch(url,{method:'POST',body});if(!r.ok)throw 0;load(await r.json());$('msg').textContent=ok}
  catch(e){$('msg').textContent='Could not reach the hat.'}
}
$('f').addEventListener('submit',e=>{e.preventDefault();post('/api/settings',new URLSearchParams(new FormData($('f'))),'Saved.')});
$('reset').addEventListener('click',()=>{
  if(confirm('Reset all settings to their defaults? This overwrites the saved values.'))post('/api/reset',null,'Defaults restored.');
});

const NOTES=['C','C&#9839;','D','D&#9839;','E','F','F&#9839;','G','G&#9839;','A','A&#9839;','B'];
let showNote=false,lastHz=null;
try{showNote=localStorage.getItem('hzMode')==='note'}catch(e){}
function noteName(f){
  const n=Math.round(12*Math.log2(f/440)+69);
  return NOTES[((n%12)+12)%12]+(Math.floor(n/12)-1);
}
function showHz(){
  $('hz').innerHTML=lastHz===null?'&ndash;':showNote?noteName(lastHz):Math.round(lastHz);
  $('hzUnit').textContent=showNote?'note':'Hz';
}
function toggleHz(){
  showNote=!showNote;
  try{localStorage.setItem('hzMode',showNote?'note':'hz')}catch(e){}
  showHz();
}
$('hzCard').addEventListener('click',toggleHz);
$('hzCard').addEventListener('keydown',e=>{if(e.key==='Enter'||e.key===' '){e.preventDefault();toggleHz()}});
$('hzUnit').classList.add('tapHint');
showHz();

async function pollTuning(){
  try{
    const s=await (await fetch('/api/status')).json();
    setVersion(s.version);
    $('rx').textContent=s.receivers;
    $('db').textContent=s.db.toFixed(0);
    lastHz=s.gated||!(s.freq>0)?null:s.freq;
    showHz();
    $('lvl').style.width=(s.level/2.55)+'%';
    $('cards').classList.remove('off');
  }catch(e){$('cards').classList.add('off')}
}
function ago(ms){
  const s=Math.floor(ms/1000);
  if(s<2)return 'now';
  if(s<60)return s+' s ago';
  if(s<3600)return Math.floor(s/60)+' min ago';
  return Math.floor(s/3600)+' h ago';
}
function signalClass(r){return r>=-60?'good':r>=-75?'fair':'weak'}
let chanBusy=false,chanWanted=0;
function showChannel(d){
  const sel=$('chan');
  if(!sel.options.length)for(let c=d.channels[0];c<=d.channels[1];c++)
    sel.add(new Option(c+([1,6,11].includes(c)?'  (no overlap)':''),c));
  if(!chanBusy)sel.value=d.pending||d.channel;
  sel.dataset.current=d.channel;
  if(d.pending)$('chanMsg').textContent='Switching to channel '+d.pending+'…';
  else if(chanWanted&&d.channel===chanWanted){$('chanMsg').textContent='Now on channel '+d.channel+'.';chanWanted=0}
}
$('chan').addEventListener('change',async()=>{
  const sel=$('chan'),ch=Number(sel.value),cur=Number(sel.dataset.current);
  if(ch===cur)return;
  if(!confirm('Move all hats from channel '+cur+' to channel '+ch+'? Your phone will drop off and needs to rejoin conductor-hat.')){sel.value=cur;return}
  chanBusy=true;
  try{
    const r=await fetch('/api/channel',{method:'POST',body:new URLSearchParams({channel:ch})});
    if(!r.ok)throw 0;
    chanWanted=ch;
    $('chanMsg').textContent='Switching to channel '+ch+'… If this page stops updating, rejoin conductor-hat.';
  }catch(e){sel.value=cur;$('chanMsg').textContent='Could not reach the hat.'}
  chanBusy=false;
});
async function pollHats(){
  try{
    const d=await (await fetch('/api/hats')).json();
    setVersion(d.version);
    showChannel(d);
    const hats=d.hats.sort((a,b)=>(b.online-a.online)||(b.rssi-a.rssi));
    const live=hats.filter(h=>h.online).length;
    $('hatSummary').textContent=hats.length?hats.length+' seen since power-on, '+live+' online':'';
    $('hatRows').innerHTML=hats.length?hats.map(h=>
      '<tr class="'+(h.online?'':'gone')+'">'+
      '<td class="id"><span class="dot'+(h.online?' live':'')+'"></span>'+h.id+'</td>'+
      '<td class="'+(h.online?signalClass(h.rssi):'')+'">'+h.rssi+' dBm</td>'+
      '<td class="'+(h.version!==d.version?'mismatch':'')+'">v'+h.version+'</td>'+
      '<td>'+ago(h.age)+'</td></tr>').join('')
      :'<tr><td colspan="4" class="empty">No receiver hats seen yet.</td></tr>';
    $('hatTable').classList.remove('off');
  }catch(e){$('hatTable').classList.add('off')}
}

function showMode(d){
  document.querySelectorAll('.mode').forEach(b=>b.classList.toggle('on',Number(b.dataset.mode)===d.mode));
  const s=$('bpm');
  s.min=d.bpmLimits[0];s.max=d.bpmLimits[1];
  $('bpm_min').textContent=d.bpmLimits[0]+' BPM';$('bpm_max').textContent=d.bpmLimits[1]+' BPM';
  if(document.activeElement!==s){s.value=d.bpm;$('bpm_o').textContent=d.bpm+' BPM'}
}
$('bpm').addEventListener('input',()=>{$('bpm_o').textContent=$('bpm').value+' BPM'});
$('bpm').addEventListener('change',()=>{postMode({bpm:$('bpm').value});$('bpm').blur()});
async function postMode(body){
  try{
    const r=await fetch('/api/mode',{method:'POST',body:new URLSearchParams(body)});
    if(!r.ok)throw 0;
    showMode(await r.json());
    $('modeMsg').textContent='';
  }catch(e){$('modeMsg').textContent='Could not reach the hat.'}
}
document.querySelectorAll('.mode').forEach(b=>b.addEventListener('click',()=>postMode({mode:b.dataset.mode})));
async function pollModes(){
  try{showMode(await (await fetch('/api/mode')).json())}catch(e){}
}

let timer=null;
async function pollNow(){
  clearTimeout(timer);
  if(view==='hats'){await pollHats();timer=setTimeout(pollNow,1000)}
  else if(view==='modes'){await pollModes();timer=setTimeout(pollNow,1000)}
  else{await pollTuning();timer=setTimeout(pollNow,500)}
}
fetch('/api/settings').then(r=>r.json()).then(load);
setView(location.hash.slice(1));
</script>
</body>
</html>
)HTML";

static void sendJson(const char *json) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

static void sendSettings() {
  const Settings &s = settings();
  char buf[384];
  snprintf(buf, sizeof(buf),
           "{\"gate\":%.0f,\"minFreq\":%.0f,\"release\":%.0f,\"agcDecay\":%.1f,"
           "\"limits\":{\"gate\":[%.0f,%.0f,1],\"minFreq\":[%.0f,%.0f,5],"
           "\"release\":[%.0f,%.0f,10],\"agcDecay\":[%.1f,%.1f,0.1]}}",
           s.gateDbfs, s.minFreqHz, s.releasePerS, s.agcDecayDbPerS, GATE_DBFS_MIN, GATE_DBFS_MAX, MIN_FREQ_HZ_MIN,
           MIN_FREQ_HZ_MAX, LEVEL_RELEASE_MIN, LEVEL_RELEASE_MAX, AGC_DECAY_MIN, AGC_DECAY_MAX);
  sendJson(buf);
}

static void handleStatus() {
  char buf[192];
  snprintf(buf, sizeof(buf),
           "{\"version\":%u,\"receivers\":%d,\"db\":%.1f,\"freq\":%.0f,\"level\":%u,\"gated\":%s}", FIRMWARE_VERSION,
           radioReceiverCount(), lastFrame.db, lastFrame.freq, lastFrame.level, lastFrame.gated ? "true" : "false");
  sendJson(buf);
}

static void handleHats() {
  static HatInfo hats[MAX_RECEIVERS];
  int n = radioHats(hats, MAX_RECEIVERS);
  uint32_t now = millis();
  String json;
  json.reserve(48 + n * 80);
  char head[128];
  snprintf(head, sizeof(head), "{\"version\":%u,\"channel\":%u,\"pending\":%u,\"channels\":[%u,%u],\"hats\":[",
           FIRMWARE_VERSION, radioChannel(), radioPendingChannel(), CHANNEL_MIN, CHANNEL_MAX);
  json += head;
  for (int i = 0; i < n; i++) {
    const HatInfo &h = hats[i];
    uint32_t age = now - h.lastMs;
    char buf[96];
    snprintf(buf, sizeof(buf), "%s{\"id\":\"%02X:%02X:%02X\",\"rssi\":%d,\"version\":%u,\"age\":%lu,\"online\":%s}",
             i ? "," : "", h.mac[3], h.mac[4], h.mac[5], h.rssi, h.version, (unsigned long)age,
             age <= RECEIVER_TIMEOUT_MS ? "true" : "false");
    json += buf;
  }
  json += "]}";
  sendJson(json.c_str());
}

static void handleSettingsPost() {
  Settings s = settings();
  if (server.hasArg("gate")) s.gateDbfs = server.arg("gate").toFloat();
  if (server.hasArg("minFreq")) s.minFreqHz = server.arg("minFreq").toFloat();
  if (server.hasArg("release")) s.releasePerS = server.arg("release").toFloat();
  if (server.hasArg("agcDecay")) s.agcDecayDbPerS = server.arg("agcDecay").toFloat();
  settingsSave(s);
  sendSettings();
}

static void handleChannel() {
  if (!server.hasArg("channel") || !radioRequestChannel((uint8_t)server.arg("channel").toInt())) {
    server.send(400, "text/plain", "bad channel");
    return;
  }
  char buf[64];
  snprintf(buf, sizeof(buf), "{\"channel\":%u,\"pending\":%u}", radioChannel(), radioPendingChannel());
  sendJson(buf);
}

static void sendMode() {
  char buf[96];
  snprintf(buf, sizeof(buf), "{\"mode\":%u,\"bpm\":%u,\"bpmLimits\":[%u,%u]}", modeCurrent(), modeBpm(), BPM_MIN,
           BPM_MAX);
  sendJson(buf);
}

static void handleModePost() {
  if (server.hasArg("mode") && !modeSet((uint8_t)server.arg("mode").toInt())) {
    server.send(400, "text/plain", "bad mode");
    return;
  }
  if (server.hasArg("bpm") && !modeSetBpm((uint16_t)server.arg("bpm").toInt())) {
    server.send(400, "text/plain", "bad bpm");
    return;
  }
  sendMode();
}

static void handleReset() {
  settingsReset();
  sendSettings();
}

void webBegin() {
  server.on("/", HTTP_GET, []() { server.send(200, "text/html", PAGE); });
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/hats", HTTP_GET, handleHats);
  server.on("/api/settings", HTTP_GET, sendSettings);
  server.on("/api/settings", HTTP_POST, handleSettingsPost);
  server.on("/api/reset", HTTP_POST, handleReset);
  server.on("/api/channel", HTTP_POST, handleChannel);
  server.on("/api/mode", HTTP_GET, sendMode);
  server.on("/api/mode", HTTP_POST, handleModePost);
  server.onNotFound([]() {
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });
  server.begin();
}

void webUpdate(const AudioFrame &frame) {
  lastFrame = frame;
}

void webHandle() {
  server.handleClient();
}
