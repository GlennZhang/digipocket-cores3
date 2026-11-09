#pragma once
#include <cmath>
#include <stdint.h>
#include <algorithm>
struct WalkingPose {int x;bool flip;};
inline WalkingPose walkingPose(int frame,int width,bool sleeping){
 auto position=[width](int f){return std::min(std::max(0,32-width),std::max(0,8+(int)std::floor(std::sin(f/4.0)*5+0.5)));};
 if(sleeping)return {std::min(8,std::max(0,32-width)),false};
 int x=position(frame);bool flip=true;
 for(int i=1;i<=32;i++){int previous=position(frame-i);if(previous!=x){flip=x>previous;break;}}
 return {x,flip};
}

struct Wandering {
 int x=8;bool right=false,facingRight=false,started=false;uint32_t lastTick=0,seed=0x6d2b79f5;
 double random(){uint32_t v=seed;v^=v<<13;v^=v>>17;v^=v<<5;seed=v;return seed/4294967296.0;}
 WalkingPose update(uint32_t tick,int width,bool sleeping=false,bool poop=false,bool active=true){
  int limit=std::max(0,32-width);x=std::min(limit,x);
  if(!started){started=true;lastTick=tick;}
  int count=std::min((uint32_t)32,tick-lastTick);lastTick=tick;
  if(active&&!sleeping)while(count-->0){
   if(random()>=.4)continue;
   int next=x+(right?2:-2);
   if(next<0||next>limit||(poop&&next<32&&next+width>24)){right=!right;facingRight=right;continue;}
   x=next;facingRight=right;if(random()<.2)right=!right;
  }
  return {sleeping?std::min(7,limit):x,sleeping?false:facingRight};
 }
};
