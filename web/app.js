import {ensurePlay,restoreSnapshot,snapshot} from './play.js';
import {DeviceModel} from './model.js';import {drawDevice} from './render.js';
const read=async path=>{let r=await fetch(path);if(!r.ok)throw Error('资源缺失: '+path);return r.json();};const canvas=document.querySelector('#device'),hint=document.querySelector('#hint');
const [catalog,ui,backgrounds,care]=await Promise.all([read('data/catalog.json'),read('data/device-ui.json'),read('data/backgrounds.json'),read('data/care-assets.json')]);ui.backgrounds=backgrounds;ui.care=care;const model=new DeviceModel(catalog,read);try{Object.assign(model.settings,JSON.parse(localStorage.getItem('digipocket-settings-v2')||'{}'));}catch{}
const saveKey='digipocket-save-v1';
let lastSave=null;
try{lastSave=localStorage.getItem(saveKey);}catch{}
if(lastSave){
 const saved=JSON.parse(lastSave);
 if(saved.version!==1||!catalog.some(entry=>entry.path===saved.pet?.path))
  throw Error('存档角色不在当前素材包中，请恢复对应素材后刷新。');
 await ensurePlay(model);
 Object.assign(model.progress,saved.progress);
 await restoreSnapshot(model,saved.pet);
}else await model.beginEgg('database/Digitama/Agu_Digitama.json');

function saveGame(){
 if(model.busy||(model.pendingEffect&&model.pendingEffect.type!=='hatch'))return;
 const next=JSON.stringify({version:1,pet:snapshot(model),progress:model.progress});
 if(next===lastSave)return;
 try{localStorage.setItem(saveKey,next);lastSave=next;}catch{}
}
window.addEventListener('pagehide',saveGame);
let audio,lastInput=Date.now(),lastMinute=Date.now(),sleepingScreen=false;
function beep(){if(!model.settings.sound)return;try{audio ||=new (window.AudioContext||window.webkitAudioContext)();let o=audio.createOscillator(),g=audio.createGain();o.type='square';o.frequency.value=1150;g.gain.value=[0,.015,.03,.05][model.settings.sound];o.connect(g);g.connect(audio.destination);o.start();o.stop(audio.currentTime+.035);}catch{}}
const playButton=document.querySelector('#open-play'),chartLink=document.querySelector('#evolution-chart');
let inputQueue=Promise.resolve();function press(k,hold=false){inputQueue=inputQueue.then(async()=>{lastInput=Date.now();if(sleepingScreen){sleepingScreen=false;model.screenOff=false;model.powerSleeping=false;lastMinute=Date.now();paint();return;}beep();await model.press(k,hold);localStorage.setItem('digipocket-settings-v2',JSON.stringify(model.settings));paint();});return inputQueue;}
for(const k of ['a','b','c']){let button=document.querySelector('#key-'+k),timer,held=false;button.onpointerdown=e=>{e.preventDefault();held=false;timer=setTimeout(()=>{held=true;press(k.toUpperCase(),true);},550);};button.onpointerup=e=>{clearTimeout(timer);if(!held)press(k.toUpperCase());};button.onpointercancel=()=>clearTimeout(timer);button.onpointerleave=()=>clearTimeout(timer);button.onclick=e=>{if(e.detail===0)press(k.toUpperCase());};}
document.addEventListener('keydown',e=>{if(e.target.closest('summary,a,#open-play'))return;let k={a:'A',b:'B',c:'C',ArrowLeft:'A',Enter:'B',Escape:'C'}[e.key.toLowerCase()]||{ArrowLeft:'A',Enter:'B',Escape:'C'}[e.key];if(k){e.preventDefault();press(k,e.repeat);}});
playButton.onclick=()=>press('A',true);
function paint(){model.settle();saveGame();let timeout=[0,30000,60000,300000][model.settings.timeout];sleepingScreen=model.screenOff||model.powerSleeping||!!(timeout&&!(['battle','battleIntro'].includes(model.pendingEffect?.type)||model.screen==='battleTiming')&&Date.now()-lastInput>timeout);playButton.disabled=sleepingScreen||model.busy||model.screen!=='home';
chartLink.hash=new URLSearchParams({pet:model.path}).toString();
if(sleepingScreen){let c=canvas.getContext('2d');c.fillStyle='#151b15';c.fillRect(0,0,320,240);hint.textContent=model.powerSleeping?'模拟整机休眠 · 点击屏幕唤醒':'屏幕已熄灭 · 点击屏幕或按钮唤醒';return;}drawDevice(canvas,model,ui);hint.textContent=model.hint();document.body.dataset.screen=model.screen;}
canvas.addEventListener('pointerdown',()=>{if(sleepingScreen)press('C');});
setInterval(()=>{if(model.pet.stats.stage==='Digitama')lastMinute=Date.now();if(model.powerSleeping){lastMinute=Date.now();paint();return;}let elapsed=Date.now()-lastMinute,n=Math.floor(elapsed*model.settings.speed/60000);if(n){lastMinute=Date.now();model.tick(Math.min(n,60));}paint();},60);paint();
