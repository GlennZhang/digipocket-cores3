// Source sprites face left. Only flip them when travelling to the right.
export function walkingPose(frame,width=16,sleeping=false){
 const position=f=>Math.min(Math.max(0,32-width),Math.max(0,8+Math.round(Math.sin(f/4)*5)));
 if(sleeping)return {x:Math.min(8,Math.max(0,32-width)),flip:false};
 const x=position(frame);let flip=true;
 // Keep the last travel direction during repeated positions at a turn.
 for(let i=1;i<=32;i++){const previous=position(frame-i);if(previous!==x){flip=x>previous;break;}}
 return {x,flip};
}

// Match the reference's two-pixel wandering: 40% chance to move each main tick,
// then 20% chance to turn. A fixed PRNG keeps browser/hardware behavior comparable.
export class Wandering {
 constructor(){this.x=8;this.right=false;this.facingRight=false;this.lastTick=null;this.seed=0x6d2b79f5;}
 random(){let x=this.seed;x^=x<<13;x^=x>>>17;x^=x<<5;this.seed=x>>>0;return this.seed/4294967296;}
 update(tick,width=16,sleeping=false,poop=false,active=true){
  const limit=Math.max(0,32-width);this.x=Math.min(limit,this.x);
  if(this.lastTick===null)this.lastTick=tick;
  let count=Math.min(32,Math.max(0,tick-this.lastTick));this.lastTick=tick;
  if(active&&!sleeping)while(count-->0){
   if(this.random()>=.4)continue;
   const next=this.x+(this.right?2:-2);
   if(next<0||next>limit||(poop&&next<32&&next+width>24)){this.right=!this.right;this.facingRight=this.right;continue;}
   this.x=next;this.facingRight=this.right;if(this.random()<.2)this.right=!this.right;
  }
  return {x:sleeping?Math.min(7,limit):this.x,flip:sleeping?false:this.facingRight};
 }
}
