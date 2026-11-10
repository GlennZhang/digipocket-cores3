#include <M5Unified.h>
#include <SD.h>
#include <vector>
#include <Preferences.h>
#include <memory>
#include "ui_assets.h"
#include "care_assets.h"
#include "background_assets.h"
#include "walking_pose.h"
#include "pet_assets.h"
#include "evo_routes.h"
#include "pet_engine.h"
#include "training_rules.h"
#include "shot_training.h"
#include "play_state.h"
#include "battle.h"
#include "battle_intro.h"
BattleLog battleLog;Fighter combatOwn,combatRival;BattleChances combatChances;int combatPage=0,combatOpening=0;
int trainMode=0,shotRound=0,shotHits=0,shotGuard=0,shotGuardY=99,shotLane=0;bool shotFeedback=false,shotSuccess=false;uint32_t shotStarted=0;
// Save restoration uses several nested records; the default 8 KB stack is insufficient.
SET_LOOP_TASK_STACK_SIZE(16384);
M5Canvas canvas(&M5.Display);
enum UiMode {HOME,UI_STATUS,FEED,TRAIN,TIMING,BATTLE,CARE,GALLERY,GALLERY_INFO,EVO,SETTINGS,CLOCK,POWER_CONFIRM,PLAY,DUNGEON_TIMING,BATTLE_TIMING,SHOT_TRAIN,ANIMATION,COMBAT_STATS,MATCH_INFO};
UiMode uiMode=HOME,returnMode=HOME;int cursor=-1,page=0,choice=0,protein=0,soundLevel=1,brightness=3,colorMode=0,theme=0,timeSpeed=1,screenTimeout=0,backgroundIndex=-1,petStyle=0;uint16_t ghost=0x9D50;uint32_t timingStart=0,lastInput=0;bool screenOff=false,sleepRequested=false,ignoreTouch=false;String outcome;
GameState game;PetState state;int petIndex=0,frameNo=0,galleryIndex=0;bool gallery=false,sdReady=false;String message="Hello, Tamer!",action="Walk";uint32_t actionUntil=0,actionStarted=0,lastFrame=0,lastMinute=0;int pendingAction=0,actionFood=0,trainingHitType=1;Wandering wanderer;String serialLine;
struct Entry {String name,path,stage; int energy;int baseWeight;int attack;};std::vector<Entry> catalog;
std::vector<uint16_t> sdPixels;std::vector<String> sdLabels;int sdWidth=0,sdHeight=0;String activeName,activePath,activeStage;
bool evolutionMissing=false;int energyMax=30,baseWeight=5,activeAttack=1;uint16_t bg=0xE775,ink=0x2226;
#include "sd_diagnostics.h"
#include "audio_player.h"
#include "dim_sprites.h"
void report(){Serial.printf("STATE {\"pet\":\"%s\",\"stage\":\"%s\",\"hunger\":%d,\"training\":%d,\"battles\":%d,\"wins\":%d,\"minutes\":%d,\"sleeping\":%s,\"sd\":%s,\"catalog\":%u}\n",activeName.c_str(),activeStage.c_str(),state.hunger,state.training,state.battles,state.wins,state.stageMinutes,state.sleeping?"true":"false",sdReady?"true":"false",catalog.size());}
void say(String s,String a="Happy"){message=s;action=a;actionStarted=millis();actionUntil=actionStarted+(a=="Happy"?2376:2500);Serial.println("EVENT "+s);}
bool selectPet(String path){for(int i=0;i<PET_COUNT;i++)if(path==pets[i].path){sdPixels.clear();sdLabels.clear();petIndex=i;activeName=pets[i].name;activePath=path;activeStage=pets[i].stage;energyMax=pets[i].energy+game.energyBonus;baseWeight=pets[i].baseWeight;activeAttack=pets[i].attack;state.weight=std::max(state.weight,baseWeight);state.energy=std::min(state.energy,energyMax);return true;}
 if(!sdReady)return false;String file="/digipet/"+path;file.replace(".json",".bin");File f=SD.open(file);if(!f)return false;uint8_t header[10];if(f.read(header,10)!=10||memcmp(header,"DGP1",4)){f.close();return false;}int w=header[4]|header[5]<<8,h=header[6]|header[7]<<8,n=header[8]|header[9]<<8;if(w<1||h<1||w*h>4096||n<1||n>64){f.close();return false;}if(f.size()!=10+n*(32+w*h*2)){f.close();return false;}std::vector<uint16_t> nextPixels(w*h*n);std::vector<String> nextLabels;for(int i=0;i<n;i++){char label[33]={};if(f.read((uint8_t*)label,32)!=32||f.read((uint8_t*)&nextPixels[i*w*h],w*h*2)!=(size_t)(w*h*2)){f.close();return false;}nextLabels.push_back(label);}f.close();sdPixels=std::move(nextPixels);sdLabels=std::move(nextLabels);sdWidth=w;sdHeight=h;activePath=path;for(auto& e:catalog)if(e.path==path){activeName=e.name;activeStage=e.stage;energyMax=e.energy+game.energyBonus;baseWeight=e.baseWeight;activeAttack=e.attack;state.weight=std::max(state.weight,baseWeight);state.energy=std::min(state.energy,energyMax);break;}return true;}
// One NVS transaction saves the active partner, items, bank and dungeon together.
int32_t lifetimeTraining[11]={};
EvolutionHistory evolutionHistory[11]={};
int32_t rewindItems=0;
FullSave lastPetSave={};bool havePetSave=false,saveBlocked=false;
PetSave capturePet(){PetSave v={};v.version=2;strlcpy(v.path,activePath.c_str(),sizeof(v.path));int32_t a[]={state.hunger,state.strength,state.energy,state.happy,state.poop,state.care,state.training,state.battles,state.wins,state.stageMinutes,state.totalMinutes,state.sleeping?1:0,protein,state.weight};memcpy(v.values,a,sizeof(a));return v;}
FullSave captureFull(){FullSave f={};f.version=5;f.rewindItems=rewindItems;f.pet=capturePet();f.game=game;memcpy(f.lifetimeTraining,lifetimeTraining,sizeof(lifetimeTraining));memcpy(f.history,evolutionHistory,sizeof(evolutionHistory));return f;}
void savePet(){if(saveBlocked)return;FullSave v=captureFull();if(!activePath.length()||(havePetSave&&!memcmp(&v,&lastPetSave,sizeof(v))))return;Preferences p;if(!p.begin("digipet-save",false))return;size_t n=p.putBytes("pet-v1",&v,sizeof(v));p.end();if(n==sizeof(v)){lastPetSave=v;havePetSave=true;}else Serial.println("SAVE FAILED");}
bool validPet(const PetSave&v){if(v.version!=2||!memchr(v.path,0,sizeof(v.path)))return false;for(int x:v.values)if(x<0)return false;return v.values[0]<=4&&v.values[1]<=4&&v.values[3]<=100&&v.values[4]<=8&&v.values[11]<=1&&v.values[8]<=v.values[7];}
void applyPetValues(const PetSave&v){state.hunger=v.values[0];state.strength=v.values[1];state.energy=std::min((int)v.values[2],energyMax);state.happy=v.values[3];state.poop=v.values[4];state.care=v.values[5];state.training=v.values[6];state.battles=v.values[7];state.wins=v.values[8];state.stageMinutes=v.values[9];state.totalMinutes=v.values[10];state.sleeping=v.values[11];protein=v.values[12];state.weight=v.values[13]>0?v.values[13]:baseWeight;}
bool validGame(const GameState&g){if(g.bits<0||g.bits>99999999||g.bankLimit<5||g.bankLimit>10||g.energyBonus<0||g.energyBonus>999||g.evoBlocked<0||g.evoBlocked>1||g.levelPlus<0||g.levelPlus>1||g.fullMinutes<0||g.fullMinutes>720||g.autoCleanMinutes<0||g.autoCleanMinutes>1440||g.dungeonFloor<1||g.dungeonFloor>31||g.dungeonHP<0||g.dungeonHP>10||g.dungeonStatus<0||g.dungeonStatus>2)return false;for(int x:g.inventory)if(x<0||x>9999)return false;for(auto&bank:g.bank){if(!memchr(bank.path,0,sizeof(bank.path)))return false;for(int x:bank.values)if(x<0)return false;if(bank.path[0]){PetSave pet={};pet.version=2;memcpy(pet.path,bank.path,sizeof(pet.path));memcpy(pet.values,bank.values,sizeof(pet.values));if(!validPet(pet)||bank.values[14]>999||bank.values[15]>1||bank.values[16]>1||bank.values[17]>720||bank.values[18]>1440)return false;}}return true;}
bool restorePet(){PetSave v={};std::unique_ptr<FullSave> all(new(std::nothrow) FullSave{});if(!all)return false;Preferences p;if(!p.begin("digipet-save",true))return false;size_t n=p.getBytesLength("pet-v1");bool modern=false;if(n==sizeof(FullSave)||n==offsetof(FullSave,history)){p.getBytes("pet-v1",all.get(),n);modern=validGame(all->game)&&(n==sizeof(FullSave)?all->version==5:all->version==4);for(auto& history:all->history){if(history.count>16){modern=false;break;}for(int i=0;i<history.count;i++)if(history.routes[i]>=ROUTE_COUNT)modern=false;}for(int x:all->lifetimeTraining)if(x<0||x>1000000000)modern=false;if(all->rewindItems<0||all->rewindItems>9999)modern=false;if(modern)v=all->pet;}else if(n==sizeof(FullSaveV3)){p.getBytes("pet-v1",all.get(),n);if(all->version==3&&validGame(all->game)){modern=true;v=all->pet;all->lifetimeTraining[0]=v.values[6];for(int i=0;i<10;i++)all->lifetimeTraining[i+1]=all->game.bank[i].values[6];}}else if(n==sizeof(v))p.getBytes("pet-v1",&v,sizeof(v));else if(n==sizeof(PetSaveV1)){PetSaveV1 old={};p.getBytes("pet-v1",&old,sizeof(old));if(old.version==1){v.version=2;memcpy(v.path,old.path,sizeof(v.path));memcpy(v.values,old.values,sizeof(old.values));}}p.end();if(!validPet(v))return false;if(modern){game=all->game;rewindItems=all->rewindItems;memcpy(evolutionHistory,all->history,sizeof(evolutionHistory));memcpy(lifetimeTraining,all->lifetimeTraining,sizeof(lifetimeTraining));}else{game=GameState();rewindItems=0;for(auto& history:evolutionHistory)history={};memset(lifetimeTraining,0,sizeof(lifetimeTraining));lifetimeTraining[0]=v.values[6];}if(!selectPet(v.path)){saveBlocked=true;Serial.println("Saved pet assets unavailable; keeping original save");return false;}applyPetValues(v);lastPetSave=captureFull();havePetSave=true;return true;}
std::vector<uint16_t> previewPixels;std::vector<String> previewLabels;int previewPet=0,previewW=0,previewH=0,previewAttack=1,previewEnergy=30,previewAttribute=0,previewBaseWeight=5,previewStage=3;bool previewValid=false;
void refreshPreview(){auto livePixels=std::move(sdPixels);auto liveLabels=std::move(sdLabels);int oldPet=petIndex,oldW=sdWidth,oldH=sdHeight,oldMax=energyMax,oldEnergy=state.energy,oldBaseWeight=baseWeight,oldWeight=state.weight,oldAttack=activeAttack;String name=activeName,path=activePath,stage=activeStage;previewPath=catalog[galleryIndex].path;previewValid=selectPet(previewPath);if(previewValid){previewPixels=std::move(sdPixels);previewLabels=std::move(sdLabels);previewPet=petIndex;previewAttack=activeAttack;previewEnergy=energyMax-game.energyBonus;previewAttribute=battleAttribute(activePath.c_str());previewBaseWeight=baseWeight;previewStage=battleStage(activeStage.c_str());previewW=sdWidth;previewH=sdHeight;}sdPixels=std::move(livePixels);sdLabels=std::move(liveLabels);petIndex=oldPet;sdWidth=oldW;sdHeight=oldH;energyMax=oldMax;baseWeight=oldBaseWeight;activeAttack=oldAttack;state.weight=oldWeight;state.energy=oldEnergy;activeName=name;activePath=path;activeStage=stage;}
bool prepareOpponent(String wanted="",bool excludeCurrent=false){
 if(!wanted.length())wanted=game.levelPlus?String("Ultimate-Super Ultimate"):activeStage;
 std::vector<int> pool;
 for(unsigned i=0;i<catalog.size();i++)if(catalog[i].stage==wanted&&(!excludeCurrent||catalog[i].path!=activePath))pool.push_back(i);
 int oldIndex=galleryIndex;
 while(!pool.empty()){
  int pick=random(pool.size());galleryIndex=pool[pick];pool[pick]=pool.back();pool.pop_back();refreshPreview();
  if(previewValid){Serial.printf("RIVAL %s %s\n",previewPath.c_str(),wanted.c_str());galleryIndex=oldIndex;return true;}
 }
 galleryIndex=oldIndex;return false;
}
void evolve(){if(uiMode==MATCH_INFO||pendingAction||uiMode==BATTLE_TIMING||uiMode==TIMING||uiMode==SHOT_TRAIN||game.evoBlocked||game.levelPlus||activeStage=="Digitama")return;int duration=stageDuration(activeStage.c_str());if(!duration||state.stageMinutes<duration)return;for(int i=0;i<ROUTE_COUNT;i++)if(activePath==evoRoutes[i].from){auto&r=evoRoutes[i];int q=routeQuality(activeStage.c_str(),state);const char* chosen=selectRoute(r.best,r.middle,r.worst,q);int bonus=std::min(999,game.energyBonus+state.training/5);if(chosen&&selectPet(chosen)){auto& history=evolutionHistory[0];if(history.count==16){memmove(history.routes,history.routes+1,15*sizeof(uint16_t));history.count--;}history.routes[history.count++]=i;energyMax+=bonus-game.energyBonus;game.energyBonus=bonus;evolutionMissing=false;state.stageMinutes=0;state.training=0;state.care=0;say("DIGIVOLVE! "+activeName,"Evolve");actionUntil=actionStarted+5000;uiMode=ANIMATION;returnMode=HOME;outcome="EVOLVE!";report();return;}evolutionMissing=true;say("Evolution needs SD assets","Refuse");return;}}
void advance(int count){if(activeStage=="Digitama")return;for(int i=0;i<count;i++){if(game.fullMinutes>0)state.hunger=state.strength=4;if(game.autoCleanMinutes>0)state.poop=0;minuteTick(state,energyMax);if(game.fullMinutes>0){state.hunger=state.strength=4;game.fullMinutes--;}if(game.autoCleanMinutes>0){state.poop=0;game.autoCleanMinutes--;}evolve();}}
void startFeed(int food);void startWash();
void pressKey(char key,bool hold);
void doAction(String a){if(state.sleeping&&a!="sleep"){say("Sleeping... tap WAKE","Sleep");return;}if(a=="feed"){startFeed(0);}else if(a=="train"){if(state.energy<1){say("Need rest!");return;}state.energy--;int gain=std::min(999-state.training,trainingHitType==2?2:1);state.training+=gain;lifetimeTraining[0]=std::min(1000000000,lifetimeTraining[0]+gain);state.strength=std::min(4,state.strength+1);if(state.weight>baseWeight)state.weight--;say("Getting stronger!","Attack");}else if(a=="clean"){startWash();}else if(a=="sleep"){state.sleeping=!state.sleeping;say(state.sleeping?"Good night!":"Good morning!",state.sleeping?"Sleep":"Happy");}else if(a=="pet"){state.happy=std::min(100,state.happy+10);say("Happy to be with you!");}else if(a=="battle"){uiMode=BATTLE;pressKey('B',false);}report();}
void pixel(int x,int y,uint16_t color){if(x>=0&&x<32&&y>=0&&y<16)canvas.fillRect(16+x*7,64+y*7,petStyle?7:6,petStyle?7:6,color);}
void careBitmap(const uint8_t* data,int w,int h,int x,int y,bool flip=false){for(int yy=0;yy<h;yy++)for(int xx=0;xx<w;xx++)if(data[yy*w+(flip?w-1-xx:xx)])pixel(x+xx,y+yy,ink);}
void poopSprites(int wave=-999){for(int i=0;i<std::min(2,state.poop);i++)if(wave<=8-i*8)careBitmap(care_poop[0],8,8,24,8-i*8,frameNo%2);}
void projectile(int x,int y,int attackId=0,bool flip=false){int index=std::max(0,std::min(99,(attackId?attackId:activeAttack)-1));for(int yy=0;yy<8;yy++)for(int xx=0;xx<8;xx++){uint16_t c=care_attacks[index][yy*8+(flip?7-xx:xx)];if(c==0xF81F)continue;if(!colorMode){int r=((c>>11)&31)*255/31,g=((c>>5)&63)*255/63,b=(c&31)*255/31;if((r*299+g*587+b*114)/1000>150)continue;c=ink;}pixel(x+xx,y+yy,c);}}
void eraseRoomPixel(int x,int y){int px=16+x*7,py=64+y*7,size=petStyle?7:6;if(backgroundIndex<0){canvas.fillRect(px,py,size,size,petStyle?bg:bg-0x0821);return;}for(int yy=0;yy<size;yy++)for(int xx=0;xx<size;xx++)canvas.drawPixel(px+xx,py+yy,screenBackgrounds[backgroundIndex][((py+yy)/4)*64+(px+xx)/4]);}
// Compact 4px menu glyphs; pet sprites retain the original room pixel scale.
void roomText(String text,int x,int y,int width=32,bool centered=false){text.toUpperCase();int span=width*7,len=text.length()*16-4,offset=len>span?std::max(0,(int)((millis()/240)%(text.length()*4+span/4))*4-span):0;
 int origin=16+x*7+(centered?std::max(0,(span-len)/2):0),top=64+y*7;
 for(unsigned i=0;i<text.length();i++){const uint8_t*glyph=nullptr;for(int j=0;j<GLYPH_COUNT;j++)if(pixelFont[j].c==text[i]){glyph=pixelFont[j].pixels;break;}
 for(int yy=0;yy<5;yy++)for(int xx=0;xx<3;xx++){bool on=glyph&&glyph[yy*3+xx];if(text[i]==':')on=xx==1&&(yy==1||yy==3);int px=origin+i*16+xx*4-offset;if(on&&px>=16+x*7&&px+4<=16+(x+width)*7)canvas.fillRect(px,top+yy*4,4,4,ink);}}
}
void centerText(String text,int y){roomText(text,0,y,32,true);}
void petSprite(int x,int y,String wanted="Walk",bool flip=false,int frameIndex=-1){const auto&p=pets[petIndex];int w=sdPixels.empty()?p.width:sdWidth,h=sdPixels.empty()?p.height:sdHeight,n=sdPixels.empty()?p.count:sdLabels.size();if(drawDimSprite(drawingPreview?previewPath:activePath,std::max(16,std::min(128,16+x*7)),64,112,112,wanted,flip,frameIndex))return;std::vector<int> options;for(int i=0;i<n;i++){String label=sdPixels.empty()?String(p.labels[i]):sdLabels[i];if(label.startsWith(wanted))options.push_back(i);}if(options.empty())for(int i=0;i<n;i++){String label=sdPixels.empty()?String(p.labels[i]):sdLabels[i];if(label.startsWith("Walk"))options.push_back(i);}int currentFrame=frameIndex<0?frameNo:frameIndex;int f=options.empty()?currentFrame%n:options[currentFrame%options.size()];auto data=sdPixels.empty()?p.frames:sdPixels.data();for(int yy=0;yy<h;yy++)for(int xx=0;xx<w;xx++){uint16_t c=data[f*w*h+yy*w+xx];if(c==0xF81F)continue;if(!colorMode){int r=((c>>11)&31)*255/31,g=((c>>5)&63)*255/63,b=(c&31)*255/31;if((r*299+g*587+b*114)/1000>150)continue;c=ink;}pixel(x+(flip?w-1-xx:xx),y+yy,c);}}
void previewSprite(int x,int y,String kind="Walk",bool flip=false,int frameIndex=-1){if(!previewValid)return;int old=petIndex,w=sdWidth,h=sdHeight;auto live=std::move(sdPixels);auto labels=std::move(sdLabels);sdPixels=std::move(previewPixels);sdLabels=std::move(previewLabels);petIndex=previewPet;sdWidth=previewW;sdHeight=previewH;drawingPreview=true;petSprite(x,y,kind,flip,frameIndex);drawingPreview=false;previewPixels=std::move(sdPixels);previewLabels=std::move(sdLabels);sdPixels=std::move(live);sdLabels=std::move(labels);petIndex=old;sdWidth=w;sdHeight=h;}
void hearts(int value){const uint8_t h[]={0,1,0,1,0,1,1,1,1,1,1,1,1,1,1,0,1,1,1,0,0,0,1,0,0};for(int i=0;i<4;i++)for(int y=0;y<5;y++)for(int x=0;x<5;x++)if(h[y*5+x]&&(i<value||y<2||x==0||x==4||y==4))pixel(2+i*7+x,9+y,i<value?ink:ghost);}
void uiAnimate(String animation,String text,UiMode back=HOME,uint32_t duration=0){if(!duration)duration=animation=="Happy"?2376:animation=="Angry"?3200:animation=="Refuse"?4000:1800;pendingAction=0;action=animation;outcome=text;actionStarted=millis();actionUntil=actionStarted+duration;returnMode=back;uiMode=ANIMATION;}
Fighter combatPlayer(){Fighter own;own.stage=std::max(battleStage(activeStage.c_str()),game.levelPlus?6:0);own.attribute=battleAttribute(activePath.c_str());own.baseWeight=baseWeight;own.training=state.training;own.total=lifetimeTraining[0];own.age=state.totalMinutes/1440;own.battles=state.battles;own.strength=state.strength;own.hunger=state.hunger;own.weight=state.weight;return own;}
void startPreparedBattle(){if(battleLog.dungeon){uiAnimate("BattleStart","",PLAY,2700);pendingAction=12;}else{uiAnimate("Battle","",BATTLE,battleLog.duration);pendingAction=8;}}
void startBattleFight(int type,bool dungeon=false){
 Fighter own=combatPlayer();Fighter rival;rival.stage=dungeon&&game.dungeonFloor<=20?6:previewStage;rival.attribute=previewAttribute;rival.baseWeight=previewBaseWeight;
 double enemyAccuracy=dungeon?std::min(1.0,.5+(game.dungeonFloor-1)*.05):-1;
 if(dungeon){int hp=game.dungeonFloor==31?10:game.dungeonFloor%10==0?5:3;fight(battleLog,own,rival,type,2,esp_random(),false,game.dungeonHP,hp,20,enemyAccuracy);battleLog.dungeon=true;}
 else{int r=random(1000000),enemyType=r>800000?2:r>300000?1:0;fight(battleLog,own,rival,type,enemyType,esp_random());}
 startPreparedBattle();
}
void startFeed(int food){if(state.poop||(!food&&state.hunger>=4)||(food&&state.strength>=4&&state.energy>=energyMax)){uiAnimate("Refuse",state.poop?"CLEAN FIRST":"FULL",FEED);return;}if(state.sleeping){uiAnimate("Sleep","SLEEPING",FEED);return;}uiAnimate("Eat","YUM!",FEED,CARE_EAT_MS);actionFood=food;pendingAction=1;}
void startTrainingResult(int hit){bool win=hit!=0;trainingHitType=hit;uiAnimate(win?"TrainHit":"TrainMiss",hit==2?"MEGA!":win?"GOOD!":"MISS!",TRAIN,win?5500:1500);pendingAction=win?3:4;}
void resolveShot(int lane){if(shotFeedback)return;shotLane=lane;shotGuardY=shotShieldCenter(millis()-shotStarted,shotGuard);shotSuccess=lane>=0&&!shotShieldBlocks(lane,shotGuardY);if(shotSuccess)shotHits++;shotFeedback=true;shotStarted=millis();}
void updateShotTraining(){if(uiMode!=SHOT_TRAIN)return;if(!shotFeedback){if(millis()-shotStarted>=10000)resolveShot(-1);return;}if(millis()-shotStarted<1400)return;
 if(++shotRound>=5){trainingHitType=shotHits==5?2:shotHits>=3?1:0;uiAnimate("ShotResult",String(shotHits)+"/5",TRAIN,1600);pendingAction=trainingHitType?3:4;return;}
 shotFeedback=false;shotGuard=random(2);shotStarted=millis();}
void startWash(){if(!state.poop){uiAnimate("Refuse","NO POOP",CARE);return;}uiAnimate("Wash","CLEAN!",HOME,CARE_WASH_MS);pendingAction=2;}
#include "play_logic.h"
#include "combat_view.h"
void finishUiAction(){if(uiMode!=ANIMATION||(int32_t)(millis()-actionUntil)<0)return;int effect=pendingAction;pendingAction=0;if(effect==9||effect==10){uiMode=effect==10?DUNGEON_TIMING:BATTLE_TIMING;timingStart=millis();return;}
 if(effect==12){uiAnimate("Battle","",PLAY,battleLog.duration);pendingAction=11;return;}
 if(effect==11){const auto& final=battleLog.steps[battleLog.stepCount-1];bool win=final.own>0&&final.rival<=0;dungeonAfterHP=final.own;state.battles=std::min(999,state.battles+1);if(win)state.wins=std::min(999,state.wins+1);settleDungeon(win);uiAnimate(win?"Happy":"Angry",win?"FLOOR CLEAR":"DUNGEON LOST",PLAY);pendingAction=13;report();return;}
 if(effect==13){uiMode=PLAY;return;}if(effect==8){state.battles=std::min(999,state.battles+1);if(battleLog.result>0){state.wins=std::min(999,state.wins+1);int gain=std::min(2,std::max(0,999-state.training));state.training+=gain;lifetimeTraining[0]=std::min(1000000000,lifetimeTraining[0]+gain);}game.bits=std::min(99999999,game.bits+(battleLog.result>0?40:20));uiAnimate(battleLog.result>0?"Happy":battleLog.result<0?"Angry":"Injured",battleLog.result>0?"WIN!":battleLog.result<0?"LOSE!":"DRAW",BATTLE);report();return;}if(effect==5){hatchEgg();return;}if(effect==1){if(!actionFood){state.hunger=std::min(4,state.hunger+1);state.weight++;}else{state.strength=std::min(4,state.strength+1);protein++;state.weight+=5;state.energy=std::min(energyMax,state.energy+7);}if(!actionFood&&state.hunger==4)playSound("e07");report();}if(effect==2){state.poop=0;uiAnimate("Happy","CLEAN!");report();}else if(effect==3||effect==4){if(effect==3){int gain=std::min(999-state.training,trainingHitType==2?2:1);state.training+=gain;lifetimeTraining[0]=std::min(1000000000,lifetimeTraining[0]+gain);state.strength=std::min(4,state.strength+1);if(state.weight>baseWeight)state.weight--;}uiAnimate(effect==3?"Happy":"Angry",effect==3?"GOOD!":"MISS!",TRAIN);report();}else uiMode=returnMode;if(effect)evolve();}

// Notification timers use wall time, independently of accelerated pet growth.
int alertLevel=1;bool careAlerts=true,keySounds=true;
uint32_t ringStarted=0,alertWakeUntil=0;int ringStep=-1;bool alertAutoWoke=false;
uint32_t noticeLast=0,noticeMuteUntil=0,missNoticeUntil=0;int noticeMask=0,noticeCare=-1;String noticePet;
void stopCareRing(){ringStep=-1;M5.Speaker.stop(2);}
int gentleVolume(int level){const int levels[]={0,14,22,32,45};return levels[std::max(0,std::min(4,level))];}
void previewSound(int level){M5.Speaker.stop();bool ready=M5.Speaker.begin();int volume=gentleVolume(level);M5.Speaker.setVolume(volume);bool accepted=level&&ready?M5.Speaker.tone(880,300):false;Serial.printf("AUDIO preview level=%d volume=%d initialized=%s accepted=%s\n",level,volume,ready?"yes":"no",accepted?"yes":"no");}
void updateCareRing(){if(ringStep==-1)return;uint32_t elapsed=millis()-ringStarted;int step=elapsed/240;if(step>=8){stopCareRing();return;}if(step!=ringStep){ringStep=step;const int notes[]={784,988,1175,0,784,988,1047,0};if(notes[step]){M5.Speaker.setVolume(gentleVolume(alertLevel));M5.Speaker.tone(notes[step],140,2,true);}}}
void beginCareRing(){if(screenOff){screenOff=false;alertAutoWoke=true;}alertWakeUntil=millis()+30000;M5.Display.setBrightness((brightness+1)*63);
 if(alertLevel){ringStarted=millis();ringStep=-2;updateCareRing();}}
int careNoticeSeconds(){int minutes=60-state.totalMinutes%60;return std::max(0,(minutes*60000-(int)(millis()-lastMinute))/std::max(1,timeSpeed)/1000);}
void updateCareNotice(){
 uint32_t now=millis();if(noticePet!=activePath){noticePet=activePath;noticeCare=state.care;noticeMask=0;missNoticeUntil=0;}
 if(noticeCare>=0&&state.care>noticeCare)missNoticeUntil=now+60000;noticeCare=state.care;
 int mask=0;if(activeStage!="Digitama"&&!state.sleeping){if(state.hunger<=1)mask|=1;if(state.poop)mask|=2;
 if((state.hunger<=1||state.poop>=7)&&careNoticeSeconds()<=300/std::max(1,timeSpeed))mask|=4;if((state.hunger<=1||state.poop>=7)&&careNoticeSeconds()<=60/std::max(1,timeSpeed))mask|=16;
 if(missNoticeUntil&&(int32_t)(missNoticeUntil-now)>0)mask|=8;}
 if(!mask){noticeMask=0;noticeMuteUntil=0;noticeLast=0;if(ringStep!=-1)stopCareRing();return;}
 bool changed=(mask&~noticeMask)!=0;bool due=!noticeLast||now-noticeLast>=300000;
 bool quiet=uiMode==MATCH_INFO||pendingAction||uiMode==TIMING||uiMode==DUNGEON_TIMING||uiMode==BATTLE_TIMING||uiMode==SHOT_TRAIN||uiMode==ANIMATION;
 if(!quiet&&careAlerts&&(changed||(int32_t)(now-noticeMuteUntil)>=0)&&(changed||due)){
  beginCareRing();
  noticeLast=now;noticeMask=mask;
 }
}
String careNoticeText(){if(!careAlerts||state.sleeping||activeStage=="Digitama")return "";
 if(missNoticeUntil&&(int32_t)(missNoticeUntil-millis())>0)return "CARE MISS +1  TOTAL "+String(state.care);
 if((state.hunger<=1||state.poop>=7)&&careNoticeSeconds()<=300/std::max(1,timeSpeed))return "CARE DUE IN "+String(careNoticeSeconds())+"S";
 if(state.hunger<=1&&state.poop)return "FOOD + CLEAN";
 if(state.hunger<=1)return state.hunger?"FOOD LOW":"NEED FOOD";
 if(state.poop)return "NEED CLEAN";return "";}
String lastAudioAction;uint32_t lastAudioStarted=0;int audioMilestone=-1,lastAudioBattleStep=-1;uint32_t lastShotAudio=0;bool shotImpactPlayed=false;
void updateActionAudio(){
 if(uiMode==SHOT_TRAIN){if(shotFeedback&&lastShotAudio!=shotStarted){lastShotAudio=shotStarted;shotImpactPlayed=false;if(shotLane>=0)playSound("e05");}if(shotFeedback&&millis()-shotStarted>=650&&!shotImpactPlayed){shotImpactPlayed=true;if(shotSuccess)playSound("e06");}return;}
 if(uiMode!=ANIMATION)return;
 if(lastAudioStarted!=actionStarted||lastAudioAction!=action){lastAudioStarted=actionStarted;lastAudioAction=action;audioMilestone=-1;lastAudioBattleStep=-1;
  if(action=="BattleIntro"){if(pendingAction==10)playSound("e04");else playSound("e22",0,-1,3);}
  else if(action=="BattleStart")playSound("e22",0,-1,3);
  else if(action=="Happy")playSound("e07");else if(action=="Angry"||action=="Refuse"||action=="TrainMiss"||action=="Injured")playSound("e08");
  else if(action=="TrainHit")playSound("e05");else if(action=="Evolve")playSound("e09");else if(action=="Jogress")playSound("e11",0,-1,3);else if(action=="Item")playSound("e21");
 }
 uint32_t elapsed=millis()-actionStarted;
 if(action=="Battle"){uint32_t tick;const auto&scene=battleScene(battleLog,elapsed,tick);int step=&scene-battleLog.steps;if(step!=lastAudioBattleStep){lastAudioBattleStep=step;int f=scene.frame;BattleRound empty={};const auto&r=scene.index<battleLog.count?battleLog.rounds[scene.index]:empty;
  if(f==41||f==45)playSound("e11");else if(f==1||f==11)playSound("e05");else if((f==5&&r.hitA)||(f==15&&r.hitB))playSound("e06");}return;}
 if(action=="TrainHit"&&elapsed>=3000&&audioMilestone<0){audioMilestone=0;playSound("e06");}
 if(action=="Evolve"&&elapsed>=2500&&audioMilestone<0){audioMilestone=0;playSound("e11",0,-1,2);}
 if(action=="Egg"&&elapsed>=10000&&audioMilestone<0){audioMilestone=0;playSound("e22");}
}
void saveSettings(){Preferences p;p.begin("digipocket",false);p.putInt("sound",soundLevel);p.putInt("soundver",2);p.putInt("alertvol",alertLevel);p.putBool("alerts",careAlerts);p.putBool("keysound",keySounds);p.putInt("bright",brightness);p.putInt("color",colorMode);p.putInt("theme",theme);p.putInt("speed",timeSpeed);p.putInt("timeout",screenTimeout);p.putInt("bg",backgroundIndex);p.putInt("style",petStyle);p.end();M5.Speaker.setVolume(gentleVolume(soundLevel));M5.Display.setBrightness((brightness+1)*63);}
void pressKey(char key,bool hold=false){lastInput=millis();if(ringStep!=-1)stopCareRing();alertAutoWoke=false;alertWakeUntil=0;if(noticeMask&&noticeLast)noticeMuteUntil=millis()+600000;if(screenOff){screenOff=false;M5.Display.setBrightness((brightness+1)*63);return;}if(key=='C'&&hold){screenOff=true;M5.Display.setBrightness(0);return;}if(keySounds&&soundLevel){M5.Speaker.setVolume(gentleVolume(soundLevel));playSound("sys0",1);}if(uiMode==ANIMATION){if(key=='C'&&!pendingAction)uiMode=HOME;return;}if(uiMode==PLAY){pressPlay(key,hold);return;}if(uiMode==COMBAT_STATS){if(key=='C')openPlay();else if(key=='A'||key=='B')combatPage=(combatPage+1)%4;return;}if(uiMode==MATCH_INFO){if(key=='A')combatPage=(combatPage+1)%3;else if(key=='B')startPreparedBattle();else if(key=='C'){if(battleLog.dungeon){dungeonSettlementPending=false;openPlay(P_DUNGEON);}else uiMode=BATTLE;}return;}if(key=='C'){if(uiMode==DUNGEON_TIMING){openPlay(P_DUNGEON);return;}if(uiMode==SHOT_TRAIN||uiMode==TIMING){uiMode=TRAIN;return;}if(uiMode==HOME){if(cursor>=0)cursor=-1;else uiMode=CLOCK;}else if(uiMode==GALLERY_INFO)uiMode=GALLERY;else{uiMode=HOME;cursor=-1;}return;}
 if(uiMode==HOME){if(key=='A'&&hold){openPlay();return;}if(key=='A')cursor=(cursor+1)%8;if(key=='B'&&cursor>=0){UiMode modes[]={UI_STATUS,FEED,TRAIN,BATTLE,CARE,GALLERY,EVO,SETTINGS};uiMode=modes[cursor];page=0;choice=0;if(uiMode==GALLERY){galleryIndex=0;refreshPreview();}}return;}
 if(uiMode==UI_STATUS){if(key=='A'||key=='B')page=(page+1)%7;return;}
 if(uiMode==FEED){if(key=='A')choice^=1;if(key=='B'){if(state.sleeping){uiAnimate("Sleep","SLEEPING",FEED);return;}startFeed(choice);}return;}
 if(uiMode==SHOT_TRAIN){if(!shotFeedback&&(key=='A'||key=='B'))resolveShot(key=='A'?0:1);return;}
 if(uiMode==TRAIN){if(key=='A')trainMode^=1;if(key=='B'){if(activeStage=="Digitama"){uiAnimate("Refuse","TOO YOUNG",TRAIN);return;}if(state.sleeping||state.energy<2){uiAnimate("Sleep","NEED REST");return;}if(!state.hunger||state.poop){uiAnimate("Refuse",state.poop?"CLEAN FIRST":"NEED FOOD",TRAIN);return;}state.energy=std::max(0,state.energy-2);state.weight=std::max(1,state.weight-2);if(trainMode){uiMode=SHOT_TRAIN;shotRound=shotHits=0;shotGuard=random(2);shotFeedback=false;shotStarted=millis();}else{uiMode=TIMING;timingStart=millis();}}return;}
 if(uiMode==DUNGEON_TIMING){if(key=='B')dungeonResult(trainingHit(trainingPosition(millis()-timingStart),state,energyMax));return;}
 if(uiMode==BATTLE_TIMING){if(key=='B')startBattleFight(trainingHit(trainingPosition(millis()-timingStart),state,energyMax));return;}
 if(uiMode==TIMING){if(key=='B'){int hit=trainingHit(trainingPosition(millis()-timingStart),state,energyMax);startTrainingResult(hit);}return;}
 if(uiMode==BATTLE){if(key=='B'){if(activeStage=="Digitama"){uiAnimate("Refuse","TOO YOUNG");return;}if(state.sleeping||!state.energy){uiAnimate("Sleep","NEED REST");return;}if(!state.hunger||state.poop){uiAnimate("Refuse",state.poop?"CLEAN FIRST":"NEED FOOD",BATTLE);return;}if(!prepareOpponent()){uiAnimate("Refuse","NO RIVAL",BATTLE);return;}state.energy=std::max(0,state.energy-5);state.weight=std::max(1,state.weight-4);uiAnimate("BattleIntro","START",BATTLE,3000);pendingAction=9;}return;}
 if(uiMode==CARE){if(key=='A')choice=(choice+1)%3;if(key=='B'){if(choice==2){openPlay(P_EGGS);return;}if(!choice){state.sleeping=!state.sleeping;uiAnimate(state.sleeping?"Sleep":"Happy",state.sleeping?"GOOD NIGHT":"MORNING!");}else startWash();}return;}
 if(uiMode==GALLERY){if(key=='A'){galleryIndex=(galleryIndex+(hold?50:1))%catalog.size();refreshPreview();}if(key=='B')uiMode=GALLERY_INFO;return;}
 if(uiMode==EVO){if(key=='B'&&page==5){openPlay();return;}if(key=='A'||key=='B')page=(page+1)%6;return;}
 if(uiMode==CLOCK){if(key=='A'){uiMode=SETTINGS;page=0;}return;}
 if(uiMode==POWER_CONFIRM){if(key=='A')choice^=1;if(key=='B'){sleepRequested=choice==1;uiMode=HOME;cursor=-1;}return;}
 if(uiMode==SETTINGS){if(key=='A')page=(page+1)%16;if(key=='B'){if(page==0)soundLevel=(soundLevel+1)%5;if(page==1)brightness=(brightness+1)%4;if(page==2)colorMode^=1;if(page==3)theme=(theme+1)%3;if(page==4)timeSpeed=timeSpeed==1?60:1;if(page==5)advance(60);if(page==6)uiMode=CLOCK;if(page==7)screenTimeout=(screenTimeout+1)%4;if(page==9)backgroundIndex=(backgroundIndex+2)%(BACKGROUND_COUNT+1)-1;if(page==10)petStyle=(petStyle+1)%3;if(page==11){uiMode=POWER_CONFIRM;choice=0;}if(page==12)openPlay();if(page==13)careAlerts=!careAlerts;if(page==14)alertLevel=(alertLevel+1)%5;if(page==15)keySounds=!keySounds;saveSettings();if(page==0)previewSound(soundLevel);if(page==14)previewSound(alertLevel);}return;}}
#include "battle_render.h"
// Draw the shooting exercise in fixed, separate screen regions, including 32px pets.
void renderShotTraining(){
 canvas.fillRect(16,64,224,112,bg);canvas.setFont(&fonts::Font0);canvas.setTextDatum(top_left);canvas.setTextSize(1);canvas.setTextColor(ink);
 canvas.drawString("ROUND "+String(shotRound+1)+"/5   HITS "+String(shotHits),20,67);
 int w=sdPixels.empty()?pets[petIndex].width:sdWidth,h=sdPixels.empty()?pets[petIndex].height:sdHeight;
 int count=sdPixels.empty()?pets[petIndex].count:sdLabels.size(),f=0;String wanted=shotFeedback?"Attack":"Walk";
 for(int i=0;i<count;i++){String label=sdPixels.empty()?String(pets[petIndex].labels[i]):sdLabels[i];if(label.startsWith(wanted)){f=i;break;}}
 const uint16_t*data=sdPixels.empty()?pets[petIndex].frames:sdPixels.data();
 int scale=std::max(1,std::min(64/std::max(1,w),64/std::max(1,h))),left=24+(64-w*scale)/2,top=88+(64-h*scale)/2;
 if(!drawDimSprite(activePath,24,88,64,64,wanted,true,0))for(int y=0;y<h;y++)for(int x=0;x<w;x++){uint16_t c=data[f*w*h+y*w+(w-1-x)];if(c==0xF81F)continue;if(!colorMode){int r=((c>>11)&31)*255/31,g=((c>>5)&63)*255/63,b=(c&31)*255/31;if((r*299+g*587+b*114)/1000>150)continue;c=ink;}canvas.fillRect(left+x*scale,top+y*scale,scale,scale,c);}
 canvas.drawFastHLine(108,116,122,ghost);
 for(int lane=0;lane<2;lane++){int y=lane?128:86;canvas.drawRect(213,y,17,26,ghost);}
 int guardY=shotFeedback?shotGuardY:shotShieldCenter(millis()-shotStarted,shotGuard);canvas.fillRect(213,guardY-9,5,18,ink);
 uint32_t elapsed=millis()-shotStarted;
 if(shotFeedback&&shotLane>=0&&elapsed<650){int x=108+elapsed*80/650,y=shotLane?129:87,index=std::max(0,std::min(99,activeAttack-1));
 for(int yy=0;yy<8;yy++)for(int xx=0;xx<8;xx++){uint16_t c=care_attacks[index][yy*8+xx];if(c!=0xF81F)canvas.fillRect(x+xx*3,y+yy*3,3,3,colorMode?c:ink);}}
 if(shotFeedback&&elapsed>=650){canvas.setTextSize(2);canvas.drawString(shotSuccess?"HIT!":shotLane<0?"TIME":"BLOCK",108,100);canvas.setTextSize(1);}
 canvas.drawString(shotFeedback?"NEXT...":"A:HIGH  B:LOW",108,164);
}
void renderEvolutionDetails(){
 canvas.fillRect(8,60,240,120,bg);canvas.setFont(&fonts::Font0);canvas.setTextDatum(top_left);canvas.setTextSize(2);canvas.setTextColor(ink);
 auto line=[&](String t,int row){if(t.length()>20){int offset=(millis()/450)%(t.length()-20+6);offset=std::max(0,std::min((int)t.length()-20,offset-2));t=t.substring(offset,offset+20);}canvas.drawString(t,8,60+row*20);};
 line(String(page+1)+"/6  "+(page==0?"EVOLUTION":page==1?"PREDICTED":page==2?"BEST":page==3?"MIDDLE":page==4?"WORST":"SPECIAL"),0);
 if(page==5){line("ITEM / DNA ROUTES",2);line("Open special menu",3);return;}
 if(activeStage=="Digitama"){String baby="";for(auto&r:eggRoutes)if(activePath==r.egg){baby=pathName(r.baby);break;}line("Egg -> "+baby,2);line("HATCH: ABOUT 12 SEC",3);return;}
 const char*target=nullptr;bool found=false;int quality=page>=2?page-2:routeQuality(activeStage.c_str(),state);
 for(auto&r:evoRoutes)if(activePath==r.from){found=true;target=selectRoute(r.best,r.middle,r.worst,quality);break;}
 if(!found||!target){line("NO NORMAL EVOLUTION",2);line("SEE SPECIAL ROUTES",3);return;}
 line("To: "+pathName(target),1);
 int duration=stageDuration(activeStage.c_str());line("TIME "+String(state.stageMinutes)+"/"+String(duration)+" MIN",2);
 bool baby=activeStage=="Baby I"||activeStage=="Baby II",child=activeStage=="Child",adult=activeStage=="Adult";
 String careNeed=baby?(quality==0?"0-2":quality==1?"3-5":">=6"):(quality==2?(child||adult?">=4":">=3"):(child||adult?"<=3":"<=2"));
 line("CARE "+String(state.care)+" NEED "+careNeed,3);
 if(!baby&&quality<2){int threshold=child?25:adult?40:activeStage=="Ultimate-Super Ultimate"||activeStage=="Mega"?100:80;
 line(String(child?"TRAIN ":"FIGHTS ")+String(child?state.training:state.battles)+" NEED "+(quality==0?">=":"<")+String(threshold),4);
 }else line("NO TRAIN/FIGHT LIMIT",4);
 bool available=false;for(auto&e:catalog)if(e.path==target){available=true;break;}
 line(game.evoBlocked||game.levelPlus?"EVOLUTION LOCKED":!available?"TARGET NEEDS ASSETS":"TARGET IN CATALOG",5);

}
void render(){if(screenOff){canvas.fillSprite(0x0861);canvas.pushSprite(0,0);return;}uint16_t palette[3][3]={{0xB613,0x2184,0x9D50},{0xC637,0x31A5,0xAD74},{0xD5F0,0x49C4,0xC54D}};bg=palette[theme][0];ink=palette[theme][1];ghost=palette[theme][2];if(backgroundIndex>=0){ink=0x21A6;ghost=0x5B89;}canvas.fillSprite(bg);if(backgroundIndex>=0){for(int y=0;y<60;y++)for(int x=0;x<64;x++)canvas.fillRect(x*4,y*4,4,4,screenBackgrounds[backgroundIndex][y*64+x]);if(uiMode!=HOME&&uiMode!=ANIMATION&&uiMode!=GALLERY)canvas.fillRect(12,60,232,120,bg);}if(backgroundIndex<0&&!petStyle)for(int y=0;y<16;y++)for(int x=0;x<32;x++)canvas.fillRect(16+x*7,64+y*7,6,6,bg-0x0821);
 int width=sdPixels.empty()?pets[petIndex].width:sdWidth,height=sdPixels.empty()?pets[petIndex].height:sdHeight;if(petStyle==2)for(auto& sprite:dimSprites)if(sprite.path==activePath&&sprite.pixels){width=height=16;break;}auto pose=wanderer.update(millis()/CARE_WALK_MS,width,state.sleeping,state.poop>0,uiMode==HOME);
 if(uiMode==HOME){petSprite(pose.x,(16-height)/2,state.sleeping?"Sleep":"Walk",pose.flip);poopSprites();if(state.sleeping)careBitmap(care_sleep[frameNo%2],9,8,23,0);}
 else if(uiMode==UI_STATUS){if(page==0){roomText(String(state.totalMinutes/1440)+"YR",0,2,16);roomText(String(state.weight)+"G",0,9,16);petSprite(16,0);}if(page==1||page==2){centerText(page==1?"HUNGER":"STRENGTH",1);hearts(page==1?state.hunger:state.strength);}if(page==3){centerText("ENERGY",1);centerText(String(state.energy)+"/"+String(energyMax),9);}if(page==4){centerText("WIN "+String(state.battles?state.wins*100/state.battles:0)+"%",1);centerText(String(state.wins)+"/"+String(state.battles),9);}if(page==5){centerText("TRAIN",1);centerText(String(state.training),9);}if(page==6){centerText("CARE MISS",1);centerText(String(state.care),9);}}
 else if(uiMode==FEED){careBitmap(care_meat[0],8,8,15,0);careBitmap(care_protein[0],8,8,15,8);careBitmap(care_arrow[0],4,7,4,choice?9:0);}
 else if(uiMode==CARE&&choice==2){centerText("NEW EGG",1);centerText("B START",9);}
 else if(uiMode==CARE){roomText(choice==0?">":" ",0,1);roomText(choice==1?">":" ",0,9);roomText(state.sleeping?"LIGHT ON":"LIGHT OFF",5,1,27);roomText("TOILET",5,9,27);}
 else if(uiMode==TRAIN){centerText(trainMode?"HIGH LOW":"TRAIN",1);centerText("A MODE B GO",9);}
 else if(uiMode==BATTLE){centerText("BATTLE",1);centerText("B START",9);}
 else if(uiMode==SHOT_TRAIN){renderShotTraining();}
 else if(uiMode==TIMING||uiMode==DUNGEON_TIMING||uiMode==BATTLE_TIMING){roomText("HIT",10,0);for(int x=2;x<30;x++){pixel(x,7,ink);pixel(x,11,ink);}for(int y=7;y<12;y++){pixel(2,y,ink);pixel(29,y,ink);}auto w=trainingWindow(state,energyMax);for(int x=3+w.min;x<=4+w.max;x++){pixel(x,6,ink);pixel(x,12,ink);}int x=3+trainingPosition(millis()-timingStart);for(int y=8;y<11;y++){pixel(x,y,ink);pixel(x+1,y,ink);}}
 else if(uiMode==ANIMATION){
 if(action=="Egg"){uint32_t elapsed=millis()-actionStarted;petSprite(8,0,elapsed>=10000?"hatch":String("egg ")+String((elapsed/800)%2+1),false,0);}
 else if(action=="Evolve"||action=="Jogress"){if(((millis()-actionStarted)/180)%2==0)petSprite(8,0,"Happy",false,0);else for(int y=0;y<16;y++)for(int x=0;x<32;x++)if((x+y)%2==0)pixel(x,y,ink);}
 else if(action=="Item"){if(playItem>=0)careBitmap(playItems[playItem].icon,12,12,10,2);else petSprite(8,0,"Happy");}
 else if(action=="ShotResult"){centerText(outcome,2);centerText(trainingHitType==2?"MEGA!":trainingHitType?"GOOD!":"MISS!",10);}
 else if(action=="TrainHit"||action=="TrainMiss"){uint32_t elapsed=millis()-actionStarted;int step=elapsed/500;float fraction=(elapsed%500)/500.0f;if(action=="TrainMiss")petSprite(2,(16-height)/2,"Injured 1",true,0);else if(step<2){petSprite(2,(16-height)/2,"Attack",true,0);int x=(int)std::floor(20+(step+fraction)*6+0.5f);if(trainingHitType==2){projectile(x,0);projectile(x,8);}else projectile(x,4);}else if(step<6){careBitmap(care_walls[0],8,16,24,0);int x=(int)std::floor(16-(6-step-fraction)*6+0.5f);if(trainingHitType==2){projectile(x,0);projectile(x,8);}else projectile(x,4);}else if(step<9){bool invert=((elapsed%500)/80)%2;for(int y=0;y<16;y++)for(int x=0;x<32;x++)if((bool)care_hit[y*32+x]!=invert)pixel(x,y,ink);}else careBitmap(care_walls[trainingHitType==2?1:0],8,16,24,0);}
 else if(action=="Eat"){int step=(millis()-actionStarted)/CARE_EAT_STEP_MS,bite=step>=7?3:step>=5?2:step>=3?1:0;petSprite(2,(16-height)/2,step%2?"Eat 2":"Eat 1",true,0);if(bite<3)careBitmap(actionFood?care_protein[bite]:care_meat[bite],8,8,20,4);}
 else if(action=="Wash"){int wave=-6+(int)((millis()-actionStarted)/CARE_WASH_STEP_MS);petSprite(pose.x,(16-height)/2,"Walk",pose.flip,0);poopSprites(wave);for(int y=0;y<std::min(16,wave+6);y++)for(int x=0;x<32;x++)eraseRoomPixel(x,y);for(int y=0;y<std::min(16,std::max(0,wave));y++)for(int x=0;x<32;x++)if((x+y)%2==0)pixel(x,y,ink);else eraseRoomPixel(x,y);for(int i=0;i<4;i++)careBitmap(care_water[0],8,6,i*8,wave);}
 else if(action=="Happy"||action=="Angry"||action=="Refuse"){int step=(millis()-actionStarted)/(action=="Happy"?594:800),alternate=step%2;String kind=action=="Happy"?(alternate?"Eat 2":"Happy"):action=="Angry"?(alternate?"Injured 1":"Angry"):"Refuse";petSprite(8,(16-height)/2,kind,action=="Refuse"&&alternate,0);if(!alternate)careBitmap(action=="Happy"?care_happy[0]:care_angry[0],10,action=="Happy"?8:10,22,2);}
 else if(action=="BattleIntro"||action=="BattleStart"){for(int y=0;y<16;y++)for(int x=0;x<32;x++)if(battleIntroPixel(x,y,millis()-actionStarted))pixel(x,y,ink);}
 else if(action=="Battle"){renderBattle(width,height);}
 else if(actionUntil-millis()<650)centerText(outcome,5);else petSprite(8,0,action);}
 else if(uiMode==GALLERY_INFO){centerText(catalog[galleryIndex].stage,1);centerText("READONLY",9);}
 else if(uiMode==GALLERY){roomText(catalog[galleryIndex].name,0,1,16);roomText("VIEW "+String(galleryIndex+1),0,10,16);if(previewValid)previewSprite(16,0);}
 else if(uiMode==PLAY){renderExtendedPlay();}
 else if(uiMode==COMBAT_STATS||uiMode==MATCH_INFO){renderCombat();}
 else if(uiMode==EVO){renderEvolutionDetails();}
 else if(uiMode==POWER_CONFIRM){centerText("SLEEP?",1);centerText(choice?" NO>YES":">NO YES",9);}
 else if(uiMode==SETTINGS){const char* titles[]={"FX VOLUME","BRIGHT","COLOR","THEME","SPEED","TIME","CLOCK","AUTO OFF","ABOUT","BACKGND","STYLE","POWER","EXTRA MENU","CARE ALERT","ALERT VOL","KEY SOUND"};centerText(titles[page],1);String value;const char* sounds[]={"OFF","TINY","LOW","MED","HIGH"};const char* themes[]={"GREEN","GRAY","AMBER"};const char* sleeps[]={"NEVER","30SEC","1MIN","5MIN"};if(page==0)value=sounds[soundLevel];if(page==1)value=String((brightness+1)*25)+"%";if(page==2)value=colorMode?"ON":"OFF";if(page==3)value=themes[theme];if(page==4)value=String(timeSpeed)+"X";if(page==5)value="+1 HOUR";if(page==6)value="B VIEW";if(page==7)value=sleeps[screenTimeout];if(page==8)value="V2.10.0";if(page==9)value=backgroundIndex<0?"LCD":"BG "+String(backgroundIndex+1);if(page==10)value=petStyle==2?"DIM COLOR":petStyle?"CLEAR":"RETRO";if(page==11)value="LIGHT";if(page==12)value="B OPEN";if(page==13)value=careAlerts?"ON":"OFF";if(page==14)value=sounds[alertLevel];if(page==15)value=keySounds?"ON":"OFF";centerText(value,9);}
 else if(uiMode==CLOCK){m5::rtc_time_t t;M5.Rtc.getTime(&t);char str[6];snprintf(str,sizeof(str),"%02d:%02d",t.hours,t.minutes);centerText(str,2);centerText("A SET",10);}
 if(uiMode!=PLAY&&uiMode!=COMBAT_STATS&&uiMode!=MATCH_INFO)for(int i=0;i<8;i++){int x=20+i%4*60,y=i<4?23:185;uint16_t c=cursor==i?ink:ghost;for(int yy=0;yy<16;yy++)for(int xx=0;xx<16;xx++)if(menuIcons[i][yy*16+xx])canvas.fillRect(x+xx*2,y+yy*2,2,2,c);if(cursor==i)canvas.fillRect(x+7,y+35,18,2,ink);}
 if(uiMode==HOME||alertWakeUntil&&(int32_t)(alertWakeUntil-millis())>0){String notice=careNoticeText();if(notice.length()){canvas.setFont(&fonts::Font0);canvas.setTextSize(1);canvas.setTextDatum(top_left);canvas.setTextColor(ink);canvas.drawString(notice,16,226);}else if(uiMode==HOME){canvas.setFont(&fonts::Font0);canvas.setTextSize(1);canvas.setTextDatum(top_left);canvas.setTextColor(ink);canvas.drawString("HOLD A: EXTRA",16,226);}}
 canvas.fillRect(256,0,64,240,0x2124);canvas.setTextDatum(middle_center);canvas.setFont(&fonts::Font0);canvas.setTextSize(2);for(int i=0;i<3;i++){canvas.fillCircle(288,40+i*80,20,0x39E6);canvas.drawCircle(288,40+i*80,20,0x5B29);canvas.setTextColor(0xE754);char k[2]={(char)('A'+i),0};canvas.drawString(k,288,40+i*80);}canvas.pushSprite(0,0);}
void screenshot(){uint32_t size=54+320*240*3;uint8_t h[54]={0x42,0x4d};memcpy(h+2,&size,4);h[10]=54;h[14]=40;int32_t w=320,hh=240;memcpy(h+18,&w,4);memcpy(h+22,&hh,4);h[26]=1;h[28]=24;Serial.printf("BMP:%u\n",size);Serial.write(h,54);uint8_t row[960];for(int y=239;y>=0;y--){for(int x=0;x<320;x++){uint16_t c=canvas.readPixel(x,y);row[x*3]=(c&31)*255/31;row[x*3+1]=((c>>5)&63)*255/63;row[x*3+2]=((c>>11)&31)*255/31;}Serial.write(row,960);}Serial.println("\nBMP_END");}

void command(String s){s.trim();if(s=="state"){report();reportSD();}else if(s=="sd")mountSD();else if(s=="screenshot")screenshot();else if(s=="A"||s=="B"||s=="C")pressKey(s[0]);else if(s=="hour")advance(60);else if(s=="evolve")advance(std::max(1,stageDuration(activeStage.c_str())-state.stageMinutes));else if(s.startsWith("select ")){int i=s.substring(7).toInt();if(i>=0&&i<(int)catalog.size()){galleryIndex=i;refreshPreview();uiMode=GALLERY_INFO;}}else doAction(s);render();}
void setup(){auto cfg=M5.config();cfg.internal_spk=true;M5.begin(cfg);Serial.begin(115200);Serial.printf("START DigiPocket V2.10.0 PSRAM=%u heap=%u\n",ESP.getPsramSize(),ESP.getFreeHeap());M5.Display.setRotation(1);canvas.setColorDepth(16);canvas.setPsram(true);if(!canvas.createSprite(320,240)){M5.Display.setBrightness(160);M5.Display.fillScreen(TFT_BLACK);M5.Display.setTextColor(TFT_WHITE);M5.Display.println("Display memory allocation failed");Serial.println("BOOT FAILED: canvas allocation");while(true)delay(1000);}M5.Speaker.setVolume(0);Serial.printf("AUDIO init=%s\n",M5.Speaker.begin()?"yes":"no");randomSeed(esp_random());mountSD();loadAudio();catalog.reserve(PET_COUNT);for(int i=0;i<PET_COUNT;i++)catalog.push_back({pets[i].name,pets[i].path,pets[i].stage,pets[i].energy,pets[i].baseWeight,pets[i].attack});
if(sdReady){File f=SD.open("/digipet/catalog.tsv");if(f){catalog.clear();while(f.available()){String l=f.readStringUntil('\n');int a=l.indexOf('\t'),b=l.indexOf('\t',a+1),c=l.indexOf('\t',b+1),d=l.indexOf('\t',c+1),e=d<0?-1:l.indexOf('\t',d+1);if(a>0&&b>a&&c>b)catalog.push_back({l.substring(0,a),l.substring(a+1,b),l.substring(b+1,c),l.substring(c+1,d<0?l.length():d).toInt(),d<0?5:l.substring(d+1,e<0?l.length():e).toInt(),e<0?1:l.substring(e+1).toInt()});}f.close();}if(catalog.empty())for(int i=0;i<PET_COUNT;i++)catalog.push_back({pets[i].name,pets[i].path,pets[i].stage,pets[i].energy,pets[i].baseWeight,pets[i].attack});}
Preferences pref;pref.begin("digipocket",true);soundLevel=std::max(0,std::min(4,pref.getInt("sound",1)));if(pref.getInt("soundver",1)<2&&soundLevel>0)soundLevel++;alertLevel=std::max(0,std::min(4,pref.getInt("alertvol",1)));careAlerts=pref.getBool("alerts",true);keySounds=pref.getBool("keysound",true);brightness=std::max(0,std::min(3,pref.getInt("bright",3)));colorMode=pref.getInt("color",0)==1;theme=std::max(0,std::min(2,pref.getInt("theme",0)));timeSpeed=pref.getInt("speed",1)==60?60:1;screenTimeout=std::max(0,std::min(3,pref.getInt("timeout",0)));backgroundIndex=std::max(-1,std::min(BACKGROUND_COUNT-1,pref.getInt("bg",-1)));petStyle=std::max(0,std::min(2,pref.getInt("style",0)));pref.end();M5.Display.setBrightness((brightness+1)*63);M5.Speaker.setVolume(gentleVolume(soundLevel));selectPet("database/Baby I/Botamon.json");bool restored=restorePet();if(!saveBlocked&&(!restored||activeStage=="Digitama"))beginEgg(restored?activePath:"database/Digitama/Agu_Digitama.json");lastPetSave=captureFull();havePetSave=true;lastMinute=lastInput=millis();Serial.printf("BOOT DigiPocket V2.10.0 PSRAM=%u display=%dx%d\n",ESP.getPsramSize(),M5.Display.width(),M5.Display.height());report();reportSD();render();}
void loop(){M5.update();while(Serial.available()){char c=Serial.read();if(c=='\n'){command(serialLine);serialLine="";}else if(serialLine.length()<200)serialLine+=c;}if(!M5.Touch.getCount())ignoreTouch=false;if(M5.Touch.getCount()){auto t=M5.Touch.getDetail();if(screenOff&&t.wasPressed()){pressKey('C');ignoreTouch=true;}if(!ignoreTouch&&t.x>=256){if(uiMode==SHOT_TRAIN){if(t.wasPressed())pressKey('A'+std::min(2,(int)t.y/80));}else if(t.wasClicked())pressKey('A'+std::min(2,(int)t.y/80));if(t.wasHold()&&(t.y<80||t.y>=160))pressKey(t.y<80?'A':'C',true);}}if(sleepRequested&&!M5.Touch.getCount()){sleepRequested=false;savePet();M5.Speaker.stop();M5.Display.setBrightness(0);M5.Display.sleep();M5.Display.waitDisplay();M5.Power.lightSleep(M5.Power.sleep_no_timer,true);M5.Display.wakeup();M5.Display.setBrightness((brightness+1)*63);lastInput=millis();lastMinute=millis();ignoreTouch=true;}
finishUiAction();updateShotTraining();if(uiMode==DUNGEON_TIMING&&millis()-timingStart>=10000)dungeonResult(0);if(uiMode==TIMING&&millis()-timingStart>=10000)startTrainingResult(0);if(uiMode==BATTLE_TIMING&&millis()-timingStart>=10000){uiMode=HOME;cursor=-1;}savePet();uint32_t interval=60000/timeSpeed;if(millis()-lastMinute>=interval){int n=(millis()-lastMinute)/interval;lastMinute+=n*interval;advance(n);savePet();}updateActionAudio();updateCareNotice();updateCareRing();if(alertAutoWoke&&(int32_t)(millis()-alertWakeUntil)>=0){alertAutoWoke=false;screenOff=true;M5.Display.setBrightness(0);}uint32_t waits[]={0,30000,60000,300000};if(uiMode!=MATCH_INFO&&uiMode!=ANIMATION&&uiMode!=TIMING&&uiMode!=SHOT_TRAIN&&uiMode!=DUNGEON_TIMING&&uiMode!=BATTLE_TIMING&&!(alertWakeUntil&&(int32_t)(alertWakeUntil-millis())>0)&&waits[screenTimeout]&&millis()-lastInput>waits[screenTimeout]){screenOff=true;M5.Display.setBrightness(0);}if(millis()-lastFrame>=60){lastFrame=millis();frameNo=millis()/CARE_WALK_MS;render();}delay(5);}
