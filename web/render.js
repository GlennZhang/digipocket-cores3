import {drawBattle} from './battle-render.js';
import {playView} from './play.js';
import {trainingPosition,trainingWindow} from './training-rules.js';
import {Wandering} from './walking.js';
import {CARE_TIMING,foodStage,actionStep} from './care-animation.js';
const walkers=new WeakMap();
import {routes,progress} from './model.js';
export function drawDevice(canvas,m,ui,now=Date.now()){
 const ctx=canvas.getContext('2d');ctx.imageSmoothingEnabled=false;const palettes=[['#b5c29a','#253125','#9eac86'],['#c2c5b9','#30352e','#aaafa0'],['#d6bd83','#483824','#c1a96e']];let [bg,ink,ghost]=palettes[m.settings.theme],room=Array(512).fill(null);if(m.settings.background>=0){ink='#223626';ghost='#58734c';}const mono=m.settings.color===0,frame=Math.floor(now/CARE_TIMING.walk);
 function dot(x,y,c=ink){if(x>=0&&x<32&&y>=0&&y<16)room[y*32+x]=c;}
 function text(str,x,y,width=32){str=String(str).toUpperCase();let len=str.length*4-1,offset=len>width?Math.max(0,(Math.floor(now/240)%(len+width))-width):0;for(let i=0;i<str.length;i++){let a=ui.font[str[i]]||ui.font[' '];if(str[i]===':')a=[0,0,0,0,1,0,0,0,0,0,1,0,0,0,0];for(let yy=0;yy<5;yy++)for(let xx=0;xx<3;xx++){let px=x+i*4+xx-offset;if(px>=x&&px<x+width&&a[yy*3+xx])dot(px,y+yy);}}}
 function center(str,y){let n=String(str).length*4-1;text(str,Math.max(0,Math.floor((32-n)/2)),y,32);}
 function heart(x,y,filled){let h=[0,1,0,1,0,1,1,1,1,1,1,1,1,1,1,0,1,1,1,0,0,0,1,0,0];for(let yy=0;yy<5;yy++)for(let xx=0;xx<5;xx++)if(h[yy*5+xx]&&(filled||yy<2||xx===0||xx===4||yy===4))dot(x+xx,y+yy,filled?ink:ghost);}
 function sprite(data,x,y,kind='Walk',flip=false,index=frame){if(!data)return;let labels=Object.keys(data.animations),options=labels.filter(k=>k.startsWith(kind));if(!options.length)options=labels.filter(k=>k.startsWith('Walk'));let pixels=data.animations[options[index%options.length]||labels[0]];for(let yy=0;yy<data.height;yy++)for(let xx=0;xx<data.width;xx++){let v=pixels[yy*data.width+xx];if(!v||v==='0')continue;let color=typeof v==='string'?v:v===1?'#1a1a1a':`#${(v&0xffffff).toString(16).padStart(6,'0')}`;if(mono){let n=parseInt(color.slice(1,7),16),l=.299*(n>>16)+.587*((n>>8)&255)+.114*(n&255);if(l>150)continue;color=ink;}dot(x+(flip?data.width-1-xx:xx),y+yy,color);}}
 function bitmap(pixels,w,h,x,y,flip=false){if(!pixels)return;for(let yy=0;yy<h;yy++)for(let xx=0;xx<w;xx++)if(pixels[yy*w+(flip?w-1-xx:xx)])dot(x+xx,y+yy);}
 function projectile(x,y,data=m.pet,flip=false){const attack=ui.care.attacks['Attack_'+(data.stats.attack_sprite||1)]||ui.care.attacks.Attack_1;sprite({width:attack.width,height:attack.height,animations:{Attack:attack.sprite}},x,y,'Attack',flip,0);}
 function poop(count,wave=-Infinity){for(let i=0;i<Math.min(2,count);i++)if(wave<=8-i*8)bitmap(ui.care?.poop[0],8,8,24,8-i*8,frame%2===1);}
 const s=m.stats,screen=m.screen,pet=m.pet;
 if(!walkers.has(m))walkers.set(m,new Wandering());
 const pose=walkers.get(m).update(frame,pet?.width||16,s.sleeping,s.poop>0,screen==='home');
if(!pet){center('LOADING',5);}else if(screen==='home'){sprite(pet,pose.x,Math.floor((16-pet.height)/2),s.sleeping?'Sleep':'Walk',pose.flip);poop(s.poop);if(s.sleeping)bitmap(ui.care.sleep[frame%2],9,8,23,0);}
 else if(screen==='status'){if(m.page===6){center('CARE MISS',1);center(String(s.careMistakes),9);}else if(m.page===0){text(`${Math.floor(s.totalTimeAlive/1440)}YR`,0,2,16);text(`${s.weight}G`,0,9,16);sprite(pet,16,0);}if(m.page===1||m.page===2){center(m.page===1?'HUNGER':'STRENGTH',1);for(let i=0;i<4;i++)heart(2+i*7,9,i<(m.page===1?s.hunger:s.strength));}if(m.page===3){center('ENERGY',1);center(`${s.energy}/${m.maxEnergy()}`,9);}if(m.page===4){center('WIN '+(s.battles?Math.floor(s.wins/s.battles*100):0)+'%',1);center(`${s.wins}/${s.battles}`,9);}if(m.page===5){center('TRAIN',1);center(String(s.trainingsCurStage),9);}}
 else if(screen==='feed'){bitmap(ui.care.meat[0],8,8,15,0);bitmap(ui.care.protein[0],8,8,15,8);bitmap(ui.care.arrow[0],4,7,4,m.choice===0?0:9);}
 else if(screen==='care'&&m.choice===2){center('NEW EGG',1);center('B:START',9);}
 else if(screen==='care'){text(m.choice===0?'>':' ',0,1);text(m.choice===1?'>':' ',0,9);text(s.sleeping?'LIGHT ON':'LIGHT OFF',5,1,27);text('TOILET',5,9,27);}
 else if(screen==='train'||screen==='battle'){center(screen==='train'?'TRAIN':'BATTLE',1);center('B:START',9);}
 else if(screen==='timing'||screen==='dungeonTiming'||screen==='battleTiming'){text('HIT',10,0);for(let x=2;x<30;x++){dot(x,7);dot(x,11);}for(let y=7;y<12;y++){dot(2,y);dot(29,y);}const w=trainingWindow(s,m.maxEnergy());for(let x=3+w.min;x<=4+w.max;x++){dot(x,6);dot(x,12);}const x=3+trainingPosition(now-m.started);for(let y=8;y<11;y++){dot(x,y);dot(x+1,y);}}
 else if(screen==='animation'){
  if(m.animation==='Egg'){const elapsed=now-m.actionStarted;sprite(pet,8,0,elapsed>=10000?'hatch':'egg '+(Math.floor(elapsed/800)%2+1),false,0);}
  else if(m.animation==='Evolve'||m.animation==='Jogress'){if(Math.floor((now-m.actionStarted)/180)%2===0)sprite(pet,8,0,'Happy',false,0);else for(let y=0;y<16;y++)for(let x=0;x<32;x++)if((x+y)%2===0)dot(x,y);}
  else if(m.animation==='Item'){const item=m.special?.items.find(x=>x.id===m.play?.itemId);if(item)bitmap(item.icon,12,12,10,2);else sprite(pet,8,0,'Happy');}
  else if(m.animation==='TrainHit'||m.animation==='TrainMiss'){
   const elapsed=Math.max(0,now-m.actionStarted),step=Math.floor(elapsed/500),fraction=(elapsed%500)/500;
   if(m.animation==='TrainMiss')sprite(pet,2,Math.floor((16-pet.height)/2),'Injured 1',true,0);
   else if(step<2){sprite(pet,2,Math.floor((16-pet.height)/2),'Attack',true,0);const x=Math.round(20+(step+fraction)*6);if(m.trainingHitType==='mega'){projectile(x,0);projectile(x,8);}else projectile(x,4);}
   else if(step<6){bitmap(ui.care.walls.Wall_1.sprite,8,16,24,0);const x=Math.round(16-(6-step-fraction)*6);if(m.trainingHitType==='mega'){projectile(x,0);projectile(x,8);}else projectile(x,4);}
   else if(step<9){const invert=Math.floor((elapsed%500)/80)%2===1;const pixels=ui.care.hits.Hit_1.sprite;for(let y=0;y<16;y++)for(let x=0;x<32;x++)if(Boolean(pixels[y*32+x])!==invert)dot(x,y);}
   else bitmap((m.trainingHitType==='mega'?ui.care.walls.Wall_2:ui.care.walls.Wall_1).sprite,8,16,24,0);
  }else if(m.animation==='Eat'){

   const step=actionStep(m.actionStarted,now,CARE_TIMING.eatStep),bite=foodStage(step);
   sprite(pet,2,Math.floor((16-pet.height)/2),step%2===0?'Eat 1':'Eat 2',true,0);
   if(bite<3)bitmap((m.actionFood===0?ui.care.meat:ui.care.protein)[bite],8,8,20,4);
  }else if(m.animation==='Wash'){
   const wave=-6+actionStep(m.actionStarted,now,CARE_TIMING.washStep);
   sprite(pet,pose.x,Math.floor((16-pet.height)/2),'Walk',pose.flip,0);poop(s.poop,wave);
   for(let y=0;y<Math.min(16,wave+6);y++)for(let x=0;x<32;x++)room[y*32+x]=null;
   for(let y=0;y<Math.min(16,Math.max(0,wave));y++)for(let x=0;x<32;x++)room[y*32+x]=(x+y)%2===0?ink:null;
   for(let i=0;i<4;i++)bitmap(ui.care.water[0],8,6,i*8,wave);
  }else if(['Happy','Angry','Refuse'].includes(m.animation)){
   const step=actionStep(m.actionStarted,now,m.animation==='Happy'?594:800),alternate=step%2;
   const kind=m.animation==='Happy'?(alternate?'Eat 2':'Happy'):m.animation==='Angry'?(alternate?'Injured 1':'Angry'):'Refuse';
   sprite(pet,8,Math.floor((16-pet.height)/2),kind,m.animation==='Refuse'&&alternate===1,0);
   if(!alternate)bitmap(m.animation==='Happy'?ui.care.happy[0]:ui.care.angry[0],10,m.animation==='Happy'?8:10,22,2);
  }else if(m.animation==='BattleIntro'){if(Math.floor((now-m.actionStarted)/500)%2===0)center('START',5);}
  else if(m.animation==='Battle'){drawBattle(m,now,{sprite,projectile,dot,bitmap,hit:ui.care.hits.Hit_1.sprite});
  }else if(now>m.animationUntil-650)center(m.outcome,5);
  else sprite(pet,8,0,m.animation);
 }
 else if(screen==='galleryInfo'){center(m.catalog[m.galleryIndex].stage,1);center('READONLY',9);}
 else if(screen==='gallery'){let c=m.catalog[m.galleryIndex];text(c.name,0,1,16);text(`VIEW ${m.galleryIndex+1}`,0,10,16);sprite(m.pending,16,0);}
 else if(screen==='powerConfirm'){center('SLEEP?',1);text(m.choice===0?'>NO YES':' NO>YES',0,9);}
 else if(screen==='play'){const v=playView(m);if(v.path){sprite(m.eggPreview,8,0,'egg 1',false,0);bitmap(ui.care.arrow[0],4,7,2,4,true);bitmap(ui.care.arrow[0],4,7,26,4);}else if(v.icon){bitmap(v.icon,12,12,0,2);text(v.title,14,0,18);text(v.line,14,9,18);}else{center(v.title,1);text(v.line,0,9);}}
 else if(screen==='evo'&&m.page===5){center('SPECIAL',1);center('B:OPEN',9);}
 else if(screen==='evo'){let e=routes[m.path];if(m.page===0){center('EVOLVE',1);let p=progress(pet.stats.stage,s);let time=e?Math.max(0,p.max-s.timeInCurrentStage):0;center(m.progress.evoBlocked||m.progress.levelPlus?'LOCKED':e?time+'MIN':'MAX',9);}else{const key=['best','middle','worst'][m.page-2];center(m.page===1?'NEXT':key,1);let name=(m.page===1?m.predictedEvolution():e?.[key])?.split('/').pop().replace('.json','')||'NONE';text(name,0,9);}}
 else if(screen==='settings'){const titles=['SOUND','BRIGHT','COLOR','THEME','SPEED','TIME','CLOCK','AUTO OFF','ABOUT','BACKGND','STYLE','POWER','PLAY'];let values=[['OFF','LOW','MED','HIGH'][m.settings.sound],['25%','50%','75%','100%'][m.settings.brightness],m.settings.color?'ON':'OFF',['GREEN','GRAY','AMBER'][m.settings.theme],m.settings.speed+'X','+1 HOUR','B:VIEW',['NEVER','30SEC','1MIN','5MIN'][m.settings.timeout],'V2.5.2',m.settings.background<0?'LCD':('BG '+(m.settings.background+1)),m.settings.style?'CLEAR':'RETRO','LIGHT','B:OPEN'];center(titles[m.page],1);center(values[m.page],9);}
 else if(screen==='clock'){center(new Date(now).toLocaleTimeString('en-GB',{hour:'2-digit',minute:'2-digit'}),2);center('A:SET',10);}
 ctx.fillStyle=bg;ctx.fillRect(0,0,256,240);const scenery=ui.backgrounds?.[m.settings.background];if(scenery){for(let by=0;by<60;by++)for(let bx=0;bx<64;bx++){let n=scenery.pixels[by*64+bx];ctx.fillStyle=`rgb(${((n>>11)&31)*255/31},${((n>>5)&63)*255/63},${(n&31)*255/31})`;ctx.fillRect(bx*4,by*4,4,4);}if(!['home','animation','gallery'].includes(screen)){ctx.fillStyle=bg+'dc';ctx.fillRect(12,60,232,120);}}for(let y=0;y<16;y++)for(let x=0;x<32;x++){if(!room[y*32+x]&&(scenery||m.settings.style))continue;ctx.fillStyle=room[y*32+x]||ghost;ctx.globalAlpha=room[y*32+x]?1:.13;ctx.fillRect(16+x*7,64+y*7,m.settings.style?7:6,m.settings.style?7:6);}ctx.globalAlpha=1;
 for(let i=0;i<8;i++){let active=m.cursor===i,x=20+(i%4)*60,y=i<4?23:185;ctx.fillStyle=active?ink:ghost;for(let yy=0;yy<16;yy++)for(let xx=0;xx<16;xx++)if(ui.icons[i]?.[yy*16+xx])ctx.fillRect(x+xx*2,y+yy*2,2,2);if(active){ctx.fillRect(x+7,y+35,18,2);}}
 ctx.fillStyle='#202521';ctx.fillRect(256,0,64,240);for(let i=0;i<3;i++){ctx.fillStyle='#363e36';ctx.beginPath();ctx.arc(288,40+i*80,20,0,Math.PI*2);ctx.fill();ctx.strokeStyle='#596451';ctx.lineWidth=1;ctx.stroke();let a=ui.font[['A','B','C'][i]];ctx.fillStyle='#e2e8d6';for(let y=0;y<5;y++)for(let x=0;x<3;x++)if(a[y*3+x])ctx.fillRect(284+x*3,33+i*80+y*3,3,3);}
 // Brightness is previewed; hardware uses its actual backlight.
 canvas.style.filter=`brightness(${[.55,.7,.85,1][m.settings.brightness]})`;
}
