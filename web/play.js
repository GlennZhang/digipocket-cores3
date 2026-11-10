import {o as routes} from './data/evolution.js';
const activeKeys=['energyBonus','evoBlocked','levelPlus','fullMinutes','autoCleanMinutes'];
export const playMenus=['ITEMS','SHOP','PARTNERS','JOGRESS','SPECIAL','DUNGEON','NEW EGG'];
export function newProgress(){return {bits:0,inventory:{},bank:Array(10).fill(null),bankLimit:5,energyBonus:0,evoBlocked:false,levelPlus:false,fullMinutes:0,autoCleanMinutes:0,dungeon:{floor:1,hp:10,status:0}};}
export async function ensurePlay(m){if(!m.special)m.special=await m.data('data/special.json');if(!m.balance)m.balance=await m.data('data/game-balance.json');return m.special;}
export async function openPlay(m,mode='menu'){await ensurePlay(m);m.play={mode,index:0,choice:0};if(mode==='eggs')m.eggPreview=await m.data(eggPath(m));m.screen='play';}
export function maxEnergy(m){return (m.pet?.stats?.energy_max??30)+(m.progress.energyBonus||0);}
export function eggPath(m){return Object.keys(m.special.eggs)[m.play.index%Object.keys(m.special.eggs).length];}
const rewindChip={id:'rewind_chip',name:'Rewind Chip',price:10000};
function rewindTargets(m){
 const history=m.stats.evolutionHistory||[];
 if(history.length)return [history.at(-1)];
 if(m.stats.fusionBarrier||m.pet.stats.stage==='Digitama')return [];
 return Object.keys(routes).filter(path=>Object.values(routes[path]).includes(m.path));
}
export function itemsList(m,shop=false){return [...m.special.items,rewindChip].filter(i=>shop?i.price!==null:(m.progress.inventory[i.id]||0)>0);}
export function specialList(m){return [...(m.special.armor[m.path]||[]),...(m.special.xRoutes[m.path]?[{itemId:'x_antibody',result:m.special.xRoutes[m.path]}]:[])];}
export function miracleTargets(m,id){if(id==='miracle_egg')return m.catalog.filter(c=>c.stage==='Ultimate-Super Ultimate'&&!c.path.includes('Atratusmon')).map(c=>c.path);const seen=new Set(),pending=[m.path],out=[];while(pending.length){const p=pending.pop();if(seen.has(p))continue;seen.add(p);if(p!==m.path&&p.startsWith('database/Ultimate-Super Ultimate/')&&!p.includes('Atratusmon'))out.push(p);for(const next of Object.values(routes[p]||{}))if(next)pending.push(next);}return out.sort();}
export function jogressTarget(m,slot){const partner=m.progress.bank[slot];return partner?m.special.jogress[[m.path,partner.path].sort().join('|')]||null:null;}
function activeFlags(m){return Object.fromEntries(activeKeys.map(k=>[k,m.progress[k]]));}
function resetActive(m){for(const key of activeKeys)m.progress[key]=key==='evoBlocked'||key==='levelPlus'?false:0;}
export function snapshot(m){return {path:m.path,stats:structuredClone(m.stats),flags:activeFlags(m)};}
export async function restoreSnapshot(m,pet){await m.data(pet.path);await m.select(pet.path);Object.assign(m.progress,pet.flags);m.stats=structuredClone(pet.stats);m.stats.totalTrainings??=m.stats.trainingsCurStage||0;m.stats.energy=Math.min(m.stats.energy,maxEnergy(m));if(m.pet.stats.stage==='Digitama'){m.animate('Egg','HATCHING','home',12000);m.pendingEffect={type:'hatch',path:m.special.eggs[m.path]};}else m.animate('Happy','WELCOME','home');}
export async function beginEgg(m,path){await ensurePlay(m);if(!m.special.eggs[path])throw Error('Not a starter egg');await m.data(path);resetActive(m);await m.select(path,true);m.animate('Egg','HATCHING','home',12000);m.pendingEffect={type:'hatch',path:m.special.eggs[path]};}
export async function finishHatch(m,path){m.busy=true;try{await m.select(path,true);m.stats.energy=maxEnergy(m);m.animate('Happy','HELLO!','home');}catch{m.message='孵化素材不可用';}finally{m.busy=false;}}
export async function evolveTo(m,path,magic=false,record=false){const previous=m.path;if(m.progress.levelPlus)throw Error('Level+ 伙伴不能继续进化');await m.data(path);m.progress.energyBonus=Math.min(999,m.progress.energyBonus+Math.floor(m.stats.trainingsCurStage/5));await m.select(path);m.stats.evolutionHistory=record?[...(m.stats.evolutionHistory||[]),previous].slice(-16):[];m.stats.careMistakes=0;m.stats.trainingsCurStage=magic?999:0;m.stats.poop=0;m.animate('Evolve','EVOLVE!','home',5000);}
export function planItem(m,id){if(id==='rewind_chip'){
  const targets=rewindTargets(m);
  if(!targets.length)throw Error('NO PREVIOUS FORM');
  return {targets};
 }
 if(!m.special.items.some(x=>x.id===id))throw Error('UNSUPPORTED');const p=m.progress,s=m.stats;
 if(p.levelPlus&&(['x_antibody','best_fruit','normal_fruit','worst_fruit','miracle_egg','miracle_fruit','orb_of_wishes','jogres_disk'].includes(id)||m.special.items.find(x=>x.id===id)?.category==='Armor-Spirit'))throw Error('LEVEL+ LOCK');
 const matching=specialList(m).filter(x=>x.itemId===id);if(matching.length)return {targets:matching.map(x=>x.result)};
 if(m.special.items.find(x=>x.id===id)?.category==='Armor-Spirit'||id==='x_antibody')throw Error('NO RECIPE');
 if(['best_fruit','normal_fruit','worst_fruit'].includes(id)){const r=routes[m.path],to=id==='best_fruit'?r?.best:id==='normal_fruit'?r?.middle||r?.best:r?.worst||r?.best;if(!to)throw Error('NO ROUTE');return {targets:[to]};}
 if(id==='miracle_egg'||id==='miracle_fruit'){const targets=miracleTargets(m,id);if(!targets.length)throw Error('NO ROUTE');return {targets,magic:true};}
 if(id==='orb_of_wishes'){if(m.path!=='database/Baby II/Poromon.json')throw Error('NEED POROMON');return {targets:['database/Child/Ukkomon.json']};}
 if(id==='jogres_disk')return {jogress:true};
 if(id==='heart_candy'&&(p.dungeon.hp>=10||p.dungeon.status!==0))throw Error('NO NEED');
 if(id==='storage_drive'&&p.bankLimit>=10)throw Error('BANK FULL');
 if(id==='level_plus'&&(p.levelPlus||!['Child','Adult','Perfect'].includes(m.pet.stats.stage)))throw Error('NOT ELIGIBLE');
 return {modify:true};
}
export async function useItem(m,id,target){const p=m.progress;if(!(p.inventory[id]>0))throw Error('NEED ITEM');const plan=planItem(m,id);if(plan.targets){if(!plan.targets.includes(target))throw Error('BAD TARGET');if(id==='rewind_chip'){
  await m.data(target);
  await m.select(target);
  m.stats.evolutionHistory?.pop();
  m.stats.careMistakes=0;
  m.stats.trainingsCurStage=0;
  p.levelPlus=false;
  m.animate('Evolve','DEVOLVE!','home',5000);
 }else await evolveTo(m,target,plan.magic);
 p.inventory[id]--;return;}
 if(plan.jogress){m.play={mode:'jogress',index:0,choice:0};m.screen='play';return;}
 switch(id){
 case 'energy_drink':m.stats.energy=maxEnergy(m);break;
 case 'training_pack':m.stats.totalTrainings=(m.stats.totalTrainings||0)+Math.min(10,999-m.stats.trainingsCurStage);m.stats.trainingsCurStage=Math.min(999,m.stats.trainingsCurStage+10);break;
 case 'elite_training_pack':m.stats.totalTrainings=(m.stats.totalTrainings||0)+Math.min(100,999-m.stats.trainingsCurStage);m.stats.trainingsCurStage=Math.min(999,m.stats.trainingsCurStage+100);break;
 case 'super_carrot':m.stats.weight=Math.max(1,m.stats.weight-10);break;
 case 'time_gear':{const previous=m.path;await m.tick(120);if(m.path!==previous){p.inventory[id]--;return;}break;}
 case 'anti_evo_chip':p.evoBlocked=!p.evoBlocked;break;
 case 'level_plus':p.levelPlus=true;break;
 case 'care_mistake_eraser':m.stats.careMistakes=0;break;
 case 'junk_food':m.stats.careMistakes++;break;
 case 'premium_meat':m.stats.hunger=m.stats.strength=4;p.fullMinutes=m.balance.overrides.premium_meat.durationMinutes;break;
 case 'poop_clean_pill':m.stats.poop=0;p.autoCleanMinutes=m.balance.overrides.poop_clean_pill.durationMinutes;break;
 case 'alarm_clock':m.stats.sleeping=false;break;
 case 'sleeping_pill':m.stats.sleeping=true;break;
 case 'storage_drive':p.bankLimit++;break;
 case 'heart_candy':p.dungeon.hp=Math.min(10,p.dungeon.hp+5);break;
 default:throw Error('UNSUPPORTED');
 }
 p.inventory[id]--;m.animate('Item','ITEM USED','play',2400);m.play={mode:'bag',index:0,choice:0,itemId:id};
}
function confirm(m,operation,title,extra={}){m.play={...m.play,parent:{...m.play},mode:'confirm',choice:0,operation,title,...extra};}
export async function playPress(m,key,hold=false){const q=m.play,p=m.progress;
 if(key==='C'){if(q.mode==='eggs'&&m.pendingBankSlot!==undefined){delete m.pendingBankSlot;m.play={mode:'bank',index:0,choice:0};return;}if(q.mode==='confirm')m.play=q.parent;else if(q.mode==='menu'){m.screen='home';m.cursor=-1;}else m.play={mode:'menu',index:0,choice:0};return;}
 let count=q.mode==='menu'?playMenus.length:q.mode==='bag'?itemsList(m).length:q.mode==='shop'?itemsList(m,true).length:q.mode==='eggs'?Object.keys(m.special.eggs).length:q.mode==='bank'||q.mode==='jogress'?p.bankLimit:q.mode==='bankAction'?3:q.mode==='special'?specialList(m).length:q.mode==='targets'?q.targets.length:1;
 if(key==='A'){if(q.mode==='confirm')q.choice^=1;else q.index=(q.index+(hold?10:1))%Math.max(1,count);if(q.mode==='eggs')m.eggPreview=await m.data(eggPath(m));return;}
 if(key!=='B')return;
 try{
 if(q.mode==='menu'){const mode=['bag','shop','bank','jogress','special','dungeon','eggs'][q.index];if(mode==='eggs')await openPlay(m,mode);else m.play={mode,index:0,choice:0};return;}
 if(q.mode==='bag'||q.mode==='shop'){const item=itemsList(m,q.mode==='shop')[q.index];if(!item){m.message='背包为空，可在商店购买或通过挑战获得';return;}if(q.mode==='shop'){confirm(m,'buy',item.name,{itemId:item.id});return;}const plan=planItem(m,item.id);if(plan.targets?.length>1){m.play={mode:'targets',index:0,targets:plan.targets,itemId:item.id};return;}if(plan.jogress){m.play={mode:'jogress',index:0,choice:0};return;}confirm(m,'use',item.name,{itemId:item.id,target:plan.targets?.[0]});return;}
 if(q.mode==='eggs'){confirm(m,'egg','NEW EGG?',{target:eggPath(m)});return;}
 if(q.mode==='bank'){m.play={mode:'bankAction',slot:q.index,index:0};return;}
 if(q.mode==='bankAction'){if(q.index===0&&p.bank[q.slot])throw Error('SLOT USED');if(q.index>0&&!p.bank[q.slot])throw Error('EMPTY SLOT');confirm(m,['store','swap','release'][q.index],['STORE+EGG?','SWAP?','RELEASE?'][q.index],{slot:q.slot});return;}
 if(q.mode==='jogress'){const target=jogressTarget(m,q.index);if(!target)throw Error('NO DNA PAIR');if(!(p.inventory.jogres_disk>0))throw Error('NEED DISK');confirm(m,'jogress',target.split('/').pop().replace('.json',''),{slot:q.index,target});return;}
 if(q.mode==='special'){const r=specialList(m)[q.index];if(!r)throw Error('NO RECIPE');confirm(m,'use',r.result.split('/').pop().replace('.json',''),{itemId:r.itemId,target:r.result});return;}
 if(q.mode==='targets'){confirm(m,'use',q.targets[q.index].split('/').pop().replace('.json',''),{itemId:q.itemId,target:q.targets[q.index]});return;}
 if(q.mode==='dungeon'){await m.beginDungeon();return;}
 if(q.mode==='confirm'){
  if(!q.choice){m.play=q.parent;return;}
  if(q.operation==='restartDungeon'){p.dungeon={floor:1,hp:10,status:0};m.play={mode:'dungeon',index:0};}
  else if(q.operation==='buy'){const item=itemsList(m,true).find(i=>i.id===q.itemId);if(item.id==='storage_drive'&&p.bankLimit>=10)throw Error('BANK FULL');if(p.bits<item.price)throw Error('NOT ENOUGH BITS');if((p.inventory[item.id]||0)>=9999)throw Error('ITEM LIMIT');p.bits-=item.price;p.inventory[item.id]=(p.inventory[item.id]||0)+1;m.play=q.parent;m.message='购买成功';}
  else if(q.operation==='use')await useItem(m,q.itemId,q.target);
  else if(q.operation==='egg'){const previous=m.pendingBankSlot!==undefined?snapshot(m):null;await beginEgg(m,q.target);if(previous){p.bank[m.pendingBankSlot]=previous;delete m.pendingBankSlot;}}
  else if(q.operation==='store'){if(m.pet.stats.stage==='Digitama')throw Error('HATCH FIRST');m.pendingBankSlot=q.slot;await openPlay(m,'eggs');}
  else if(q.operation==='swap'){const next=p.bank[q.slot],old=snapshot(m);await restoreSnapshot(m,next);p.bank[q.slot]=old;}
  else if(q.operation==='release'){p.bank[q.slot]=null;m.play={mode:'bank',index:q.slot,choice:0};}
  else if(q.operation==='jogress'){if(!(p.inventory.jogres_disk>0)||jogressTarget(m,q.slot)!==q.target)throw Error('NO DNA PAIR');await evolveTo(m,q.target);m.stats.fusionBarrier=true;p.inventory.jogres_disk--;p.bank[q.slot]=null;m.stats.energy=Math.max(0,m.stats.energy-5);m.stats.weight=Math.max(1,m.stats.weight-4);m.animation='Jogress';}
 }
 }catch(error){m.message=error.message;m.play=q.mode==='confirm'?q.parent:q;m.screen='play';}
}
export function playView(m){const q=m.play,p=m.progress;
 if(q.mode==='menu')return {title:'BITS '+p.bits,line:playMenus[q.index]};
 if(q.mode==='confirm')return {title:q.title,line:q.choice?' NO >YES':'>NO  YES'};
 if(q.mode==='bag'||q.mode==='shop'){const item=itemsList(m,q.mode==='shop')[q.index];return item?{title:item.name,line:q.mode==='shop'?item.price+'B':'x'+p.inventory[item.id],icon:item.icon}:{title:'ITEMS',line:'EMPTY'};}
 if(q.mode==='eggs'){const egg=eggPath(m);return {title:'NEW EGG',line:egg.split('/').pop().replace('_Digitama.json',''),path:egg};}
 if(q.mode==='bank'||q.mode==='jogress'){const slot=p.bank[q.index];return {title:(q.mode==='bank'?'SLOT ':'DNA ')+(q.index+1),line:slot?slot.path.split('/').pop().replace('.json',''):'EMPTY'};}
 if(q.mode==='bankAction')return {title:'SLOT '+(q.slot+1),line:['STORE','SWAP','RELEASE'][q.index]};
 if(q.mode==='special'){const recipe=specialList(m)[q.index];return recipe?{title:recipe.itemId.replace(/_/g,' '),line:recipe.result.split('/').pop().replace('.json','')}:{title:'SPECIAL',line:'NO RECIPE'};}
 if(q.mode==='targets')return {title:'EVOLVE TO',line:q.targets[q.index].split('/').pop().replace('.json','')};
 const d=p.dungeon;return {title:'F'+d.floor+' HP'+d.hp,line:d.status===1?'CLEARED':d.status===2?'B:RESTART':'B:ENTER'};
}
export async function beginDungeon(m){await ensurePlay(m);const p=m.progress,d=p.dungeon;if(d.status){m.play={mode:'confirm',parent:{mode:'dungeon',index:0},choice:0,operation:'restartDungeon',title:'RESTART?'};m.screen='play';return;}
 if(['Digitama','Baby I','Baby II'].includes(m.pet.stats.stage))throw Error('GROW FIRST');if(m.stats.sleeping||m.stats.poop||m.stats.hunger===0||m.stats.energy<10)throw Error(m.stats.energy<10?'NEED 10 EP':'NEED CARE');
 const stage=d.floor<=10?'Adult':d.floor<=20?'Perfect':d.floor<=30?'Ultimate-Super Ultimate':'Special',pool=m.catalog.filter(c=>c.stage===stage&&c.path!==m.path);if(!pool.length)throw Error('NEED SD');m.opponent=await m.data(pool[Math.floor(Math.random()*pool.length)].path);m.stats.energy-=10;m.dungeonPending=true;m.screen='dungeonTiming';m.started=Date.now();
}
export function dungeonResult(m,hit){const d=m.progress.dungeon;let hp=d.hp,enemyHP=d.floor===31?10:d.floor%10===0?5:3;for(let i=0;i<30&&hp>0&&enemyHP>0;i++){if(hit!=='miss'&&Math.random()<(hit==='mega'?.8:.65))enemyHP-=hit==='mega'?2:1;if(enemyHP>0&&Math.random()<.55)hp-=d.floor===31?2:1;}const win=enemyHP<=0&&hp>0;m.stats.battles++;if(win)m.stats.wins++;m.animate(win?'BattleWin':'BattleLose',win?'WIN!':'LOSE!','play',2800);m.pendingEffect={type:'dungeon',win,hp:Math.max(0,hp)};m.play={mode:'dungeon',index:0};m.dungeonPending=false;}
export function settleDungeon(m,effect){const p=m.progress,d=p.dungeon;if(!effect.win){d.hp=effect.hp;d.status=2;return;}const floor=d.floor;p.bits=Math.min(99999999,p.bits+(floor<=10?2000:floor<=20?3000:floor<=30?5000:200000));d.hp=floor%10===0?10:Math.min(10,Math.max(1,effect.hp));const give=pool=>{const id=pool[Math.floor(Math.random()*pool.length)];p.inventory[id]=Math.min(9999,(p.inventory[id]||0)+1);};if(floor===31)give(m.balance.loot.final);else{const count=floor>=21?2:floor>=11?1:0;for(let i=0;i<count;i++)give(m.balance.loot.regular);if(floor%10===0){give(m.balance.loot.material);if(Math.random()*100<m.balance.bossAidChance)give(m.balance.loot.aid);if(floor>=20&&Math.random()*100<m.balance.bossFruitChance)give(m.balance.loot.fruit);}}if(floor===31){d.hp=10;d.status=1;}else d.floor++;}
