// Auto-loaded by FruitNinja_ESP32.ino via #include "index_html.h"
// Contains the entire game hub (HTML + CSS + JS) as one PROGMEM string served at "/"
// Four games share one canvas, one sensor/button polling loop, and one high-score store:
//   Space War (tilt), Sky Catch (tilt), Tilt Maze (tilt), Reaction Rush (buttons only).

const char INDEX_HTML[] PROGMEM = R"HTMLPAGE(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
<title>ESP32 Game Hub</title>
<style>
  html,body { margin:0; padding:0; overflow:hidden; background:#02040a; height:100%; touch-action:none;
              font-family:'Segoe UI', system-ui, sans-serif; }
  canvas { display:block; }
  #vignette {
    position:fixed; inset:0; pointer-events:none; z-index:4;
    background:radial-gradient(ellipse at center, rgba(0,0,0,0) 55%, rgba(0,0,0,0.6) 100%);
  }
  #hud {
    position:fixed; top:14px; left:14px; z-index:5; color:#fff;
    background:linear-gradient(#0d2038,#081326); border:3px solid #2fa6ff; border-radius:12px;
    padding:8px 16px; box-shadow:0 4px 14px rgba(0,140,255,0.35);
  }
  #hud .score { font-size:26px; font-weight:800; text-shadow:0 0 8px #2fd0ff; }
  #hearts { margin-top:4px; font-size:20px; letter-spacing:4px; }
  #hearts .h { color:#2fe0a0; }
  #hearts .e { color:#3a3a3a; }
  #topbar { position:fixed; top:14px; right:14px; z-index:5; display:flex; gap:8px; }
  #topbar button {
    background:linear-gradient(#5ad4ff,#0d8fdb); border:2px solid #0a5c8f; border-radius:10px;
    padding:9px 14px; font-size:13px; font-weight:700; cursor:pointer; color:#03202f;
    box-shadow:0 3px 0 #094768;
  }
  #topbar button:active { transform:translateY(2px); box-shadow:none; }
  #status { position:fixed; bottom:8px; left:12px; color:#7fefc0; font-family:monospace; font-size:11px;
            z-index:5; opacity:0.7; transition:opacity 0.3s; }
  #toast {
    position:fixed; top:14px; left:50%; transform:translateX(-50%); z-index:20;
    background:linear-gradient(#ff6a4d,#c22a12); color:#fff; padding:10px 22px; border-radius:10px;
    font-weight:700; font-size:14px; box-shadow:0 4px 14px rgba(0,0,0,0.4); opacity:0; transition:opacity 0.3s;
  }
  #toast.show { opacity:1; }
  #overlay {
    position:fixed; inset:0; display:flex; flex-direction:column; align-items:center; justify-content:center;
    background:radial-gradient(ellipse at center, rgba(10,20,45,0.88), rgba(0,0,2,0.94));
    color:#fff; text-align:center; z-index:10; padding:20px; box-sizing:border-box; overflow-y:auto;
  }
  #overlay h1 {
    font-size:40px; margin:0 0 4px 0; letter-spacing:3px;
    background:linear-gradient(#eafcff,#2fd0ff); -webkit-background-clip:text; background-clip:text; color:transparent;
    text-shadow:0 0 30px rgba(47,208,255,0.55);
  }
  #overlay p { max-width:440px; line-height:1.5; color:#cfe8f5; font-size:15px; }
  #overlay .highscore { color:#2fd0ff; font-weight:700; margin-top:6px; }
  #overlay button.play {
    margin-top:20px; background:linear-gradient(#ff6a4d,#c22a12); color:#fff; border:none; border-radius:14px;
    padding:16px 36px; font-size:19px; font-weight:800; cursor:pointer; box-shadow:0 5px 0 #7a1a0a;
  }
  #overlay button.play:active { transform:translateY(3px); box-shadow:none; }
  #overlay button.back {
    margin-top:12px; background:transparent; border:2px solid #2fa6ff; color:#cfe8f5; border-radius:12px;
    padding:10px 26px; font-size:14px; font-weight:700; cursor:pointer;
  }
  #overlay button.back:active { transform:translateY(2px); }
  .hub-grid {
    display:grid; grid-template-columns:repeat(auto-fit,minmax(150px,1fr)); gap:16px;
    max-width:640px; width:100%; margin-top:18px;
  }
  .tile {
    background:linear-gradient(#0d2038,#081326); border:3px solid #2fa6ff; border-radius:14px;
    padding:18px 10px; cursor:pointer; box-shadow:0 4px 14px rgba(0,140,255,0.25);
    transition:transform 0.15s;
  }
  .tile:hover { transform:translateY(-3px); }
  .tile:active { transform:translateY(1px); }
  .tile .icon { font-size:38px; }
  .tile h3 { margin:8px 0 4px; font-size:16px; color:#eafcff; }
  .tile .sub { font-size:11.5px; color:#9fc9e0; min-height:30px; line-height:1.4; }
  .tile .hs { font-size:12px; color:#2fd0ff; margin-top:8px; font-weight:700; }
  .hidden { display:none !important; }
</style>
</head>
<body>

<canvas id="game"></canvas>
<div id="vignette"></div>

<div id="hud" class="hidden">
  <div class="score">Score: <span id="score">0</span></div>
  <div id="hearts"></div>
</div>
<div id="topbar">
  <button id="btnMenu" class="hidden" onclick="backToHub()">Menu</button>
  <button id="btnCalibrate" class="hidden" onclick="calibrate()">Calibrate</button>
</div>
<div id="status">connecting to sensor...</div>
<div id="toast" class="hidden"></div>

<div id="overlay">
  <div id="overlayHub">
    <h1>GAME HUB</h1>
    <p>Tilt the sensor or use the two buttons to play. Pick a game:</p>
    <div class="hub-grid" id="hubGrid"></div>
    <p style="margin-top:18px;font-size:12px;opacity:0.7;">
      Tip: after Game Over, hold BOTH buttons to play again.<br>
      Hold ONE button for 3 seconds (from any menu) to clear all high scores.
    </p>
  </div>
  <div id="overlayGame" class="hidden">
    <h1 id="ogTitle">TITLE</h1>
    <p id="ogText">description</p>
    <div class="highscore" id="ogHighscore">High Score: 0</div>
    <button class="play" id="ogButton" onclick="ogButtonClick()">Launch</button><br>
    <button class="back" onclick="backToHub()">Back to Hub</button>
  </div>
</div>

<script>
const canvas = document.getElementById('game');
const ctx = canvas.getContext('2d');
function resize(){ canvas.width = window.innerWidth; canvas.height = window.innerHeight; }
window.addEventListener('resize', resize);
resize();

// ---------------- Sound (synthesized, no files needed) ----------------
let audioCtx = null;
function ensureAudio() { if (!audioCtx) audioCtx = new (window.AudioContext || window.webkitAudioContext)(); }
function tone(freq, dur, type, vol) {
  if (!audioCtx) return;
  const osc = audioCtx.createOscillator();
  const gain = audioCtx.createGain();
  osc.type = type; osc.frequency.value = freq;
  gain.gain.setValueAtTime(vol, audioCtx.currentTime);
  gain.gain.exponentialRampToValueAtTime(0.001, audioCtx.currentTime + dur);
  osc.connect(gain); gain.connect(audioCtx.destination);
  osc.start(); osc.stop(audioCtx.currentTime + dur);
}
function noiseBurst(duration, volume, startFreq, endFreq) {
  if (!audioCtx) return;
  const bufferSize = Math.floor(audioCtx.sampleRate * duration);
  const buffer = audioCtx.createBuffer(1, bufferSize, audioCtx.sampleRate);
  const data = buffer.getChannelData(0);
  for (let i=0;i<bufferSize;i++) data[i] = (Math.random()*2-1) * (1 - i/bufferSize);
  const noise = audioCtx.createBufferSource();
  noise.buffer = buffer;
  const filter = audioCtx.createBiquadFilter();
  filter.type = 'lowpass';
  filter.frequency.setValueAtTime(startFreq || 1200, audioCtx.currentTime);
  filter.frequency.exponentialRampToValueAtTime(endFreq || 100, audioCtx.currentTime + duration);
  const gain = audioCtx.createGain();
  gain.gain.setValueAtTime(volume, audioCtx.currentTime);
  gain.gain.exponentialRampToValueAtTime(0.001, audioCtx.currentTime + duration);
  noise.connect(filter); filter.connect(gain); gain.connect(audioCtx.destination);
  noise.start();
}

function sndFire(){ tone(920 + Math.random()*80, 0.045, 'square', 0.035); }
function sndHit(){
  tone(500 + Math.random()*400, 0.09, 'triangle', 0.10);
  noiseBurst(0.05, 0.05, 2600, 500);
}
function sndDefuse(){ tone(1400, 0.08, 'square', 0.09); setTimeout(()=>tone(700,0.1,'square',0.07),60); }
function sndExplosion(){
  noiseBurst(0.55, 0.4, 1500, 60);
  tone(90, 0.4, 'sawtooth', 0.22);
  setTimeout(()=>tone(50, 0.3, 'square', 0.16), 50);
}
function sndPowerUp(){ tone(1100,0.14,'sine',0.13); setTimeout(()=>tone(1500,0.16,'sine',0.11),90); }
function sndCombo(){ tone(1000 + Math.random()*200,0.1,'square',0.08); }
function sndLose(){ tone(280,0.55,'sawtooth',0.22); }
function sndEnemyHit(){ tone(200,0.12,'sawtooth',0.09); }
function sndSpawnEnemy(){
  if (!audioCtx) return;
  const osc = audioCtx.createOscillator();
  const gain = audioCtx.createGain();
  osc.type = 'sine';
  osc.frequency.setValueAtTime(620, audioCtx.currentTime);
  osc.frequency.exponentialRampToValueAtTime(280, audioCtx.currentTime + 0.13);
  gain.gain.setValueAtTime(0.05, audioCtx.currentTime);
  gain.gain.exponentialRampToValueAtTime(0.001, audioCtx.currentTime + 0.15);
  osc.connect(gain); gain.connect(audioCtx.destination);
  osc.start(); osc.stop(audioCtx.currentTime + 0.15);
}
function sndSpawnMine(){
  if (!audioCtx) return;
  const osc = audioCtx.createOscillator();
  const gain = audioCtx.createGain();
  osc.type = 'square';
  osc.frequency.setValueAtTime(180, audioCtx.currentTime);
  osc.frequency.exponentialRampToValueAtTime(110, audioCtx.currentTime + 0.18);
  gain.gain.setValueAtTime(0.07, audioCtx.currentTime);
  gain.gain.exponentialRampToValueAtTime(0.001, audioCtx.currentTime + 0.2);
  osc.connect(gain); gain.connect(audioCtx.destination);
  osc.start(); osc.stop(audioCtx.currentTime + 0.2);
}
function sndSpawnOrb(){
  tone(900, 0.08, 'sine', 0.06);
  setTimeout(()=>tone(1300, 0.1, 'sine', 0.06), 60);
}
function sndBomb(){ noiseBurst(0.4, 0.35, 1200, 80); tone(90, 0.3, 'sawtooth', 0.2); }
function sndWebThrow(){ tone(500,0.05,'square',0.06); setTimeout(()=>tone(750,0.06,'square',0.06),40); }
function sndCrash(){ noiseBurst(0.3, 0.3, 1000, 100); tone(120, 0.25, 'sawtooth', 0.18); }
function sndMazeGoal(){
  tone(900,0.1,'sine',0.1); setTimeout(()=>tone(1300,0.14,'sine',0.1),80); setTimeout(()=>tone(1700,0.16,'sine',0.1),160);
}
function sndMazeHazard(){ tone(160,0.25,'sawtooth',0.18); noiseBurst(0.2,0.2,800,150); }
function sndReactionGo(){ tone(700,0.08,'square',0.08); }
function sndReactionGood(){ tone(1200,0.07,'square',0.09); }
function sndReactionBad(){ tone(150,0.2,'sawtooth',0.16); }

// ---------------- Sensor polling ----------------
let sensor = {ax:0, ay:0, az:1, gx:0, gy:0, gz:0, btn1:false, btn2:false};
let offset = {ax:0, ay:0};
let smooth = {ax:0, ay:0};

// ---- Tilt mapping / feel config (shared by every tilt-controlled game) ----
const SWAP_AXES   = false;
const INVERT_X    = true;
const INVERT_Y    = false;
const SENSITIVITY = 13;
const DAMPING     = 0.80;
const SMOOTHING   = 0.25;
const MAX_SPEED   = 16;
const SHIP_BOTTOM_MARGIN = 90;

function tiltX() {
  smooth.ax += (sensor.ax - smooth.ax) * SMOOTHING;
  smooth.ay += (sensor.ay - smooth.ay) * SMOOTHING;
  let rawX = SWAP_AXES ? (smooth.ay - offset.ay) : (smooth.ax - offset.ax);
  if (INVERT_X) rawX = -rawX;
  return rawX;
}

// ---- Shake detection (Spider Web game) ----
// A "shake" is a sudden jump in total acceleration magnitude between two raw
// samples — unlike tilt, this deliberately looks at the UNsmoothed reading so
// a quick jerk isn't damped out. Tune SHAKE_THRESHOLD by testing on real
// hardware: too low triggers on normal handling, too high needs a hard flick.
const SHAKE_THRESHOLD = 0.7;   // required jump in |acceleration|, in g
const SHAKE_DEBOUNCE_MS = 400; // ignore further shakes for this long after one fires
let shakeMagPrev = 1;
let lastShakeTime = 0;
let pendingShake = false;

function detectShake(d) {
  const mag = Math.sqrt(d.ax*d.ax + d.ay*d.ay + d.az*d.az);
  const jerk = Math.abs(mag - shakeMagPrev);
  shakeMagPrev = mag;
  const now = performance.now();
  if (jerk > SHAKE_THRESHOLD && now - lastShakeTime > SHAKE_DEBOUNCE_MS) {
    lastShakeTime = now;
    pendingShake = true;
  }
}

// ---------------- Shared HUD (score / lives) ----------------
let score = 0;
let lives = 3;
function syncHud() {
  document.getElementById('score').textContent = score;
  updateHearts();
}
function updateHearts() {
  const el = document.getElementById('hearts');
  el.innerHTML = '';
  for (let i=0;i<3;i++) {
    const s = document.createElement('span');
    s.className = i < lives ? 'h' : 'e';
    s.textContent = '● ';
    el.appendChild(s);
  }
}

// ---------------- High scores (one per game, all clearable together) ----------------
function loadHighScores() {
  const keys = ['spacewar','maze','reaction','race','spider','jetpack','brick'];
  const hs = {};
  for (const k of keys) hs[k] = parseInt(localStorage.getItem('hub_hs_'+k) || '0');
  if (!localStorage.getItem('hub_hs_spacewar') && localStorage.getItem('sw_highscore')) {
    hs.spacewar = parseInt(localStorage.getItem('sw_highscore')) || 0;
  }
  return hs;
}
let highScores = loadHighScores();

function saveHighScore(key, value) {
  if (value > highScores[key]) {
    highScores[key] = value;
    localStorage.setItem('hub_hs_'+key, value);
    return true;
  }
  return false;
}

function clearAllHighScores() {
  for (const k of Object.keys(highScores)) {
    highScores[k] = 0;
    localStorage.removeItem('hub_hs_'+k);
  }
  localStorage.removeItem('sw_highscore');
  showToast('All high scores cleared!');
  renderHubTiles();
  if (activeGame) document.getElementById('ogHighscore').textContent = 'High Score: 0';
}

let toastTimer = null;
function showToast(msg) {
  const el = document.getElementById('toast');
  el.textContent = msg;
  el.classList.remove('hidden');
  requestAnimationFrame(() => el.classList.add('show'));
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => {
    el.classList.remove('show');
    setTimeout(() => el.classList.add('hidden'), 300);
  }, 2000);
}

// ---------------- Shared particle / floating-text effects ----------------
let particles = [];
let floats = [];
let shake = {t:0};

function burst(x, y, color, n) {
  for (let i=0;i<n;i++) {
    particles.push({
      x, y,
      vx:(Math.random()-0.5)*11, vy:(Math.random()-0.5)*11-2,
      life: 26+Math.random()*18, color
    });
  }
  if (particles.length > 220) particles.splice(0, particles.length-220);
}
function floatText(x, y, text, color, size) {
  floats.push({x, y, text, color, size: size||18, life:45, vy:-1.4});
}
function triggerShake(amount) { shake.t = amount; }

function tickSharedEffects(timeScale) {
  for (const p of particles) { p.vy += 0.18*timeScale; p.x += p.vx; p.y += p.vy; p.life -= 1; }
  particles = particles.filter(p => p.life > 0);
  for (const t of floats) { t.y += t.vy; t.life -= 1; }
  floats = floats.filter(t => t.life > 0);
  if (shake.t > 0) shake.t -= 1;
}
function drawSharedEffects() {
  for (const p of particles) {
    ctx.fillStyle = p.color;
    ctx.globalAlpha = Math.max(p.life/40, 0);
    ctx.beginPath(); ctx.arc(p.x, p.y, 4, 0, Math.PI*2); ctx.fill();
    ctx.globalAlpha = 1;
  }
  for (const t of floats) {
    ctx.globalAlpha = Math.max(t.life/45, 0);
    ctx.fillStyle = t.color;
    ctx.font = 'bold ' + t.size + 'px sans-serif';
    ctx.fillText(t.text, t.x, t.y);
    ctx.globalAlpha = 1;
  }
}

// ---------------- Shared ambient starfield background ----------------
let stars = [];
let nebulae = [];
function initStars() {
  stars = [];
  for (let i=0;i<90;i++) {
    stars.push({x:Math.random()*canvas.width, y:Math.random()*canvas.height,
                r:Math.random()*1.6+0.4, tw:Math.random()*Math.PI*2, speed:0.3+Math.random()*1.4});
  }
  nebulae = [];
  for (let i=0;i<5;i++) {
    nebulae.push({
      x: Math.random()*canvas.width, y: 40 + Math.random()*(canvas.height*0.6),
      scale: 0.8 + Math.random()*1.6, speed: 0.1 + Math.random()*0.2,
      hue: Math.random() < 0.5 ? '80,140,255' : '150,80,255'
    });
  }
}
function tickStars(timeScale) {
  for (const s of stars) {
    s.y += s.speed * timeScale;
    if (s.y > canvas.height) { s.y = 0; s.x = Math.random()*canvas.width; }
  }
  for (const nb of nebulae) {
    nb.y += nb.speed * timeScale;
    if (nb.y > canvas.height + 100) nb.y = -100;
  }
}
function drawSpaceBackground() {
  const g = ctx.createLinearGradient(0,0,0,canvas.height);
  g.addColorStop(0, '#04081c');
  g.addColorStop(0.55, '#0a1233');
  g.addColorStop(1, '#02030a');
  ctx.fillStyle = g;
  ctx.fillRect(0,0,canvas.width,canvas.height);

  for (const nb of nebulae) {
    ctx.save();
    ctx.translate(nb.x, nb.y);
    ctx.scale(nb.scale, nb.scale);
    ctx.fillStyle = `rgba(${nb.hue},0.10)`;
    ctx.beginPath();
    ctx.ellipse(0,0,50,26,0,0,Math.PI*2);
    ctx.ellipse(34,8,34,18,0,0,Math.PI*2);
    ctx.ellipse(-32,10,30,16,0,0,Math.PI*2);
    ctx.fill();
    ctx.restore();
  }

  for (const s of stars) {
    const tw = 0.5 + 0.5*Math.sin(performance.now()*0.003 + s.tw);
    ctx.globalAlpha = 0.4 + tw*0.6;
    ctx.fillStyle = '#ffffff';
    ctx.beginPath();
    ctx.arc(s.x, s.y, s.r, 0, Math.PI*2);
    ctx.fill();
  }
  ctx.globalAlpha = 1;
}

// ---------------- Game hub (menu / screen switching) ----------------
const GAMES = [
  {key:'spacewar', title:'Space War', icon:'\u{1F680}', tagline:'Tilt to dodge & blast enemy fighters', usesTilt:true,
   desc:'Hold the sensor flat, tap Calibrate, then tilt left/right to slide your fighter along the defense line. Your guns fire automatically — dodge enemy fire, avoid ramming mines, and shoot them from a distance instead. Fly through the blue energy core for an overdrive boost!'},
  {key:'maze', title:'Tilt Maze', icon:'\u{1F300}', tagline:'Roll the ball through the maze to the goal', usesTilt:true,
   desc:'Tilt the sensor to roll the ball through the maze and reach the glowing green goal. Avoid the red hazards — touching one sends your ball back to the start and costs a life!'},
  {key:'reaction', title:'Reaction Rush', icon:'⚡', tagline:'Mash the right button as fast as you can', usesTilt:false,
   desc:'No tilting needed! Watch the arrow — press the LEFT or RIGHT button to match it as fast as you can. Press the wrong button, too slow, or too soon and you lose a life. It gets faster the longer you survive!'},
  {key:'race', title:'Car Race', icon:'\u{1F3CE}\u{FE0F}', tagline:'Press LEFT/RIGHT to dodge the traffic', usesTilt:false,
   desc:'No tilting needed! Press the LEFT or RIGHT button to change lanes and dodge the oncoming cars. Every car you dodge scores points — the traffic gets faster the longer you survive!'},
  {key:'spider', title:'Spider Web', icon:'\u{1F577}\u{FE0F}', tagline:'Shake the device to sling a web at the bug', usesTilt:false,
   desc:'Watch the wall for the bug — SHAKE the device to make the spider throw a web at it before the ring runs out! Let too many bugs escape and the game is over.'},
  {key:'jetpack', title:'Jetpack Aviator', icon:'\u{1F9D1}\u{200D}\u{1F680}', tagline:'Hold a button to fly, dodge the barriers', usesTilt:false,
   desc:'No tilting needed! Hold either button to fire your jetpack and rise, let go to fall. Dodge the barriers and fly as far as you can!'},
  {key:'brick', title:'Brick Breaker', icon:'\u{1F9F1}', tagline:'Tilt to slide the paddle and smash the bricks', usesTilt:true,
   desc:'Tilt left/right to slide your paddle and keep the ball in play. Smash every brick to clear the level — let the ball fall past your paddle too many times and it\'s game over!'}
];

let activeGame = null; // null = at the hub menu

function gameByKey(key) { return GAMES.find(g => g.key === key); }

function renderHubTiles() {
  const grid = document.getElementById('hubGrid');
  grid.innerHTML = '';
  for (const g of GAMES) {
    const tile = document.createElement('div');
    tile.className = 'tile';
    tile.onclick = () => enterGame(g.key);
    tile.innerHTML = '<div class="icon">'+g.icon+'</div><h3>'+g.title+'</h3>'+
      '<div class="sub">'+g.tagline+'</div>'+
      '<div class="hs">Best: '+highScores[g.key]+'</div>';
    grid.appendChild(tile);
  }
}

function showHub() {
  activeGame = null;
  document.getElementById('overlayHub').classList.remove('hidden');
  document.getElementById('overlayGame').classList.add('hidden');
  document.getElementById('overlay').classList.remove('hidden');
  document.getElementById('hud').classList.add('hidden');
  document.getElementById('btnMenu').classList.add('hidden');
  document.getElementById('btnCalibrate').classList.add('hidden');
  renderHubTiles();
}

function enterGame(key) {
  activeGame = key;
  const g = gameByKey(key);
  document.getElementById('overlayHub').classList.add('hidden');
  document.getElementById('overlayGame').classList.remove('hidden');
  document.getElementById('overlay').classList.remove('hidden');
  document.getElementById('hud').classList.remove('hidden');
  document.getElementById('btnMenu').classList.remove('hidden');
  document.getElementById('btnCalibrate').classList.toggle('hidden', !g.usesTilt);
  document.getElementById('ogTitle').textContent = g.title;
  document.getElementById('ogText').textContent = g.desc;
  document.getElementById('ogHighscore').textContent = 'High Score: ' + highScores[key];
  document.getElementById('ogButton').textContent = 'Launch';
  score = 0; lives = 3; syncHud();
}

function backToHub() {
  if (activeGame === 'spacewar') swRunning = false;
  if (activeGame === 'maze') mazeRunning = false;
  if (activeGame === 'reaction') reactionRunning = false;
  if (activeGame === 'race') raceRunning = false;
  if (activeGame === 'spider') spiderRunning = false;
  if (activeGame === 'jetpack') jetRunning = false;
  if (activeGame === 'brick') brickRunning = false;
  showHub();
}

function isPlaying() {
  if (activeGame === 'spacewar') return swRunning;
  if (activeGame === 'maze') return mazeRunning;
  if (activeGame === 'reaction') return reactionRunning;
  if (activeGame === 'race') return raceRunning;
  if (activeGame === 'spider') return spiderRunning;
  if (activeGame === 'jetpack') return jetRunning;
  if (activeGame === 'brick') return brickRunning;
  return false;
}

function startActiveGame() {
  ensureAudio();
  if (activeGame === 'spacewar') swStart();
  else if (activeGame === 'maze') mazeStart();
  else if (activeGame === 'reaction') reactionStart();
  else if (activeGame === 'race') raceStart();
  else if (activeGame === 'spider') spiderStart();
  else if (activeGame === 'jetpack') jetStart();
  else if (activeGame === 'brick') brickStart();
  document.getElementById('overlay').classList.add('hidden');
}
function ogButtonClick() { startActiveGame(); }

function showGameOverOverlay(key, title, htmlDesc, isRecord) {
  document.getElementById('overlayHub').classList.add('hidden');
  document.getElementById('overlayGame').classList.remove('hidden');
  document.getElementById('overlay').classList.remove('hidden');
  document.getElementById('ogTitle').textContent = title;
  document.getElementById('ogText').innerHTML = htmlDesc;
  document.getElementById('ogHighscore').textContent = 'High Score: ' + highScores[key] +
    (isRecord && highScores[key] > 0 ? '  NEW RECORD!' : '');
  document.getElementById('ogButton').textContent = 'Play Again';
}

// ---------------- Button handling: restart (both) + hold-3s-to-clear (one) ----------------
let btnBothPrev = false;
let btnHoldStart = 0;
let btnHoldFired = false;

function handleButtons(d) {
  const b1 = !!d.btn1, b2 = !!d.btn2;
  const bothPressed = b1 && b2;
  const exactlyOne = b1 !== b2;

  // Edge-triggered: only fires on the press, not every poll while held down,
  // and only while looking at a game's instructions/game-over screen (not mid-play).
  if (bothPressed && !btnBothPrev && activeGame && !isPlaying()) {
    startActiveGame();
  }
  btnBothPrev = bothPressed;

  // Reaction Rush uses the buttons as live gameplay input, so this gesture is
  // disabled while any game is actively being played (isPlaying()) to avoid
  // wiping every high score from a fast press-and-hold during a round.
  if (exactlyOne && !isPlaying()) {
    if (btnHoldStart === 0) btnHoldStart = performance.now();
    const held = performance.now() - btnHoldStart;
    if (!btnHoldFired && held >= 3000) {
      btnHoldFired = true;
      clearAllHighScores();
    } else if (!btnHoldFired) {
      document.getElementById('status').textContent =
        'Hold to clear all high scores... ' + Math.max(0, (3000-held)/1000).toFixed(1) + 's';
      return true;
    }
  } else {
    btnHoldStart = 0;
    btnHoldFired = false;
  }
  return false;
}

async function pollSensor() {
  try {
    const r = await fetch('/data', {cache:'no-store'});
    const d = await r.json();
    sensor = d;
    detectShake(d);
    const holding = handleButtons(d);
    const statusEl = document.getElementById('status');
    if (!holding) {
      const g = gameByKey(activeGame);
      if (isPlaying() && g && !g.usesTilt) {
        statusEl.style.opacity = '0';
      } else {
        statusEl.style.opacity = '0.7';
        statusEl.textContent = 'sensor OK  ax:'+d.ax.toFixed(2)+' ay:'+d.ay.toFixed(2);
      }
    } else {
      statusEl.style.opacity = '0.9';
    }
  } catch(e) {
    document.getElementById('status').style.opacity = '0.7';
    document.getElementById('status').textContent = 'sensor connection lost, retrying...';
  }
  setTimeout(pollSensor, 20);
}

function calibrate() {
  document.getElementById('status').textContent = 'calibrating... hold still';
  const samples = [];
  const t0 = performance.now();
  (function collect(){
    samples.push({ax: sensor.ax, ay: sensor.ay});
    if (performance.now() - t0 < 400) {
      requestAnimationFrame(collect);
    } else {
      offset.ax = samples.reduce((s,v)=>s+v.ax,0)/samples.length;
      offset.ay = samples.reduce((s,v)=>s+v.ay,0)/samples.length;
      smooth.ax = offset.ax;
      smooth.ay = offset.ay;
      document.getElementById('status').textContent = 'calibrated!';
    }
  })();
}

// ================= SPACE WAR =================
let swRunning = false;
let swTimeScale = 1;
let slowmoUntil = 0;
let overdriveUntil = 0;
let lastHitTime = 0;
let comboCount = 0;
let enemies = [], bullets = [], enemyBullets = [];
let trail = [];

const ENEMY_TYPES = [
  {name:'scout',       body:'#ff3d4d', light:'#ff9aa3', dark:'#7a0010', juice:'#ff5677', shape:'fighter', score:10, r:26},
  {name:'interceptor', body:'#ff9f1c', light:'#ffd166', dark:'#8a4f00', juice:'#ffa500', shape:'wide',    score:10, r:28},
  {name:'saucer',      body:'#3fae6a', light:'#8de0b0', dark:'#0f4f2a', juice:'#5cffb0', shape:'saucer',  score:15, r:28},
  {name:'drone',       body:'#3fc7ff', light:'#b3ecff', dark:'#0a5f8a', juice:'#5cd6ff', shape:'diamond', score:10, r:22},
  {name:'cruiser',     body:'#b04dff', light:'#e0b3ff', dark:'#4a0f8a', juice:'#c98bff', shape:'hex',     score:22, r:36, shootsBack:true}
];

let ship = {x:0, y:0, vx:0, vy:0, speed:0, bank:0};

function swReset() {
  score = 0; lives = 3; comboCount = 0; swTimeScale = 1; slowmoUntil = 0; overdriveUntil = 0;
  enemies = []; bullets = []; enemyBullets = []; trail = [];
  ship.x = canvas.width/2; ship.y = canvas.height - SHIP_BOTTOM_MARGIN; ship.vx = 0; ship.vy = 0; ship.bank = 0;
  syncHud();
}
function swStart() {
  swReset();
  swRunning = true;
  lastSpawn = performance.now();
  lastFire = performance.now();
}
function swGameOver() {
  swRunning = false;
  sndLose();
  const isRecord = saveHighScore('spacewar', score);
  showGameOverOverlay('spacewar', 'Ship Destroyed',
    'Final Score: <b>' + score + '</b><br>Tilt left/right to steer, dodge fire, and shoot mines before they get close.',
    isRecord);
}

let lastSpawn = 0;
let lastFire = 0;

function spawnWave() {
  const roll = Math.random();
  let isMine = false, isOrb = false, type = null;
  if (roll < 0.09) { isMine = true; }
  else if (roll < 0.15) { isOrb = true; }
  else { type = ENEMY_TYPES[Math.floor(Math.random()*ENEMY_TYPES.length)]; }

  const x = 60 + Math.random() * (canvas.width - 120);
  const speedBoost = Math.min(score * 0.02, 6);

  if (isMine) {
    enemies.push({
      x, y: -30, vx:(Math.random()-0.5)*1.5, vy: 2.2 + Math.random()*0.8,
      r: 26, rotation: Math.random()*Math.PI*2, rotSpeed: (Math.random()-0.5)*0.05,
      isMine:true, isOrb:false, dead:false
    });
    sndSpawnMine();
  } else if (isOrb) {
    enemies.push({
      x, y: -30, vx:(Math.random()-0.5)*1.2, vy: 2.4 + Math.random()*0.6,
      r: 24, rotation: 0, rotSpeed: 0.06,
      isMine:false, isOrb:true, dead:false
    });
    sndSpawnOrb();
  } else {
    const vy = 3 + Math.random()*2 + speedBoost*0.5;
    const vx = (Math.random()-0.5) * 3;
    enemies.push({
      x, y: -30, vx, vy,
      r: type.r, rotation: 0, wobble: Math.random()*Math.PI*2,
      body:type.body, light:type.light, dark:type.dark, shape:type.shape,
      color:type.juice, scoreValue:type.score, shootsBack: !!type.shootsBack,
      lastShot: performance.now() + Math.random()*600,
      isMine:false, isOrb:false, dead:false
    });
    sndSpawnEnemy();

    if (score > 60 && Math.random() < 0.18) {
      const x2 = 60 + Math.random() * (canvas.width - 120);
      const t2 = ENEMY_TYPES[Math.floor(Math.random()*ENEMY_TYPES.length)];
      enemies.push({
        x:x2, y:-30, vx:(Math.random()-0.5)*3, vy: 3+Math.random()*2+speedBoost*0.5,
        r:t2.r, rotation:0, wobble: Math.random()*Math.PI*2,
        body:t2.body, light:t2.light, dark:t2.dark, shape:t2.shape,
        color:t2.juice, scoreValue:t2.score, shootsBack: !!t2.shootsBack,
        lastShot: performance.now() + Math.random()*600,
        isMine:false, isOrb:false, dead:false
      });
      sndSpawnEnemy();
    }
  }
}

function firePlayerBullet() {
  bullets.push({x: ship.x, y: ship.y - 22, vy: -17});
  bullets.push({x: ship.x - 10, y: ship.y - 12, vy: -17});
  bullets.push({x: ship.x + 10, y: ship.y - 12, vy: -17});
  sndFire();
}

const SHIP_HIT_RADIUS = 20;

function swUpdate() {
  if (!swRunning) return;
  const now = performance.now();

  if (now > slowmoUntil) swTimeScale = 1;
  const overdriveActive = now < overdriveUntil;

  tickStars(swTimeScale);

  const rawX = tiltX();
  ship.vx = ship.vx*DAMPING + rawX * SENSITIVITY;
  ship.vx = Math.max(-MAX_SPEED, Math.min(MAX_SPEED, ship.vx));

  ship.x += ship.vx;
  ship.speed = Math.abs(ship.vx);
  ship.x = Math.max(10, Math.min(canvas.width-10, ship.x));
  ship.y = canvas.height - SHIP_BOTTOM_MARGIN;
  ship.bank = ship.bank*0.85 + Math.max(-1, Math.min(1, ship.vx/10))*0.15;

  trail.push({x:ship.x, y:ship.y});
  if (trail.length > 10) trail.shift();

  const interval = Math.max(360, 950 - score*4);
  if (now - lastSpawn > interval) { spawnWave(); lastSpawn = now; }

  const fireInterval = overdriveActive ? 90 : 170;
  if (now - lastFire > fireInterval) { firePlayerBullet(); lastFire = now; }

  for (const b of bullets) { b.y += b.vy * swTimeScale; }
  bullets = bullets.filter(b => b.y > -20);

  for (const eb of enemyBullets) { eb.y += eb.vy * swTimeScale; }

  for (const e of enemies) {
    if (e.dead) continue;
    e.x += e.vx * swTimeScale;
    e.y += e.vy * swTimeScale;
    if (e.isMine || e.isOrb) {
      e.rotation += e.rotSpeed * swTimeScale;
    } else {
      e.wobble += 0.05 * swTimeScale;
      e.x += Math.sin(e.wobble) * 0.6;
      e.vy += 0.012 * swTimeScale;

      if (e.shootsBack && now - e.lastShot > 1300 && e.y > 20 && e.y < canvas.height*0.7) {
        enemyBullets.push({x:e.x, y:e.y+e.r, vy: 6.5});
        e.lastShot = now;
      }
    }

    if (!e.isOrb) {
      for (const b of bullets) {
        if (b.hit) continue;
        const d = Math.hypot(b.x - e.x, b.y - e.y);
        if (d < e.r) {
          b.hit = true;
          e.dead = true;
          if (e.isMine) {
            burst(e.x, e.y, '#ffaa55', 18);
            floatText(e.x, e.y-20, 'DEFUSED +15', '#ffcc66', 18);
            score += 15;
            sndDefuse();
          } else {
            burst(e.x, e.y, e.color, 14);
            sndHit();
            let gained = e.scoreValue;
            if (now - lastHitTime < 550) {
              comboCount++;
              if (comboCount >= 2) {
                const bonus = comboCount * 5;
                gained += bonus;
                floatText(e.x, e.y-20, 'COMBO x'+comboCount+'! +'+gained, '#7fffb0', 20);
                sndCombo();
              } else {
                floatText(e.x, e.y-20, '+'+gained, '#ffffff', 16);
              }
            } else {
              comboCount = 1;
              floatText(e.x, e.y-20, '+'+gained, '#ffffff', 16);
            }
            score += gained;
            lastHitTime = now;
          }
          syncHud();
          break;
        }
      }
    }

    if (e.dead) continue;

    const dShip = Math.hypot(ship.x - e.x, ship.y - e.y);
    if (dShip < e.r + SHIP_HIT_RADIUS) {
      if (e.isMine) {
        e.dead = true;
        lives = 0;
        triggerShake(18);
        sndExplosion();
        burst(e.x, e.y, '#ff5533', 26);
        floatText(e.x, e.y-20, 'BOOM!', '#ff5533', 26);
        comboCount = 0;
      } else if (e.isOrb) {
        e.dead = true;
        score += 30;
        swTimeScale = 0.35;
        slowmoUntil = now + 2500;
        overdriveUntil = now + 2500;
        sndPowerUp();
        burst(e.x, e.y, '#5cd6ff', 16);
        floatText(e.x, e.y-20, '+30 OVERDRIVE!', '#5cd6ff', 20);
        comboCount = 0;
      } else {
        e.dead = true;
        lives -= 1;
        triggerShake(10);
        sndEnemyHit();
        burst(e.x, e.y, e.color, 16);
        floatText(e.x, e.y-20, 'HULL HIT', '#ff8866', 18);
        comboCount = 0;
      }
      syncHud();
    }
  }

  for (const e of enemies) {
    if (!e.dead && !e.isOrb && e.y > canvas.height + 60) {
      e.dead = true;
      if (!e.isMine) { lives -= 1; comboCount = 0; syncHud(); }
    }
  }
  enemies = enemies.filter(e => !e.dead && e.y < canvas.height + 200);

  for (const eb of enemyBullets) {
    if (eb.hit) continue;
    const d = Math.hypot(ship.x - eb.x, ship.y - eb.y);
    if (d < SHIP_HIT_RADIUS) {
      eb.hit = true;
      lives -= 1;
      comboCount = 0;
      triggerShake(8);
      sndEnemyHit();
      burst(eb.x, eb.y, '#ffaa55', 10);
      syncHud();
    }
  }
  enemyBullets = enemyBullets.filter(eb => !eb.hit && eb.y < canvas.height + 30);

  tickSharedEffects(swTimeScale);

  if (lives <= 0) swGameOver();
}

function drawShipShape(bank) {
  const r = 22;
  ctx.save();
  ctx.rotate(bank * 0.35);

  const flameLen = 14 + Math.random()*8;
  const fg = ctx.createLinearGradient(0, r*0.6, 0, r*0.6+flameLen);
  fg.addColorStop(0, 'rgba(120,200,255,0.9)');
  fg.addColorStop(1, 'rgba(120,200,255,0)');
  ctx.fillStyle = fg;
  ctx.beginPath();
  ctx.moveTo(-6, r*0.55);
  ctx.lineTo(0, r*0.55+flameLen);
  ctx.lineTo(6, r*0.55);
  ctx.closePath();
  ctx.fill();

  ctx.shadowColor = '#5cd6ff'; ctx.shadowBlur = 12;
  const grad = ctx.createLinearGradient(0, -r, 0, r*0.7);
  grad.addColorStop(0, '#eafcff');
  grad.addColorStop(0.5, '#5cd6ff');
  grad.addColorStop(1, '#0a5c8f');
  ctx.beginPath();
  ctx.moveTo(0, -r);
  ctx.lineTo(r*0.6, r*0.55);
  ctx.lineTo(0, r*0.25);
  ctx.lineTo(-r*0.6, r*0.55);
  ctx.closePath();
  ctx.fillStyle = grad;
  ctx.fill();
  ctx.shadowBlur = 0;
  ctx.lineWidth = 2;
  ctx.strokeStyle = 'rgba(255,255,255,0.5)';
  ctx.stroke();

  ctx.fillStyle = '#1a7fc9';
  ctx.beginPath();
  ctx.moveTo(0, -r*0.1); ctx.lineTo(r*1.05, r*0.35); ctx.lineTo(r*0.55, r*0.55); ctx.closePath();
  ctx.fill();
  ctx.beginPath();
  ctx.moveTo(0, -r*0.1); ctx.lineTo(-r*1.05, r*0.35); ctx.lineTo(-r*0.55, r*0.55); ctx.closePath();
  ctx.fill();

  ctx.fillStyle = '#e0faff';
  ctx.beginPath();
  ctx.ellipse(0, -r*0.25, r*0.18, r*0.32, 0, 0, Math.PI*2);
  ctx.fill();

  ctx.restore();
}

function drawEnemyShape(e) {
  const r = e.r;
  ctx.shadowColor = e.light; ctx.shadowBlur = 12;
  const grad = ctx.createLinearGradient(0, -r, 0, r);
  grad.addColorStop(0, e.light);
  grad.addColorStop(0.55, e.body);
  grad.addColorStop(1, e.dark);
  ctx.fillStyle = grad;
  ctx.lineWidth = 2.2;
  ctx.strokeStyle = 'rgba(0,0,0,0.35)';

  ctx.beginPath();
  if (e.shape === 'fighter') {
    ctx.moveTo(0, -r); ctx.lineTo(r*0.75, r*0.7); ctx.lineTo(0, r*0.3); ctx.lineTo(-r*0.75, r*0.7);
  } else if (e.shape === 'wide') {
    ctx.moveTo(0, -r*0.8); ctx.lineTo(r, r*0.6); ctx.lineTo(0, r*0.15); ctx.lineTo(-r, r*0.6);
  } else if (e.shape === 'diamond') {
    ctx.moveTo(0, -r); ctx.lineTo(r*0.7, 0); ctx.lineTo(0, r); ctx.lineTo(-r*0.7, 0);
  } else if (e.shape === 'hex') {
    for (let i=0;i<6;i++) {
      const ang = Math.PI/6 + i*Math.PI/3;
      const px = Math.cos(ang)*r, py = Math.sin(ang)*r;
      if (i===0) ctx.moveTo(px,py); else ctx.lineTo(px,py);
    }
  } else if (e.shape === 'saucer') {
    ctx.ellipse(0, 0, r, r*0.5, 0, 0, Math.PI*2);
  }
  ctx.closePath();
  ctx.fill();
  ctx.stroke();

  if (e.shape === 'saucer') {
    ctx.fillStyle = 'rgba(255,255,255,0.55)';
    ctx.beginPath();
    ctx.ellipse(0, -r*0.15, r*0.45, r*0.28, 0, 0, Math.PI*2);
    ctx.fill();
  }

  ctx.fillStyle = 'rgba(255,255,255,0.5)';
  ctx.beginPath();
  ctx.arc(0, -r*0.1, r*0.16, 0, Math.PI*2);
  ctx.fill();
  ctx.shadowBlur = 0;
}

function drawMineShape(e) {
  const r = e.r;
  const grad = ctx.createRadialGradient(-r*0.3, -r*0.3, r*0.1, 0, 0, r);
  grad.addColorStop(0, '#6a6a6a');
  grad.addColorStop(0.6, '#2b2b2b');
  grad.addColorStop(1, '#000000');
  ctx.beginPath();
  ctx.arc(0, 0, r, 0, Math.PI*2);
  ctx.fillStyle = grad;
  ctx.fill();
  ctx.lineWidth = 2.5;
  ctx.strokeStyle = '#ff4444';
  ctx.stroke();

  ctx.strokeStyle = '#555';
  ctx.lineWidth = 3;
  for (let i=0;i<8;i++) {
    const ang = (i/8)*Math.PI*2;
    ctx.beginPath();
    ctx.moveTo(Math.cos(ang)*r, Math.sin(ang)*r);
    ctx.lineTo(Math.cos(ang)*r*1.35, Math.sin(ang)*r*1.35);
    ctx.stroke();
  }

  ctx.fillStyle = Math.sin(performance.now()*0.01) > 0 ? '#ff3333' : '#661111';
  ctx.beginPath(); ctx.arc(0, 0, 5, 0, Math.PI*2); ctx.fill();

  ctx.beginPath();
  ctx.ellipse(-r*0.3, -r*0.3, r*0.18, r*0.12, -0.6, 0, Math.PI*2);
  ctx.fillStyle = 'rgba(255,255,255,0.25)';
  ctx.fill();
}

function drawOrbShape(e) {
  const r = e.r;
  ctx.shadowColor = '#5cd6ff';
  ctx.shadowBlur = 26;
  const grad = ctx.createRadialGradient(0,0,0,0,0,r);
  grad.addColorStop(0, '#ffffff');
  grad.addColorStop(0.5, '#5cd6ff');
  grad.addColorStop(1, '#0a5c8f');
  ctx.beginPath();
  ctx.arc(0,0,r*0.65,0,Math.PI*2);
  ctx.fillStyle = grad;
  ctx.fill();
  ctx.shadowBlur = 0;

  ctx.strokeStyle = 'rgba(180,230,255,0.8)';
  ctx.lineWidth = 2.5;
  ctx.beginPath();
  for (let i=0;i<6;i++) {
    const ang = e.rotation + i*Math.PI/3;
    const px = Math.cos(ang)*r, py = Math.sin(ang)*r;
    if (i===0) ctx.moveTo(px,py); else ctx.lineTo(px,py);
  }
  ctx.closePath();
  ctx.stroke();
}

function swDraw() {
  if (trail.length > 1) {
    ctx.strokeStyle = 'rgba(120,200,255,0.7)';
    ctx.shadowColor = '#5cd6ff'; ctx.shadowBlur = 8;
    ctx.lineWidth = 4; ctx.lineCap = 'round';
    ctx.beginPath();
    ctx.moveTo(trail[0].x, trail[0].y);
    for (let i=1;i<trail.length;i++) ctx.lineTo(trail[i].x, trail[i].y);
    ctx.stroke();
    ctx.shadowBlur = 0;
  }

  ctx.strokeStyle = '#ffe066';
  ctx.shadowColor = '#ffe066'; ctx.shadowBlur = 8;
  ctx.lineWidth = 3; ctx.lineCap = 'round';
  for (const b of bullets) {
    if (b.hit) continue;
    ctx.beginPath();
    ctx.moveTo(b.x, b.y);
    ctx.lineTo(b.x, b.y + 14);
    ctx.stroke();
  }
  ctx.shadowBlur = 0;

  ctx.strokeStyle = '#ff5577';
  ctx.shadowColor = '#ff5577'; ctx.shadowBlur = 8;
  for (const eb of enemyBullets) {
    if (eb.hit) continue;
    ctx.beginPath();
    ctx.moveTo(eb.x, eb.y);
    ctx.lineTo(eb.x, eb.y - 12);
    ctx.stroke();
  }
  ctx.shadowBlur = 0;

  for (const e of enemies) {
    if (e.dead) continue;
    ctx.save();
    ctx.beginPath();
    ctx.ellipse(e.x, e.y + e.r*0.9, e.r*0.7, e.r*0.22, 0, 0, Math.PI*2);
    ctx.fillStyle = 'rgba(0,0,0,0.3)';
    ctx.fill();
    ctx.translate(e.x, e.y);
    ctx.rotate(e.rotation || 0);
    if (e.isMine) drawMineShape(e);
    else if (e.isOrb) drawOrbShape(e);
    else drawEnemyShape(e);
    ctx.restore();
  }

  drawSharedEffects();

  if (swRunning) {
    ctx.save();
    ctx.translate(ship.x, ship.y);
    drawShipShape(ship.bank);
    ctx.restore();
  }
}

// ================= CAR RACE =================
let raceRunning = false;
let raceLane = 1;
const RACE_LANES = 3;
let raceObstacles = [];
let raceLastSpawn = 0;
let raceBtn1Prev = false, raceBtn2Prev = false;

function raceLaneX(lane) {
  const roadWidth = Math.min(canvas.width*0.6, 420);
  const roadLeft = (canvas.width-roadWidth)/2;
  const laneWidth = roadWidth/RACE_LANES;
  return roadLeft + laneWidth*(lane+0.5);
}

function raceReset() {
  score = 0; lives = 3; raceLane = 1; raceObstacles = [];
  raceBtn1Prev = !!sensor.btn1; raceBtn2Prev = !!sensor.btn2;
  syncHud();
}
function raceStart() { raceReset(); raceRunning = true; raceLastSpawn = performance.now(); }

function raceGameOver() {
  raceRunning = false;
  sndLose();
  const isRecord = saveHighScore('race', score);
  showGameOverOverlay('race', 'Crashed!',
    'Final Score: <b>' + score + '</b><br>Press LEFT or RIGHT to change lanes and dodge the oncoming cars!', isRecord);
}

function raceSpawn() {
  const lane = Math.floor(Math.random()*RACE_LANES);
  const speedBoost = Math.min(score*0.05, 6);
  raceObstacles.push({lane, y:-40, vy:5+speedBoost, passed:false});
}

function raceUpdate() {
  if (!raceRunning) return;
  tickStars(1);
  const now = performance.now();

  const b1 = !!sensor.btn1, b2 = !!sensor.btn2;
  if (b1 && !raceBtn1Prev) raceLane = Math.max(0, raceLane-1);
  if (b2 && !raceBtn2Prev) raceLane = Math.min(RACE_LANES-1, raceLane+1);
  raceBtn1Prev = b1; raceBtn2Prev = b2;

  const interval = Math.max(480, 1000-score*4);
  if (now-raceLastSpawn>interval) { raceSpawn(); raceLastSpawn = now; }

  const carY = canvas.height - 110;
  for (const o of raceObstacles) {
    o.y += o.vy;
    if (!o.passed && o.y > carY) {
      o.passed = true;
      if (o.lane === raceLane) {
        lives -= 1;
        triggerShake(10);
        sndCrash();
        burst(raceLaneX(o.lane), carY, '#ff5533', 18);
        floatText(raceLaneX(o.lane), carY-20, 'CRASH!', '#ff5533', 20);
      } else {
        score += 10;
        floatText(raceLaneX(raceLane), carY-40, '+10', '#5cffb0', 14);
      }
      syncHud();
    }
  }
  raceObstacles = raceObstacles.filter(o => o.y < canvas.height+60);

  tickSharedEffects(1);
  if (lives <= 0) raceGameOver();
}

function raceDraw() {
  const roadWidth = Math.min(canvas.width*0.6, 420);
  const roadLeft = (canvas.width-roadWidth)/2;
  ctx.fillStyle = '#20242c';
  ctx.fillRect(roadLeft, 0, roadWidth, canvas.height);

  ctx.strokeStyle = 'rgba(255,255,255,0.4)'; ctx.lineWidth = 4; ctx.setLineDash([22,18]);
  for (let i=1;i<RACE_LANES;i++) {
    const x = roadLeft + (roadWidth/RACE_LANES)*i;
    ctx.beginPath(); ctx.moveTo(x,0); ctx.lineTo(x,canvas.height); ctx.stroke();
  }
  ctx.setLineDash([]);
  ctx.strokeStyle = '#ffe066'; ctx.lineWidth = 6;
  ctx.beginPath(); ctx.moveTo(roadLeft,0); ctx.lineTo(roadLeft,canvas.height); ctx.stroke();
  ctx.beginPath(); ctx.moveTo(roadLeft+roadWidth,0); ctx.lineTo(roadLeft+roadWidth,canvas.height); ctx.stroke();

  function roundedRect(x,y,w,h,r) {
    ctx.beginPath();
    if (ctx.roundRect) ctx.roundRect(x,y,w,h,r); else ctx.rect(x,y,w,h);
  }

  for (const o of raceObstacles) {
    const x = raceLaneX(o.lane);
    ctx.fillStyle = '#ff3d4d'; ctx.shadowColor = '#ff3d4d'; ctx.shadowBlur = 10;
    roundedRect(x-22, o.y-30, 44, 60, 8);
    ctx.fill();
    ctx.shadowBlur = 0;
  }

  const carX = raceLaneX(raceLane), carY = canvas.height-110;
  ctx.fillStyle = '#3fc7ff'; ctx.shadowColor = '#3fc7ff'; ctx.shadowBlur = 14;
  roundedRect(carX-22, carY-30, 44, 60, 8);
  ctx.fill();
  ctx.shadowBlur = 0;
  ctx.fillStyle = 'rgba(255,255,255,0.6)';
  ctx.fillRect(carX-14, carY-18, 28, 16);

  drawSharedEffects();
}

// ================= SPIDER WEB =================
let spiderRunning = false;
let spiderAnchor = {x:50, y:50};
let spiderTargetX = 0, spiderTargetY = 0, spiderTargetUntil = 0, spiderTargetLimit = 3000;
let spiderWebAnim = 0;
let spiderWebFrom = {x:0,y:0}, spiderWebTo = {x:0,y:0};

function spiderNewTarget() {
  spiderTargetX = 80 + Math.random()*(canvas.width-160);
  spiderTargetY = 90 + Math.random()*(canvas.height*0.55);
  spiderTargetLimit = Math.max(1200, 3000 - score*10);
  spiderTargetUntil = performance.now() + spiderTargetLimit;
}

function spiderReset() {
  score = 0; lives = 3;
  spiderWebAnim = 0;
  pendingShake = false;
  spiderNewTarget();
  syncHud();
}
function spiderStart() { spiderReset(); spiderRunning = true; }

function spiderGameOver() {
  spiderRunning = false;
  sndLose();
  const isRecord = saveHighScore('spider', score);
  showGameOverOverlay('spider', 'Web Ran Out',
    'Final Score: <b>' + score + '</b><br>Shake the device to make the spider throw a web at the bug before time runs out!', isRecord);
}

function spiderUpdate() {
  if (!spiderRunning) return;
  tickStars(1);
  const now = performance.now();

  if (spiderWebAnim > 0) spiderWebAnim = Math.max(0, spiderWebAnim - 0.06);

  if (pendingShake) {
    pendingShake = false;
    spiderWebFrom = {x:spiderAnchor.x, y:spiderAnchor.y};
    spiderWebTo = {x:spiderTargetX, y:spiderTargetY};
    spiderWebAnim = 1;
    score += 10;
    sndWebThrow();
    burst(spiderTargetX, spiderTargetY, '#ffffff', 14);
    floatText(spiderTargetX, spiderTargetY-20, '+10', '#ffffff', 18);
    syncHud();
    spiderNewTarget();
  } else if (now > spiderTargetUntil) {
    lives -= 1;
    triggerShake(6);
    sndReactionBad();
    syncHud();
    if (lives <= 0) { spiderGameOver(); return; }
    spiderNewTarget();
  }

  tickSharedEffects(1);
}

function spiderDraw() {
  ctx.fillStyle = 'rgba(20,10,30,0.4)'; ctx.fillRect(0,0,canvas.width,canvas.height);

  const now = performance.now();
  const timeLeft = Math.max(0, spiderTargetUntil-now);
  const frac = Math.min(1, timeLeft/spiderTargetLimit);

  ctx.strokeStyle = 'rgba(255,255,255,0.25)'; ctx.lineWidth = 3;
  ctx.beginPath(); ctx.arc(spiderTargetX, spiderTargetY, 26, 0, Math.PI*2); ctx.stroke();
  ctx.strokeStyle = '#ffe066'; ctx.lineWidth = 4;
  ctx.beginPath(); ctx.arc(spiderTargetX, spiderTargetY, 26, -Math.PI/2, -Math.PI/2 + frac*Math.PI*2); ctx.stroke();

  ctx.fillStyle = '#2b2b2b';
  ctx.beginPath(); ctx.ellipse(spiderTargetX, spiderTargetY, 10, 7, 0, 0, Math.PI*2); ctx.fill();
  ctx.strokeStyle = 'rgba(255,255,255,0.5)'; ctx.lineWidth = 2;
  ctx.beginPath(); ctx.moveTo(spiderTargetX-14, spiderTargetY-10); ctx.lineTo(spiderTargetX+2, spiderTargetY-2); ctx.stroke();
  ctx.beginPath(); ctx.moveTo(spiderTargetX+14, spiderTargetY-10); ctx.lineTo(spiderTargetX-2, spiderTargetY-2); ctx.stroke();

  ctx.strokeStyle = 'rgba(255,255,255,0.15)'; ctx.lineWidth = 1;
  ctx.beginPath(); ctx.moveTo(0,0); ctx.lineTo(spiderAnchor.x, spiderAnchor.y); ctx.stroke();

  ctx.fillStyle = '#111';
  ctx.beginPath(); ctx.arc(spiderAnchor.x, spiderAnchor.y, 14, 0, Math.PI*2); ctx.fill();
  ctx.strokeStyle = '#111'; ctx.lineWidth = 2;
  for (let i=0;i<6;i++) {
    const ang = i*Math.PI/3;
    ctx.beginPath();
    ctx.moveTo(spiderAnchor.x, spiderAnchor.y);
    ctx.lineTo(spiderAnchor.x+Math.cos(ang)*22, spiderAnchor.y+Math.sin(ang)*22);
    ctx.stroke();
  }

  if (spiderWebAnim > 0) {
    const t = 1-spiderWebAnim;
    const wx = spiderWebFrom.x + (spiderWebTo.x-spiderWebFrom.x)*t;
    const wy = spiderWebFrom.y + (spiderWebTo.y-spiderWebFrom.y)*t;
    ctx.strokeStyle = '#fff'; ctx.lineWidth = 3; ctx.shadowColor = '#fff'; ctx.shadowBlur = 8;
    ctx.beginPath(); ctx.moveTo(spiderWebFrom.x, spiderWebFrom.y); ctx.lineTo(wx, wy); ctx.stroke();
    ctx.shadowBlur = 0;
    ctx.fillStyle = '#fff';
    ctx.beginPath(); ctx.arc(wx, wy, 6, 0, Math.PI*2); ctx.fill();
  }

  drawSharedEffects();
}

// ================= TILT MAZE =================
let mazeRunning = false;
let mazeLevel = 0;
let ball = {x:0, y:0, vx:0, vy:0, r:14};
let mazeWalls = [], mazeGoal = {x:0,y:0,r:18}, mazeHazards = [];

const MAZE_LAYOUTS = [
  { start:{x:0.5,y:0.92}, goal:{x:0.5,y:0.08},
    walls:[
      {x:0.10,y:0.30,w:0.55,h:0.05},
      {x:0.35,y:0.55,w:0.55,h:0.05},
      {x:0.10,y:0.78,w:0.55,h:0.05}
    ],
    hazards:[{x:0.80,y:0.42},{x:0.20,y:0.65}]
  },
  { start:{x:0.15,y:0.92}, goal:{x:0.85,y:0.08},
    walls:[
      {x:0.0, y:0.68,w:0.65,h:0.05},
      {x:0.35,y:0.45,w:0.65,h:0.05},
      {x:0.0, y:0.22,w:0.65,h:0.05}
    ],
    hazards:[{x:0.85,y:0.68},{x:0.15,y:0.45},{x:0.85,y:0.30}]
  },
  { start:{x:0.5,y:0.92}, goal:{x:0.5,y:0.08},
    walls:[
      {x:0.05,y:0.72,w:0.35,h:0.05},{x:0.60,y:0.72,w:0.35,h:0.05},
      {x:0.05,y:0.50,w:0.35,h:0.05},{x:0.60,y:0.50,w:0.35,h:0.05},
      {x:0.05,y:0.28,w:0.35,h:0.05},{x:0.60,y:0.28,w:0.35,h:0.05}
    ],
    hazards:[{x:0.5,y:0.61},{x:0.5,y:0.39}]
  }
];
const MAZE_HAZARD_R = 20;

function buildMaze() {
  const layout = MAZE_LAYOUTS[mazeLevel % MAZE_LAYOUTS.length];
  const bw = 14;
  mazeWalls = layout.walls.map(w => ({
    x: w.x*canvas.width, y: w.y*canvas.height, w: w.w*canvas.width, h: bw
  }));
  mazeWalls.push({x:0,y:0,w:canvas.width,h:bw});
  mazeWalls.push({x:0,y:canvas.height-bw,w:canvas.width,h:bw});
  mazeWalls.push({x:0,y:0,w:bw,h:canvas.height});
  mazeWalls.push({x:canvas.width-bw,y:0,w:bw,h:canvas.height});

  mazeGoal = {x:layout.goal.x*canvas.width, y:layout.goal.y*canvas.height, r:22};
  mazeHazards = layout.hazards.map(h => ({x:h.x*canvas.width, y:h.y*canvas.height, r:MAZE_HAZARD_R}));
  ball.x = layout.start.x*canvas.width; ball.y = layout.start.y*canvas.height;
  ball.vx = 0; ball.vy = 0;
}

function mazeReset() {
  score = 0; lives = 3; mazeLevel = 0;
  buildMaze();
  syncHud();
}
function mazeStart() { mazeReset(); mazeRunning = true; }

function mazeGameOver() {
  mazeRunning = false;
  sndLose();
  const isRecord = saveHighScore('maze', score);
  showGameOverOverlay('maze', 'Out of Tries',
    'Final Score: <b>' + score + '</b><br>Tilt to roll the ball to the goal and avoid the red hazards!', isRecord);
}

function circleRectOverlap(c, rect) {
  const closestX = Math.max(rect.x, Math.min(c.x, rect.x+rect.w));
  const closestY = Math.max(rect.y, Math.min(c.y, rect.y+rect.h));
  const dx = c.x-closestX, dy = c.y-closestY;
  return Math.hypot(dx,dy) < c.r;
}

function mazeUpdate() {
  if (!mazeRunning) return;
  tickStars(1);

  smooth.ax += (sensor.ax - smooth.ax) * SMOOTHING;
  smooth.ay += (sensor.ay - smooth.ay) * SMOOTHING;
  let tx = SWAP_AXES ? (smooth.ay-offset.ay) : (smooth.ax-offset.ax);
  let ty = SWAP_AXES ? (smooth.ax-offset.ax) : (smooth.ay-offset.ay);
  if (INVERT_X) tx = -tx;
  if (INVERT_Y) ty = -ty;

  ball.vx = ball.vx*DAMPING + tx*SENSITIVITY;
  ball.vx = Math.max(-MAX_SPEED, Math.min(MAX_SPEED, ball.vx));
  ball.vy = ball.vy*DAMPING + ty*SENSITIVITY;
  ball.vy = Math.max(-MAX_SPEED, Math.min(MAX_SPEED, ball.vy));

  ball.x += ball.vx;
  for (const w of mazeWalls) { if (circleRectOverlap(ball, w)) { ball.x -= ball.vx; ball.vx = 0; break; } }
  ball.y += ball.vy;
  for (const w of mazeWalls) { if (circleRectOverlap(ball, w)) { ball.y -= ball.vy; ball.vy = 0; break; } }

  for (const h of mazeHazards) {
    if (Math.hypot(ball.x-h.x, ball.y-h.y) < ball.r+h.r) {
      lives -= 1;
      triggerShake(10); sndMazeHazard();
      burst(ball.x, ball.y, '#ff5533', 16);
      floatText(ball.x, ball.y-20, 'OUCH!', '#ff5533', 20);
      syncHud();
      const layout = MAZE_LAYOUTS[mazeLevel % MAZE_LAYOUTS.length];
      ball.x = layout.start.x*canvas.width; ball.y = layout.start.y*canvas.height;
      ball.vx = 0; ball.vy = 0;
      break;
    }
  }

  if (Math.hypot(ball.x-mazeGoal.x, ball.y-mazeGoal.y) < ball.r+mazeGoal.r) {
    score += 100;
    sndMazeGoal();
    burst(mazeGoal.x, mazeGoal.y, '#5cffb0', 22);
    floatText(mazeGoal.x, mazeGoal.y-20, 'GOAL! +100', '#5cffb0', 22);
    mazeLevel++;
    buildMaze();
    syncHud();
  }

  tickSharedEffects(1);
  if (lives <= 0) mazeGameOver();
}

function mazeDraw() {
  ctx.fillStyle = 'rgba(120,60,255,0.10)'; ctx.fillRect(0,0,canvas.width,canvas.height);

  ctx.fillStyle = '#3fc7ff';
  ctx.shadowColor = '#3fc7ff'; ctx.shadowBlur = 8;
  for (const w of mazeWalls) ctx.fillRect(w.x, w.y, w.w, w.h);
  ctx.shadowBlur = 0;

  const pulse = 0.7+0.3*Math.sin(performance.now()*0.005);
  ctx.fillStyle = '#5cffb0'; ctx.shadowColor='#5cffb0'; ctx.shadowBlur = 20*pulse;
  ctx.beginPath(); ctx.arc(mazeGoal.x, mazeGoal.y, mazeGoal.r*pulse, 0, Math.PI*2); ctx.fill();
  ctx.shadowBlur = 0;

  for (const h of mazeHazards) {
    ctx.fillStyle = '#ff3d4d'; ctx.shadowColor='#ff3d4d'; ctx.shadowBlur = 14*pulse;
    ctx.beginPath(); ctx.arc(h.x, h.y, h.r*(0.85+0.15*pulse), 0, Math.PI*2); ctx.fill();
  }
  ctx.shadowBlur = 0;

  const grad = ctx.createRadialGradient(ball.x-5,ball.y-5,2,ball.x,ball.y,ball.r);
  grad.addColorStop(0,'#ffffff'); grad.addColorStop(1,'#0a5c8f');
  ctx.fillStyle = grad;
  ctx.beginPath(); ctx.arc(ball.x, ball.y, ball.r, 0, Math.PI*2); ctx.fill();

  drawSharedEffects();
}

// ================= REACTION RUSH =================
let reactionRunning = false;
let reactionPhase = 'idle';
let reactionPhaseUntil = 0;
let reactionSide = null;
let reactionWindow = 1100;
const reactionWaitRange = [500,1400];
let reactionFeedback = '';
let reactionFeedbackColor = '#fff';
let reactionBtn1Prev = false, reactionBtn2Prev = false;
let reactionStreak = 0;

function reactionReset() {
  score = 0; lives = 3; reactionWindow = 1100; reactionStreak = 0;
  reactionBtn1Prev = !!sensor.btn1; reactionBtn2Prev = !!sensor.btn2;
  syncHud();
}
function reactionStart() {
  reactionReset();
  reactionRunning = true;
  reactionPhase = 'ready';
  reactionPhaseUntil = performance.now() + 900;
  reactionFeedback = 'Get Ready...';
  reactionFeedbackColor = '#cfe8f5';
}

function reactionGameOver() {
  reactionRunning = false;
  sndLose();
  const isRecord = saveHighScore('reaction', score);
  showGameOverOverlay('reaction', 'Out of Lives',
    'Final Score: <b>' + score + '</b><br>Watch the arrow and press the matching button as fast as you can!', isRecord);
}

function reactionNextPrompt() {
  reactionPhase = 'waiting';
  const delay = reactionWaitRange[0] + Math.random()*(reactionWaitRange[1]-reactionWaitRange[0]);
  reactionPhaseUntil = performance.now() + delay;
}

function reactionMiss(msg) {
  lives -= 1;
  reactionStreak = 0;
  sndReactionBad();
  triggerShake(8);
  reactionFeedback = msg;
  reactionFeedbackColor = '#ff5577';
  syncHud();
  if (lives <= 0) { reactionGameOver(); return; }
  reactionPhase = 'feedback';
  reactionPhaseUntil = performance.now() + 500;
}

function reactionSuccess() {
  reactionStreak++;
  const gained = 10 + Math.min(reactionStreak,10)*2;
  score += gained;
  sndReactionGood();
  floatText(canvas.width/2, canvas.height/2-60, '+'+gained, '#5cffb0', 22);
  reactionFeedback = 'Nice! +'+gained;
  reactionFeedbackColor = '#5cffb0';
  reactionWindow = Math.max(450, reactionWindow - 18);
  syncHud();
  reactionPhase = 'feedback';
  reactionPhaseUntil = performance.now() + 350;
}

function reactionUpdate() {
  if (!reactionRunning) return;
  const now = performance.now();
  const b1 = !!sensor.btn1, b2 = !!sensor.btn2;
  const b1Pressed = b1 && !reactionBtn1Prev;
  const b2Pressed = b2 && !reactionBtn2Prev;
  reactionBtn1Prev = b1; reactionBtn2Prev = b2;

  if (reactionPhase === 'ready') {
    if (now >= reactionPhaseUntil) reactionNextPrompt();
    return;
  }
  if (reactionPhase === 'waiting') {
    if (b1Pressed || b2Pressed) { reactionMiss('Too soon!'); return; }
    if (now >= reactionPhaseUntil) {
      reactionPhase = 'prompt';
      reactionSide = Math.random() < 0.5 ? 'L' : 'R';
      reactionPhaseUntil = now + reactionWindow;
      sndReactionGo();
    }
    return;
  }
  if (reactionPhase === 'prompt') {
    if (b1Pressed || b2Pressed) {
      const pressedSide = b1Pressed ? 'L' : 'R';
      if (pressedSide === reactionSide) reactionSuccess();
      else reactionMiss('Wrong button!');
      return;
    }
    if (now >= reactionPhaseUntil) { reactionMiss('Too slow!'); return; }
    return;
  }
  if (reactionPhase === 'feedback') {
    if (now >= reactionPhaseUntil) reactionNextPrompt();
  }
}

function reactionDraw() {
  ctx.fillStyle = 'rgba(20,20,30,0.55)'; ctx.fillRect(0,0,canvas.width,canvas.height);

  const cx = canvas.width/2, cy = canvas.height/2;

  if (reactionPhase === 'prompt') {
    const dir = reactionSide === 'L' ? -1 : 1;
    ctx.fillStyle = '#ffe066'; ctx.shadowColor = '#ffe066'; ctx.shadowBlur = 30;
    ctx.beginPath();
    ctx.moveTo(cx+dir*90, cy-70);
    ctx.lineTo(cx+dir*90, cy+70);
    ctx.lineTo(cx+dir*180, cy);
    ctx.closePath();
    ctx.fill();
    ctx.shadowBlur = 0;
    ctx.fillStyle = '#fff'; ctx.font = 'bold 22px sans-serif'; ctx.textAlign = 'center';
    ctx.fillText(reactionSide === 'L' ? 'PRESS LEFT (D33)' : 'PRESS RIGHT (D32)', cx, cy+120);
  } else if (reactionPhase === 'waiting') {
    ctx.fillStyle = '#5cd6ff'; ctx.beginPath(); ctx.arc(cx,cy,18,0,Math.PI*2); ctx.fill();
    ctx.fillStyle = '#cfe8f5'; ctx.font='16px sans-serif'; ctx.textAlign = 'center';
    ctx.fillText('Wait for it...', cx, cy+50);
  } else {
    ctx.fillStyle = reactionFeedbackColor; ctx.font = 'bold 28px sans-serif'; ctx.textAlign = 'center';
    ctx.fillText(reactionFeedback, cx, cy);
  }

  ctx.fillStyle = '#cfe8f5'; ctx.font = '14px sans-serif';
  ctx.fillText('Streak: '+reactionStreak, cx, cy+150);
  ctx.textAlign = 'left';

  drawSharedEffects();
}

// ================= JETPACK AVIATOR =================
let jetRunning = false;
let jet = {y:0, vy:0};
const JET_GRAVITY = 0.5;
const JET_THRUST = -0.9;
const JET_MAX_VY = 9;
let jetObstacles = [];
let jetLastSpawn = 0;

function jetReset() {
  score = 0; lives = 3;
  jet.y = canvas.height/2; jet.vy = 0;
  jetObstacles = [];
  syncHud();
}
function jetStart() { jetReset(); jetRunning = true; jetLastSpawn = performance.now(); }

function jetGameOver() {
  jetRunning = false;
  sndLose();
  const isRecord = saveHighScore('jetpack', score);
  showGameOverOverlay('jetpack', 'Crashed!',
    'Final Score: <b>' + score + '</b><br>Hold either button to thrust up, release to fall — dodge the barriers!', isRecord);
}

function jetSpawnObstacle() {
  const gapH = Math.max(130, 230 - score*1.5);
  const gapY = 60 + Math.random()*(canvas.height-120-gapH);
  jetObstacles.push({x: canvas.width+40, gapY, gapH, passed:false, hit:false});
}

function jetUpdate() {
  if (!jetRunning) return;
  tickStars(1);
  const now = performance.now();
  const thrusting = !!sensor.btn1 || !!sensor.btn2;

  jet.vy += thrusting ? JET_THRUST : JET_GRAVITY;
  jet.vy = Math.max(-JET_MAX_VY, Math.min(JET_MAX_VY, jet.vy));
  jet.y += jet.vy;

  const jetX = canvas.width*0.28;
  const jetR = 16;
  if (jet.y - jetR < 0) { jet.y = jetR; jet.vy = 0; }
  if (jet.y + jetR > canvas.height) { jet.y = canvas.height-jetR; jet.vy = 0; }

  const speed = 3.4 + Math.min(score*0.03, 4);
  const spawnInterval = Math.max(900, 1500-score*6);
  if (now-jetLastSpawn>spawnInterval) { jetSpawnObstacle(); jetLastSpawn = now; }

  for (const o of jetObstacles) {
    o.x -= speed;
    if (!o.passed && o.x+22 < jetX-jetR) {
      o.passed = true;
      score += 5;
      syncHud();
    }
    if (!o.hit && Math.abs(o.x-jetX) < 22+jetR &&
        (jet.y-jetR < o.gapY || jet.y+jetR > o.gapY+o.gapH)) {
      o.hit = true;
    }
  }
  for (const o of jetObstacles) {
    if (o.hit) {
      lives -= 1;
      triggerShake(10); sndCrash();
      burst(jetX, jet.y, '#ff5533', 16);
      floatText(jetX, jet.y-20, 'CRASH!', '#ff5533', 20);
      syncHud();
      jetObstacles = jetObstacles.filter(x => x !== o);
      jet.y = canvas.height/2; jet.vy = 0;
      break;
    }
  }
  jetObstacles = jetObstacles.filter(o => o.x > -60);

  tickSharedEffects(1);
  if (lives <= 0) jetGameOver();
}

function jetDraw() {
  ctx.fillStyle = 'rgba(255,140,0,0.08)'; ctx.fillRect(0,0,canvas.width,canvas.height);

  for (const o of jetObstacles) {
    ctx.fillStyle = '#8a5a2b'; ctx.shadowColor = '#8a5a2b'; ctx.shadowBlur = 6;
    ctx.fillRect(o.x-22, 0, 44, o.gapY);
    ctx.fillRect(o.x-22, o.gapY+o.gapH, 44, canvas.height-(o.gapY+o.gapH));
    ctx.shadowBlur = 0;
  }

  const jetX = canvas.width*0.28;
  const thrusting = !!sensor.btn1 || !!sensor.btn2;
  if (thrusting) {
    ctx.fillStyle = 'rgba(255,180,60,0.85)';
    ctx.beginPath();
    ctx.moveTo(jetX-8, jet.y+14);
    ctx.lineTo(jetX, jet.y+28+Math.random()*8);
    ctx.lineTo(jetX+8, jet.y+14);
    ctx.closePath();
    ctx.fill();
  }
  const grad = ctx.createRadialGradient(jetX-5, jet.y-5, 2, jetX, jet.y, 16);
  grad.addColorStop(0, '#eafcff');
  grad.addColorStop(1, '#0a5c8f');
  ctx.fillStyle = grad; ctx.shadowColor = '#5cd6ff'; ctx.shadowBlur = 10;
  ctx.beginPath(); ctx.arc(jetX, jet.y, 16, 0, Math.PI*2); ctx.fill();
  ctx.shadowBlur = 0;

  drawSharedEffects();
}

// ================= BRICK BREAKER =================
let brickRunning = false;
let brickPaddle = {x:0, vx:0, w:110, h:14};
let brickBall = {x:0, y:0, vx:0, vy:0, r:9};
let bricks = [];
let brickLevel = 0;
const BRICK_ROWS = 5, BRICK_COLS = 8;
const BRICK_COLORS = ['#ff3d4d','#ff9f1c','#ffe066','#3fae6a','#3fc7ff'];

function buildBricks() {
  bricks = [];
  const top = 70, gap = 6;
  const areaW = Math.min(canvas.width*0.86, 520);
  const left = (canvas.width-areaW)/2;
  const bw = (areaW-gap*(BRICK_COLS-1))/BRICK_COLS;
  const bh = 20;
  for (let r=0;r<BRICK_ROWS;r++) {
    for (let c=0;c<BRICK_COLS;c++) {
      bricks.push({
        x: left + c*(bw+gap), y: top + r*(bh+gap), w:bw, h:bh,
        color: BRICK_COLORS[r % BRICK_COLORS.length], alive:true, score: (BRICK_ROWS-r)*5
      });
    }
  }
}

function brickLaunchBall() {
  brickBall.x = brickPaddle.x;
  brickBall.y = canvas.height-90;
  const speed = 6 + brickLevel*0.6;
  const ang = -Math.PI/2 + (Math.random()-0.5)*0.6;
  brickBall.vx = Math.cos(ang)*speed;
  brickBall.vy = Math.sin(ang)*speed;
}

function brickReset() {
  score = 0; lives = 3; brickLevel = 0;
  brickPaddle.x = canvas.width/2; brickPaddle.vx = 0;
  buildBricks();
  brickLaunchBall();
  syncHud();
}
function brickStart() { brickReset(); brickRunning = true; }

function brickGameOver() {
  brickRunning = false;
  sndLose();
  const isRecord = saveHighScore('brick', score);
  showGameOverOverlay('brick', 'Ball Lost',
    'Final Score: <b>' + score + '</b><br>Tilt to slide the paddle, keep the ball up, and smash every brick!', isRecord);
}

function brickUpdate() {
  if (!brickRunning) return;
  tickStars(1);

  const rawX = tiltX();
  brickPaddle.vx = brickPaddle.vx*DAMPING + rawX*SENSITIVITY;
  brickPaddle.vx = Math.max(-MAX_SPEED, Math.min(MAX_SPEED, brickPaddle.vx));
  brickPaddle.x += brickPaddle.vx;
  brickPaddle.x = Math.max(brickPaddle.w/2, Math.min(canvas.width-brickPaddle.w/2, brickPaddle.x));

  brickBall.x += brickBall.vx;
  brickBall.y += brickBall.vy;

  if (brickBall.x - brickBall.r < 0) { brickBall.x = brickBall.r; brickBall.vx *= -1; }
  if (brickBall.x + brickBall.r > canvas.width) { brickBall.x = canvas.width-brickBall.r; brickBall.vx *= -1; }
  if (brickBall.y - brickBall.r < 0) { brickBall.y = brickBall.r; brickBall.vy *= -1; }

  const paddleY = canvas.height-80;
  if (brickBall.vy > 0 &&
      brickBall.y+brickBall.r >= paddleY && brickBall.y+brickBall.r <= paddleY+brickPaddle.h+8 &&
      brickBall.x > brickPaddle.x-brickPaddle.w/2 && brickBall.x < brickPaddle.x+brickPaddle.w/2) {
    const hitPos = (brickBall.x - brickPaddle.x) / (brickPaddle.w/2);
    const speed = Math.hypot(brickBall.vx, brickBall.vy);
    const ang = -Math.PI/2 + hitPos*1.0;
    brickBall.vx = Math.cos(ang)*speed;
    brickBall.vy = Math.sin(ang)*speed;
    brickBall.y = paddleY - brickBall.r;
    sndFire();
  }

  for (const b of bricks) {
    if (!b.alive) continue;
    if (brickBall.x+brickBall.r > b.x && brickBall.x-brickBall.r < b.x+b.w &&
        brickBall.y+brickBall.r > b.y && brickBall.y-brickBall.r < b.y+b.h) {
      b.alive = false;
      score += b.score;
      sndHit();
      burst(b.x+b.w/2, b.y+b.h/2, b.color, 10);
      floatText(b.x+b.w/2, b.y-4, '+'+b.score, '#ffffff', 14);
      syncHud();
      const overlapX = Math.min(brickBall.x+brickBall.r-b.x, b.x+b.w-(brickBall.x-brickBall.r));
      const overlapY = Math.min(brickBall.y+brickBall.r-b.y, b.y+b.h-(brickBall.y-brickBall.r));
      if (overlapX < overlapY) brickBall.vx *= -1; else brickBall.vy *= -1;
      break;
    }
  }

  if (bricks.every(b => !b.alive)) {
    brickLevel++;
    score += 50;
    buildBricks();
    brickLaunchBall();
    syncHud();
  }

  if (brickBall.y - brickBall.r > canvas.height) {
    lives -= 1;
    triggerShake(8);
    sndEnemyHit();
    syncHud();
    if (lives <= 0) { brickGameOver(); return; }
    brickLaunchBall();
  }

  tickSharedEffects(1);
}

function brickDraw() {
  ctx.fillStyle = 'rgba(255,255,255,0.03)'; ctx.fillRect(0,0,canvas.width,canvas.height);

  for (const b of bricks) {
    if (!b.alive) continue;
    ctx.fillStyle = b.color; ctx.shadowColor = b.color; ctx.shadowBlur = 6;
    ctx.fillRect(b.x, b.y, b.w, b.h);
    ctx.shadowBlur = 0;
    ctx.strokeStyle = 'rgba(0,0,0,0.3)'; ctx.lineWidth = 1.5;
    ctx.strokeRect(b.x, b.y, b.w, b.h);
  }

  const paddleY = canvas.height-80;
  ctx.fillStyle = '#5cd6ff'; ctx.shadowColor = '#5cd6ff'; ctx.shadowBlur = 10;
  ctx.beginPath();
  if (ctx.roundRect) ctx.roundRect(brickPaddle.x-brickPaddle.w/2, paddleY, brickPaddle.w, brickPaddle.h, 6);
  else ctx.rect(brickPaddle.x-brickPaddle.w/2, paddleY, brickPaddle.w, brickPaddle.h);
  ctx.fill();
  ctx.shadowBlur = 0;

  const grad = ctx.createRadialGradient(brickBall.x-3, brickBall.y-3, 1, brickBall.x, brickBall.y, brickBall.r);
  grad.addColorStop(0, '#ffffff'); grad.addColorStop(1, '#ffcc66');
  ctx.fillStyle = grad;
  ctx.beginPath(); ctx.arc(brickBall.x, brickBall.y, brickBall.r, 0, Math.PI*2); ctx.fill();

  drawSharedEffects();
}

// ---------------- Generic dispatch loop ----------------
function update() {
  if (activeGame !== 'spacewar') tickStars(1);
  if (activeGame !== 'spider') pendingShake = false;
  if (activeGame === 'spacewar') swUpdate();
  else if (activeGame === 'maze') mazeUpdate();
  else if (activeGame === 'reaction') reactionUpdate();
  else if (activeGame === 'race') raceUpdate();
  else if (activeGame === 'spider') spiderUpdate();
  else if (activeGame === 'jetpack') jetUpdate();
  else if (activeGame === 'brick') brickUpdate();
}
function draw() {
  ctx.save();
  if (shake.t > 0) ctx.translate((Math.random()-0.5)*shake.t, (Math.random()-0.5)*shake.t);
  drawSpaceBackground();
  if (activeGame === 'spacewar') swDraw();
  else if (activeGame === 'maze') mazeDraw();
  else if (activeGame === 'reaction') reactionDraw();
  else if (activeGame === 'race') raceDraw();
  else if (activeGame === 'spider') spiderDraw();
  else if (activeGame === 'jetpack') jetDraw();
  else if (activeGame === 'brick') brickDraw();
  ctx.restore();
}
function loop() { update(); draw(); requestAnimationFrame(loop); }

// ---------------- Boot ----------------
initStars();
showHub();
pollSensor();
loop();
</script>
</body>
</html>
)HTMLPAGE";
