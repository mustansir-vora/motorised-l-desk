#pragma once
// Phone app served by the desk at http://desk.local
// Open it on your phone, then "Add to Home Screen" for an app icon.

const char APP_MANIFEST[] PROGMEM = R"JSON({
  "name": "Desk", "short_name": "Desk", "start_url": "/", "display": "standalone",
  "background_color": "#1f2529", "theme_color": "#1f2529",
  "icons": [{"src": "/icon.svg", "sizes": "any", "type": "image/svg+xml"}]
})JSON";

const char APP_ICON[] PROGMEM = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 96 96">
<rect width="96" height="96" rx="22" fill="#1f2529"/>
<rect x="16" y="30" width="64" height="9" rx="2" fill="#c9a77c"/>
<rect x="24" y="39" width="7" height="36" fill="#8a949b"/><rect x="65" y="39" width="7" height="36" fill="#8a949b"/>
<rect x="18" y="75" width="20" height="5" rx="2" fill="#8a949b"/><rect x="58" y="75" width="20" height="5" rx="2" fill="#8a949b"/>
<path d="M48 12l8 9H40z" fill="#ffb000"/></svg>)SVG";

const char APP_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover, user-scalable=no">
<meta name="theme-color" content="#1f2529">
<meta name="apple-mobile-web-app-capable" content="yes">
<link rel="manifest" href="/manifest.json">
<link rel="icon" href="/icon.svg">
<title>Desk</title>
<style>
:root{--page:#e6e9eb;--ink:#1f2529;--muted:#5d676e;--body:#1f2529;--key:#2c3338;--key-hi:#3a4248;
--key-ink:#e9edef;--amber:#ffb000;--amber-dim:rgba(255,176,0,.09);--red:#ff5a4e;--ok:#5fd18b;
box-sizing:border-box;padding-top:env(safe-area-inset-top,0px);padding-bottom:env(safe-area-inset-bottom,0px)}
@media (prefers-color-scheme:dark){:root{--page:#121619;--ink:#e9edef;--muted:#8c969e}}
*{box-sizing:inherit;-webkit-tap-highlight-color:transparent}
html,body{height:100%;margin:0}
body{background:var(--page);color:var(--ink);font:16px/1.4 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;
display:flex;justify-content:center;user-select:none;-webkit-user-select:none}
main{width:100%;max-width:440px;padding:14px 16px 20px;display:flex;flex-direction:column;gap:14px;min-height:100%}
header{display:flex;align-items:center;gap:10px}
header h1{font-size:20px;font-weight:650;margin:0;flex:1;letter-spacing:-.01em}
.link{display:flex;align-items:center;gap:6px;font-size:13px;color:var(--muted)}
.link i{width:8px;height:8px;border-radius:50%;background:var(--red)}
.link.on i{background:var(--ok)}
.gear{background:none;border:0;color:var(--ink);font-size:22px;padding:6px;cursor:pointer}
.handset{background:var(--body);border-radius:26px;padding:18px 18px 20px;display:flex;flex-direction:column;gap:16px;
box-shadow:inset 0 1px 0 rgba(255,255,255,.06),0 10px 30px rgba(10,14,16,.25)}
.lcd{background:#0c0f10;border-radius:14px;padding:16px 14px 12px;display:flex;flex-direction:column;align-items:center;gap:8px}
.lcd svg{width:100%;max-width:300px;height:auto;display:block}
.status{font-size:13px;color:#a7b0b6;min-height:18px;text-align:center}
.status.warn{color:var(--amber)}
.legs{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;width:100%}
.leg{font-size:11px;color:#7d878e;display:flex;flex-direction:column;gap:4px;align-items:center}
.leg b{display:block;width:100%;height:4px;border-radius:2px;background:#22292d;position:relative;overflow:hidden}
.leg b span{position:absolute;inset:0 auto 0 0;background:var(--amber);opacity:.75}
.move{display:grid;grid-template-columns:1fr 1fr;gap:12px}
.key{appearance:none;border:0;border-radius:18px;background:linear-gradient(var(--key-hi),var(--key));color:var(--key-ink);
font:600 17px/1 system-ui,sans-serif;cursor:pointer;touch-action:none;
box-shadow:0 2px 0 #14191c,inset 0 1px 0 rgba(255,255,255,.07)}
.key:active,.key.held{transform:translateY(2px);box-shadow:0 0 0 #14191c;background:var(--key)}
.key:focus-visible{outline:3px solid var(--amber);outline-offset:2px}
.arrow{height:120px;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:6px}
.arrow svg{width:34px;height:34px}
.arrow small{font-size:12px;font-weight:500;color:#9aa4ab}
.presets{display:grid;grid-template-columns:repeat(3,1fr);gap:12px}
.preset{height:76px;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:5px;position:relative}
.preset .n{font-size:24px;font-weight:650}
.preset .v{font-size:12px;color:#9aa4ab;font-variant-numeric:tabular-nums}
.preset.saving::after{content:"";position:absolute;left:12px;right:12px;bottom:8px;height:3px;border-radius:2px;
background:var(--amber);transform-origin:left;animation:fill .7s linear forwards}
.preset.active .n{color:var(--amber)}
@keyframes fill{from{transform:scaleX(0)}to{transform:scaleX(1)}}
.hint{font-size:12px;color:#7d878e;text-align:center;margin:-4px 0 0}
.stop{height:56px;width:100%;background:var(--red);color:#fff;box-shadow:0 2px 0 #9b2d25;display:none}
.stop.show{display:block}
dialog{border:0;border-radius:22px;padding:0;width:min(440px,94vw);background:var(--page);color:var(--ink)}
dialog::backdrop{background:rgba(10,14,16,.55)}
.sheet{padding:20px;display:flex;flex-direction:column;gap:18px}
.sheet h2{margin:0;font-size:19px}
.sheet h3{margin:0 0 6px;font-size:15px}
.sheet p{margin:0 0 8px;font-size:13px;color:var(--muted)}
.row{display:flex;gap:8px}
.row input{flex:1;min-width:0;font:16px system-ui,sans-serif;padding:10px 12px;border-radius:10px;
border:1px solid color-mix(in srgb,var(--muted) 40%,transparent);background:transparent;color:var(--ink)}
.btn{font:600 14px system-ui,sans-serif;padding:10px 14px;border-radius:10px;border:0;background:var(--ink);color:var(--page);cursor:pointer}
.btn.ghost{background:transparent;color:var(--ink);border:1px solid color-mix(in srgb,var(--muted) 50%,transparent)}
.grid3{display:grid;grid-template-columns:repeat(3,1fr);gap:8px}
.grid3 input{width:100%;font:16px system-ui,sans-serif;padding:10px;border-radius:10px;
border:1px solid color-mix(in srgb,var(--muted) 40%,transparent);background:transparent;color:var(--ink)}
.meta{font-size:12px;color:var(--muted)}
.toast{position:fixed;left:50%;bottom:calc(20px + env(safe-area-inset-bottom,0px));transform:translateX(-50%);
background:var(--ink);color:var(--page);padding:10px 16px;border-radius:12px;font-size:14px;opacity:0;transition:opacity .2s;pointer-events:none}
.toast.show{opacity:1}
@media (prefers-reduced-motion:reduce){.preset.saving::after{animation:none;transform:none}}
</style></head>
<body><main>
<header><h1>Desk</h1><span class="link" id="link"><i></i><span>Connecting</span></span>
<button class="gear" id="gear" aria-label="Settings">&#9881;</button></header>

<section class="handset" aria-label="Desk controls">
  <div class="lcd">
    <div id="seg" role="img" aria-label="Desk height"></div>
    <div class="status" id="status"></div>
    <div class="legs" aria-label="Leg heights">
      <div class="leg"><b><span id="l0"></span></b>Left</div>
      <div class="leg"><b><span id="l1"></span></b>Corner</div>
      <div class="leg"><b><span id="l2"></span></b>Return</div>
    </div>
  </div>
  <div class="move">
    <button class="key arrow" id="up" aria-label="Hold to raise">
      <svg viewBox="0 0 24 24"><path d="M12 5l8 10H4z" fill="currentColor"/></svg><small>Hold to raise</small></button>
    <button class="key arrow" id="down" aria-label="Hold to lower">
      <svg viewBox="0 0 24 24"><path d="M12 19l8-10H4z" fill="currentColor"/></svg><small>Hold to lower</small></button>
  </div>
  <div class="presets">
    <button class="key preset" data-n="1"><span class="n">1</span><span class="v" id="pv1">–</span></button>
    <button class="key preset" data-n="2"><span class="n">2</span><span class="v" id="pv2">–</span></button>
    <button class="key preset" data-n="3"><span class="n">3</span><span class="v" id="pv3">–</span></button>
  </div>
  <p class="hint">Tap a number to go there. Hold it to save the current height.</p>
  <button class="key stop" id="stop">Stop</button>
</section>
</main>

<dialog id="sheet"><form method="dialog" class="sheet">
  <h2>Settings</h2>
  <div><h3>Reset to bottom</h3>
    <p>Lowers all three legs to their bottom stops and re-zeroes them. Needed after a power cut, an "Err" code, or when the height shows dashes.</p>
    <div class="row"><button type="button" class="btn" id="home">Reset desk</button>
    <button type="button" class="btn ghost" id="clear">Clear error</button></div></div>
  <div><h3>Measured height</h3>
    <p>Measure from floor to desktop and enter it in millimetres to correct the display.</p>
    <div class="row"><input id="mh" type="number" inputmode="numeric" placeholder="e.g. 728"><button type="button" class="btn" id="setH">Save height</button></div></div>
  <div><h3>Calibrate travel</h3>
    <p id="calText">Measure the height and record point 1. Move the desk at least 20 cm, measure again and record point 2.</p>
    <div class="row"><input id="cp" type="number" inputmode="numeric" placeholder="height in mm"><button type="button" class="btn" id="calBtn">Record point 1</button></div></div>
  <div><h3>Preset heights (mm)</h3>
    <div class="grid3"><input id="e1" type="number" inputmode="numeric"><input id="e2" type="number" inputmode="numeric"><input id="e3" type="number" inputmode="numeric"></div>
    <div class="row" style="margin-top:8px"><button type="button" class="btn" id="saveP">Save presets</button></div></div>
  <div><h3>Highest position (mm)</h3>
    <div class="row"><input id="mx" type="number" inputmode="numeric"><button type="button" class="btn" id="saveMx">Save limit</button></div></div>
  <div class="meta" id="meta"></div>
  <div class="row"><button class="btn ghost" value="close">Done</button></div>
</form></dialog>
<div class="toast" id="toast"></div>

<script>
const $ = id => document.getElementById(id);
let S = null, online = false, holdTimer = null, holding = null;

// ---- seven-segment display, drawn like the desk's keypad LEDs
const SEGS = {'0':'abcdef','1':'bc','2':'abdeg','3':'abcdg','4':'bcfg','5':'acdfg','6':'acdefg','7':'abc','8':'abcdefg',
'9':'abcdfg','-':'g',' ':'','E':'adefg','r':'eg','S':'acdfg','t':'defg','O':'abcdef','P':'abefg'};
const H = (x1,x2,y)=>`${x1},${y} ${x1+4},${y-4} ${x2-4},${y-4} ${x2},${y} ${x2-4},${y+4} ${x1+4},${y+4}`;
const V = (x,y1,y2)=>`${x},${y1} ${x+4},${y1+4} ${x+4},${y2-4} ${x},${y2} ${x-4},${y2-4} ${x-4},${y1+4}`;
const SHAPE = {a:H(9,41,6),g:H(9,41,45),d:H(9,41,84),f:V(6,9,42),b:V(44,9,42),e:V(6,48,81),c:V(44,48,81)};
function seg7(text){
  const cells=[]; for(const ch of text){ if(ch==='.'&&cells.length) cells[cells.length-1].dot=true; else cells.push({ch,dot:false}); }
  while(cells.length<4) cells.unshift({ch:' ',dot:false});
  let g='';
  cells.forEach((c,i)=>{ const on=SEGS[c.ch]||''; g+=`<g transform="translate(${i*60} 0) skewX(-6)">`;
    for(const s in SHAPE) g+=`<polygon points="${SHAPE[s]}" fill="${on.includes(s)?'var(--amber)':'var(--amber-dim)'}"/>`;
    g+=`<circle cx="53" cy="84" r="4" fill="${c.dot?'var(--amber)':'var(--amber-dim)'}"/></g>`; });
  return `<svg viewBox="-6 -2 ${cells.length*60+4} 94" aria-hidden="true">${g}</svg>`;
}
function heightText(mm){ const v=Math.round(mm); const s=String(v); return s.slice(0,-1)+'.'+s.slice(-1); }

// ---- network
async function api(path){
  const r = await fetch(path,{signal:AbortSignal.timeout(1800),cache:'no-store'});
  const j = await r.json();
  if(!r.ok) throw new Error(j.error||'Request failed');
  return j;
}
function toast(t){ const el=$('toast'); el.textContent=t; el.classList.add('show'); clearTimeout(el._t); el._t=setTimeout(()=>el.classList.remove('show'),2200); }
async function act(path,okMsg){ try{ render(await api(path)); if(okMsg) toast(okMsg); }catch(e){ toast(e.message); } }

function render(s){
  S=s;
  let txt;
  if(s.mode==='fault') txt='Err'+s.fault;
  else if(s.mode==='homing') txt='rSt ';
  else if(!s.homed) txt='----';
  else txt=heightText(s.h);
  $('seg').innerHTML=seg7(txt);
  $('seg').setAttribute('aria-label', s.homed ? `Desk height ${(s.h/10).toFixed(1)} centimetres` : 'Height unknown');
  const st=$('status'); st.classList.remove('warn');
  if(s.mode==='fault'){ st.textContent=s.fault==1?'Legs out of step. Open settings and reset.':'Reset failed. Check leg wiring.'; st.classList.add('warn'); }
  else if(s.mode==='homing') st.textContent='Finding the bottom stop…';
  else if(!s.homed){ st.textContent='Position unknown. Reset the desk in settings.'; st.classList.add('warn'); }
  else if(s.mode==='goto') st.textContent=`Moving to ${heightText(s.target)} cm`;
  else if(s.mode==='manual') st.textContent=s.dir>0?'Raising':'Lowering';
  else if(s.saveArmed) st.textContent='Press 1, 2 or 3 on the keypad to save';
  else st.textContent='cm';
  const span=Math.max(1,s.max-s.min);
  s.legs.forEach((l,i)=>{ $('l'+i).style.width=Math.max(0,Math.min(100,(l-s.min)/span*100))+'%'; });
  s.presets.forEach((p,i)=>{ $('pv'+(i+1)).textContent=heightText(p)+' cm';
    document.querySelector(`.preset[data-n="${i+1}"]`).classList.toggle('active', s.homed && Math.abs(s.h-p)<3); });
  $('stop').classList.toggle('show', s.mode==='goto' || s.mode==='homing');
  $('meta').textContent=`${s.cpm.toFixed(2)} pulses/mm · range ${s.min}–${s.max} mm · Wi-Fi ${s.rssi} dBm`;
  $('calBtn').textContent = s.calStep ? 'Record point 2' : 'Record point 1';
}
function setOnline(v){ online=v; const l=$('link'); l.classList.toggle('on',v); l.lastElementChild.textContent=v?'Connected':'Offline'; }

async function poll(){
  try{ render(await api('/api/state')); setOnline(true); }catch(e){ setOnline(false); }
  const busy = S && S.mode!=='idle' && S.mode!=='fault';
  setTimeout(poll, busy?250:900);
}

// ---- hold-to-move with a deadman: the desk stops if these pings stop
function startHold(dir,btn){
  if(holding) return; holding=dir; btn.classList.add('held');
  if(navigator.vibrate) navigator.vibrate(15);
  const ping=()=>api('/api/move?dir='+dir).then(render).catch(()=>{});
  ping(); holdTimer=setInterval(ping,150);
}
function endHold(){
  if(!holding) return; holding=null; clearInterval(holdTimer);
  document.querySelectorAll('.held').forEach(b=>b.classList.remove('held'));
  api('/api/stop').then(render).catch(()=>{});
}
for(const [id,dir] of [['up','up'],['down','down']]){
  const b=$(id);
  b.addEventListener('pointerdown',e=>{ e.preventDefault(); b.setPointerCapture(e.pointerId); startHold(dir,b); });
  ['pointerup','pointercancel','lostpointercapture'].forEach(ev=>b.addEventListener(ev,endHold));
  b.addEventListener('keydown',e=>{ if((e.key===' '||e.key==='Enter')&&!e.repeat){ e.preventDefault(); startHold(dir,b);} });
  b.addEventListener('keyup',e=>{ if(e.key===' '||e.key==='Enter') endHold(); });
  b.addEventListener('contextmenu',e=>e.preventDefault());
}
document.addEventListener('visibilitychange',()=>{ if(document.hidden) endHold(); });
window.addEventListener('blur',endHold);

// ---- presets: tap = go, hold 0.7 s = save
document.querySelectorAll('.preset').forEach(b=>{
  let t=null, saved=false;
  b.addEventListener('pointerdown',e=>{ e.preventDefault(); saved=false; b.classList.add('saving');
    t=setTimeout(()=>{ saved=true; b.classList.remove('saving'); if(navigator.vibrate) navigator.vibrate([30,40,30]);
      act('/api/save?n='+b.dataset.n,`Saved preset ${b.dataset.n}`); },700); });
  const up=()=>{ if(t===null) return; clearTimeout(t); t=null; b.classList.remove('saving');
    if(!saved) act('/api/preset?n='+b.dataset.n); };
  b.addEventListener('pointerup',up);
  b.addEventListener('pointercancel',()=>{ clearTimeout(t); t=null; b.classList.remove('saving'); });
  b.addEventListener('pointerleave',()=>{ clearTimeout(t); t=null; b.classList.remove('saving'); });
  b.addEventListener('keydown',e=>{ if(e.key==='Enter'||e.key===' '){ e.preventDefault(); act('/api/preset?n='+b.dataset.n);} });
  b.addEventListener('contextmenu',e=>e.preventDefault());
});
$('stop').addEventListener('click',()=>act('/api/stop'));

// ---- settings
$('gear').addEventListener('click',()=>{ if(S){ [1,2,3].forEach(i=>$('e'+i).value=S.presets[i-1]); $('mx').value=S.max; } $('sheet').showModal(); });
$('home').addEventListener('click',()=>{ if(confirm('Lower the desk to the bottom and re-zero? Clear the space under the desk first.')){ $('sheet').close(); act('/api/home','Resetting'); } });
$('clear').addEventListener('click',()=>act('/api/clear','Error cleared'));
$('setH').addEventListener('click',()=>{ const v=+$('mh').value; if(v>300) act('/api/calibrate?mm='+v,'Height saved'); else toast('Enter the height in mm'); });
$('calBtn').addEventListener('click',()=>{ const v=+$('cp').value; if(v<300) return toast('Enter the height in mm');
  const second=S&&S.calStep; act('/api/calpoint?mm='+v, second?'Travel calibrated':'Point 1 recorded. Move the desk and measure again'); $('cp').value=''; });
$('saveP').addEventListener('click',async()=>{ try{ for(const i of [1,2,3]){ const v=+$('e'+i).value; if(v>300) render(await api(`/api/save?n=${i}&mm=${v}`)); } toast('Presets saved'); }catch(e){ toast(e.message); } });
$('saveMx').addEventListener('click',()=>{ const v=+$('mx').value; if(v>300) act('/api/limits?max='+v,'Limit saved'); });

$('seg').innerHTML=seg7('----');
poll();
</script></body></html>)HTML";
