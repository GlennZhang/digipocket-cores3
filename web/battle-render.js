import {battleScene} from './battle.js';
export function drawBattle(m,now,{sprite,projectile,dot,bitmap,hit}){
 const s=battleScene(m.battle,now-m.actionStarted),f=s.frame,r=s.round||{},a=m.pet,b=m.opponent,frac=s.t/500;
 const own=(kind='Walk 1',flip=true)=>sprite(a,4,Math.floor((16-a.height)/2),kind,flip,0),enemy=(kind='Walk 1',flip=false)=>sprite(b,12,Math.floor((16-b.height)/2),kind,flip,0);
 const hp=(value,x)=>{for(let i=0;i<Math.min(5,value);i++)for(let xx=0;xx<2;xx++)for(let yy=0;yy<2;yy++)dot(x+xx,14-i*3+yy);};
 const shot=(x,data,reverse,count)=>{if(count===2){projectile(x,0,data,reverse);projectile(x,8,data,reverse);}else projectile(x,4,data,reverse);};
 const impact=(reverse)=>{const invert=Math.floor(s.t/80)%2===1;for(let y=0;y<16;y++)for(let x=0;x<32;x++)if(Boolean(hit[y*32+(reverse?31-x:x)])!==invert)dot(x,y);};
 if(f>=40){const n=f-40;if(n<4){hp(s.own,0);own(n%4>=2?'Attack':'Walk 2');}else{hp(s.rival,30);enemy((n-4)%4>=2?'Attack':'Walk 2');}return;}
 const showOwn=f<=2||f>=13&&f<=19||f>=29&&f<=32,showRival=f>=3&&f<=12||f>=24&&f<=27;
 if(showOwn&&!(f>=15&&f<=17&&r.hitB))hp(s.own,0);if(showRival&&!(f>=5&&f<=7&&r.hitA))hp(s.rival,30);
 if(f===0)own();else if(f<=2){own('Attack');shot(Math.round(4+a.width+(f-1+frac)*6),a,false,r.shotsA);}
 else if(f<=4){enemy();shot(Math.round(4-(5-f-frac)*6),a,false,r.shotsA);}
 else if(f<=9){if(!r.hitA)enemy('Walk 1',true);else if(f<=7)impact(false);else enemy(f===8?'Injured 1':'Injured 2');}
 else if(f===10)enemy();else if(f<=12){enemy('Attack');shot(Math.round(4-(f-11+frac)*6),b,true,r.shotsB);}
 else if(f<=14){own();shot(Math.round(4+a.width+(15-f-frac)*6),b,true,r.shotsB);}
 else if(f<=19){if(!r.hitB)own('Walk 1',false);else if(f<=17)impact(true);else own(f===18?'Injured 1':'Injured 2');}
 else if(f>=24&&f<=27)enemy(f%2===0?'Injured 1':'Injured 2');else if(f>=29&&f<=32)own(f%2===1?'Injured 1':'Injured 2');
}
