#pragma once
#include <stdint.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include "battle_attributes.h"
struct Fighter {int stage=3,attribute=0,baseWeight=0,training=0,total=0,age=0,battles=0,strength=0,strengthMax=4,hunger=0,hungerMax=4,weight=0;};
struct BattleRound {bool hitA,hitB;int shotsA,shotsB,own,rival;};
struct BattleStep {int frame,index,own,rival,duration;};
struct BattleLog {BattleRound rounds[20];BattleStep steps[512];int count=0,stepCount=0,result=0,initialOwn=5,initialRival=5;bool dungeon=false;uint32_t duration=0;};
inline int battleStage(const char*s){const char* stages[]={"Digitama","Baby I","Baby II","Child","Adult","Perfect","Armor-Hybrid","Ultimate-Super Ultimate","Mega","Special"};const int values[]={0,1,2,3,4,5,6,6,8,12};for(int i=0;i<10;i++)if(!strcmp(s,stages[i]))return values[i];return 3;}
inline double battleAdvantage(int a,int b){return (a==1&&b==2)||(a==2&&b==3)||(a==3&&b==1)?.05:(b==1&&a==2)||(b==2&&a==3)||(b==3&&a==1)?-.05:0;}
struct BattleBonuses {double stage,training,total,age,battles,strength,hunger,weight,attribute;};
struct BattleChances {double own,rival;bool sameStageCapped=false,specialAdjusted=false;};
inline BattleBonuses battleBonuses(const Fighter&a,const Fighter&b){
 return {(a.stage-b.stage)*.1,std::min(a.training/999.0*.1,.1),std::min(a.total/6000.0*.2,.2),std::min(a.age/30.0*.05,.05),std::min(a.battles/999.0*.15,.15),(a.strengthMax?(double)a.strength/a.strengthMax:0)*.1-.05,(a.hungerMax?(double)a.hunger/a.hungerMax:0)*.1-.05,a.baseWeight>0?(1-std::min(1.0,std::abs(a.weight-a.baseWeight)/15.0))*.1-.05:0,battleAdvantage(a.attribute,b.attribute)};
}
inline double battleAccuracy(const Fighter&a,const Fighter&b){auto v=battleBonuses(a,b);return .1+v.stage+v.training+v.total+v.age+v.battles+v.strength+v.hunger+v.weight+v.attribute;}
inline BattleChances battleInitialChances(const Fighter&a,const Fighter&b,bool trainingBattle=true,double enemyAccuracy=-1){
 BattleChances result;result.own=std::floor(std::min(1.0,std::max(0.0,battleAccuracy(a,b)))*1000+.5)/1000;result.rival=std::floor(std::min(1.0,std::max(0.0,enemyAccuracy>=0?enemyAccuracy:trainingBattle?.1:battleAccuracy(b,a)))*1000+.5)/1000;
 if(a.stage==b.stage&&result.own>.7&&!trainingBattle&&enemyAccuracy<0&&result.rival>.7){result.own=result.rival=.6;result.sameStageCapped=true;}
 if(b.stage>=12&&result.own>.45){result.own=.25;result.specialAdjusted=true;}
 return result;
}

inline void buildBattleTimeline(BattleLog&log){int own=log.initialOwn,rival=log.initialRival,frame=0,index=0;log.stepCount=0;log.duration=0;auto push=[&](int f,int ms){if(log.stepCount<512){log.steps[log.stepCount++]={f,index,std::max(0,own),std::max(0,rival),ms};log.duration+=ms;}};for(int i=0;i<8;i++)push(40+i,450);push(0,500);for(int guard=0;guard<512;guard++){frame++;if(frame==1&&index>=log.count){frame=rival<=0||own>rival?24:29;push(frame,500);continue;}const BattleRound* r=index<log.count?&log.rounds[index]:nullptr;if(frame==5&&r&&r->hitA)rival-=r->shotsA;if(frame==7&&(!r||!r->hitA))frame=9;if(frame==10&&rival<=0)frame=24;if(frame==15&&r&&r->hitB)own-=r->shotsB;if(frame==17&&(!r||!r->hitB))frame=19;if(frame==20){if(own<=0)frame=29;else{frame=0;index++;}}if(frame==28||frame==33)break;push(frame,500);}log.result=own==rival?0:own>rival?1:-1;}
inline void fight(BattleLog&log,const Fighter&a,const Fighter&b,int typeA,int typeB,uint32_t seed,bool trainingBattle=true,int ownHP=5,int rivalHP=5,int maxRounds=5,double enemyAccuracy=-1){uint32_t value=seed;auto random=[&](){uint32_t t=(value+=1831565813u);t=(t^(t>>15))*(t|1u);t^=t+(t^(t>>7))*(t|61u);return (double)(t^(t>>14))/4294967296.0;};log.count=0;log.initialOwn=ownHP;log.initialRival=rivalHP;log.dungeon=false;int own=ownHP,rival=rivalHP;double missA=0,missB=0;if(a.battles>=999)typeA=2;if(b.battles>=999)typeB=2;auto initial=battleInitialChances(a,b,trainingBattle,enemyAccuracy);for(int i=0;i<std::min(20,maxRounds)&&own>0&&rival>0;i++){double pa=initial.own,pb=initial.rival;int shotsA=random()<(typeA==2?.9:typeA==0?.2:.5)?2:1;bool hitA=random()<pa+missA;missA=hitA?0:missA+.1;int shotsB=random()<(typeB==2?.9:typeB==0?.2:.5)?2:1;bool hitB=random()<pb+missB;missB=hitB?0:missB+.1;if(hitA&&hitB&&own-shotsB<=0&&rival-shotsA<=0){bool hostWins=a.age>b.age?true:b.age>a.age?false:random()<.5;if(hostWins)hitB=false;else hitA=false;}if(hitA)rival-=shotsA;if(hitB)own-=shotsB;log.rounds[log.count++]={hitA,hitB,shotsA,shotsB,own,rival};}buildBattleTimeline(log);}
inline const BattleStep& battleScene(const BattleLog&b,uint32_t elapsed,uint32_t&within){uint32_t start=0;for(int i=0;i<b.stepCount;i++){auto&s=b.steps[i];if(elapsed<start+s.duration){within=elapsed-start;return s;}start+=s.duration;}within=499;return b.steps[b.stepCount-1];}
