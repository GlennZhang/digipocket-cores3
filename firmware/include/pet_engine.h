#pragma once
#include <stdint.h>
#include <string.h>
struct PetState {int hunger=4,strength=4,energy=30,happy=80,poop=0,care=0,training=0,battles=0,wins=0,stageMinutes=0,totalMinutes=0,weight=0;bool sleeping=false;};
inline int stageDuration(const char* s){if(!strcmp(s,"Baby I"))return 10;if(!strcmp(s,"Baby II"))return 360;if(!strcmp(s,"Child"))return 720;if(!strcmp(s,"Adult")||!strcmp(s,"Armor-Hybrid"))return 2160;if(!strcmp(s,"Perfect")||!strcmp(s,"Ultimate-Super Ultimate")||!strcmp(s,"Mega"))return 2880;return 0;}
inline int routeQuality(const char* s,const PetState& p){if(!strcmp(s,"Baby I")||!strcmp(s,"Baby II"))return p.care<3?0:p.care<=5?1:2;if(!strcmp(s,"Child"))return p.care>=4?2:p.training>24?0:1;if(!strcmp(s,"Adult"))return p.care>=4?2:p.battles>39?0:1;return p.care>=3?2:p.battles>(!strcmp(s,"Ultimate-Super Ultimate")||!strcmp(s,"Mega")?99:79)?0:1;}
inline void minuteTick(PetState& p,int energyMax){p.stageMinutes++;p.totalMinutes++;if(p.totalMinutes%60==0){if(p.sleeping){p.energy+=4;if(p.energy>energyMax)p.energy=energyMax;}else{if(p.hunger>0)p.hunger--;if(p.strength>0)p.strength--;if(p.poop<8)p.poop++;if(!p.hunger||p.poop>=8)p.care++;}}}

inline const char* selectRoute(const char* best,const char* middle,const char* worst,int q){const char* p[3]={q==0?best:q==1?middle:worst,q==1?best:middle,q==0?worst:q==1?worst:best};for(auto s:p)if(s&&*s)return s;return nullptr;}
