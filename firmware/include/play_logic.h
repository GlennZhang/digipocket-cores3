#pragma once
// Device UI for the offline item, egg, partner-bank and special-evolution systems.
enum PlayMode{P_MENU,P_BAG,P_SHOP,P_BANK,P_BANK_ACTION,P_JOGRESS,P_SPECIAL,P_TARGETS,P_DUNGEON,P_EGGS,P_CONFIRM,P_NOTICE,P_DUNGEON_RESULT};
PlayMode playMode=P_MENU,playParent=P_MENU;int playIndex=0,playParentIndex=0,playChoice=0,playOperation=0,playItem=-1,playSlot=0,pendingStoreSlot=-1,dungeonAfterHP=10;String playTitle,playTarget;bool playMagic=false;std::vector<String> playTargets;
bool dungeonSettlementPending=false,dungeonWon=false;int dungeonFloorPlayed=1,dungeonBitsAwarded=0;std::vector<int> dungeonLoot;
int itemIndex(String id){for(int i=0;i<PLAY_ITEM_COUNT;i++)if(id==playItems[i].id)return i;return -1;}
bool itemIs(int i,const char* id){return i>=0&&String(playItems[i].id)==id;}
void giveItem(int i){if(i>=0){game.inventory[i]=std::min(9999,game.inventory[i]+1);playSound("e21");}}
String pathName(String path){int i=path.lastIndexOf('/');path=path.substring(i+1);path.replace(".json","");return path;}
void previewEgg(){int old=galleryIndex;previewValid=false;for(unsigned i=0;i<catalog.size();i++)if(catalog[i].path==eggRoutes[playIndex].egg){galleryIndex=i;refreshPreview();break;}galleryIndex=old;}
void openPlay(PlayMode mode=P_MENU){uiMode=PLAY;playMode=mode;playIndex=playChoice=0;if(mode==P_EGGS)previewEgg();}
void playError(String text){playParent=playMode==P_CONFIRM?playParent:playMode;playParentIndex=playIndex;playTitle=text;playMode=P_NOTICE;uiMode=PLAY;}
void confirmPlay(int operation,String title,int item=-1,String target="",int slot=0){playParent=playMode;playParentIndex=playIndex;playMode=P_CONFIRM;playChoice=0;playOperation=operation;playTitle=title;playItem=item;playTarget=target;playSlot=slot;}
constexpr int REWIND_PRICE=10000;
std::vector<String> rewindTargets(){
 const auto& history=evolutionHistory[0];
 if(activeStage=="Digitama"||(!history.count&&history.fusion))return {};
 if(history.count)return {String(evoRoutes[history.routes[history.count-1]].from)};
 std::vector<String> targets;
 for(const auto& route:evoRoutes)
  if(activePath==route.best||activePath==route.middle||activePath==route.worst)
   targets.push_back(route.from);
 return targets;
}
void resetPartnerFlags(){game.energyBonus=game.evoBlocked=game.levelPlus=game.fullMinutes=game.autoCleanMinutes=0;}
bool beginEgg(String path){bool found=false;for(auto&r:eggRoutes)if(path==r.egg){found=true;break;}if(!found)return false;int oldBonus=game.energyBonus;game.energyBonus=0;if(!selectPet(path)){game.energyBonus=oldBonus;return false;}resetPartnerFlags();evolutionHistory[0]={};lifetimeTraining[0]=0;saveBlocked=false;state=PetState();protein=0;state.weight=baseWeight;state.energy=energyMax;uiAnimate("Egg","HATCHING",HOME,12000);pendingAction=5;lastMinute=millis();return true;}
void hatchEgg(){String target="";for(auto&r:eggRoutes)if(activePath==r.egg){target=r.baby;break;}if(!target.length()||!selectPet(target)){playError("NEED SD");return;}state=PetState();protein=0;state.energy=energyMax;state.weight=baseWeight;uiAnimate("Happy","HELLO!");lastMinute=millis();}
BankPet captureBank(){BankPet record={};auto pet=capturePet();memcpy(record.path,pet.path,sizeof(record.path));memcpy(record.values,pet.values,sizeof(pet.values));int32_t extra[]={game.energyBonus,game.evoBlocked,game.levelPlus,game.fullMinutes,game.autoCleanMinutes};memcpy(record.values+14,extra,sizeof(extra));return record;}
bool restoreBank(const BankPet&r){if(!r.path[0])return false;int oldBonus=game.energyBonus;game.energyBonus=r.values[14];if(!selectPet(r.path)){game.energyBonus=oldBonus;return false;}PetSave pet={};pet.version=2;memcpy(pet.path,r.path,sizeof(pet.path));memcpy(pet.values,r.values,sizeof(pet.values));applyPetValues(pet);game.evoBlocked=r.values[15];game.levelPlus=r.values[16];game.fullMinutes=r.values[17];game.autoCleanMinutes=r.values[18];pendingAction=0;lastMinute=millis();if(activeStage=="Digitama"){uiAnimate("Egg","HATCHING",HOME,12000);pendingAction=5;}else uiAnimate("Happy","WELCOME");return true;}
String jogressTarget(int slot){if(slot<0||slot>=game.bankLimit||!game.bank[slot].path[0])return "";String other=game.bank[slot].path;for(auto&r:jogressRoutes)if((activePath==r.first&&other==r.second)||(activePath==r.second&&other==r.first))return r.result;return "";}
bool specialEvolve(String target,bool magic=false){if(game.levelPlus)return false;int old=game.energyBonus;game.energyBonus=std::min(999,old+state.training/5);if(!selectPet(target)){game.energyBonus=old;return false;}evolutionHistory[0].count=0;state.care=state.stageMinutes=state.poop=0;state.training=magic?999:0;evolutionMissing=false;uiAnimate("Evolve","EVOLVE!",HOME,5000);return true;}
std::vector<int> visibleItems(bool shop){std::vector<int> list;for(int i=0;i<PLAY_ITEM_COUNT;i++)if(shop?playItems[i].price>=0:game.inventory[i]>0)list.push_back(i);return list;}
std::vector<int> visibleSpecial(){std::vector<int> list;for(int i=0;i<SPECIAL_ROUTE_COUNT;i++)if(activePath==specialRoutes[i].from)list.push_back(i);return list;}
std::vector<String> targetsFor(int item){std::vector<String> list;if(item<0)return list;String id=playItems[item].id;for(auto&r:specialRoutes)if(activePath==r.from&&id==r.item)list.push_back(r.to);if(!list.empty())return list;
 if(id=="best_fruit"||id=="normal_fruit"||id=="worst_fruit"){for(auto&r:evoRoutes)if(activePath==r.from){String target=id=="best_fruit"?r.best:id=="normal_fruit"?(strlen(r.middle)?r.middle:r.best):(strlen(r.worst)?r.worst:r.best);if(target.length())list.push_back(target);break;}}
 if(id=="miracle_egg"){for(auto&e:catalog)if(e.stage=="Ultimate-Super Ultimate"&&e.path.indexOf("Atratusmon")<0)list.push_back(e.path);}
 if(id=="miracle_fruit"){std::vector<String> pending={activePath},seen;while(!pending.empty()){String path=pending.back();pending.pop_back();if(std::find(seen.begin(),seen.end(),path)!=seen.end())continue;seen.push_back(path);if(path!=activePath&&path.startsWith("database/Ultimate-Super Ultimate/")&&path.indexOf("Atratusmon")<0)list.push_back(path);for(auto&r:evoRoutes)if(path==r.from){for(auto next:{r.best,r.middle,r.worst})if(strlen(next))pending.push_back(next);break;}}std::sort(list.begin(),list.end());}
 return list;}
bool evolutionItem(int item){String id=playItems[item].id;if(id=="x_antibody"||id=="jogres_disk"||id.indexOf("fruit")>=0||id=="orb_of_wishes")return true;for(auto&r:specialRoutes)if(id==r.item)return true;return false;}
void useItem(int item,String target=""){if(item<0||!game.inventory[item]){playError("NEED ITEM");return;}String id=playItems[item].id;auto targets=targetsFor(item);if(game.levelPlus&&evolutionItem(item)){playError("LEVEL+ LOCK");return;}
 if(!targets.empty()){if(std::find(targets.begin(),targets.end(),target)==targets.end()){playError("BAD TARGET");return;}if(!specialEvolve(target,id=="miracle_egg"||id=="miracle_fruit")){playError("NEED SD");return;}game.inventory[item]--;return;}
 if(id=="jogres_disk"){openPlay(P_JOGRESS);return;}if(evolutionItem(item)){playError("NO RECIPE");return;}
 if(id=="energy_drink")state.energy=energyMax;else if(id=="training_pack"){int gain=std::min(10,999-state.training);state.training+=gain;lifetimeTraining[0]=std::min(1000000000,lifetimeTraining[0]+gain);}else if(id=="elite_training_pack"){int gain=std::min(100,999-state.training);state.training+=gain;lifetimeTraining[0]=std::min(1000000000,lifetimeTraining[0]+gain);}else if(id=="super_carrot")state.weight=std::max(1,state.weight-10);else if(id=="time_gear"){String previous=activePath;advance(120);if(activePath!=previous){game.inventory[item]--;return;}}else if(id=="anti_evo_chip")game.evoBlocked^=1;else if(id=="level_plus"){if(game.levelPlus||(activeStage!="Child"&&activeStage!="Adult"&&activeStage!="Perfect")){playError("NOT ELIGIBLE");return;}game.levelPlus=1;}else if(id=="care_mistake_eraser")state.care=0;else if(id=="junk_food")state.care++;else if(id=="premium_meat"){state.hunger=state.strength=4;game.fullMinutes=PREMIUM_MEAT_MINUTES;}else if(id=="poop_clean_pill"){state.poop=0;game.autoCleanMinutes=AUTO_CLEAN_MINUTES;}else if(id=="alarm_clock")state.sleeping=false;else if(id=="sleeping_pill")state.sleeping=true;else if(id=="storage_drive"){if(game.bankLimit>=10){playError("BANK FULL");return;}game.bankLimit++;}else if(id=="heart_candy"){if(game.dungeonStatus||game.dungeonHP>=10){playError("NO NEED");return;}game.dungeonHP=std::min(10,game.dungeonHP+5);}else {playError("UNSUPPORTED");return;}
 game.inventory[item]--;playMode=P_BAG;playIndex=0;playItem=item;uiAnimate("Item","ITEM USED",PLAY,2400);}
void beginDungeon(){if(game.dungeonStatus||game.dungeonHP<=0){confirmPlay(8,"RESTART?");return;}if(activeStage=="Baby I"||activeStage=="Baby II"||activeStage=="Digitama"){playError("GROW FIRST");return;}if(state.energy<10||state.sleeping||state.poop||!state.hunger){playError(state.energy<10?"NEED 10 EP":"NEED CARE");return;}
 String wanted=game.dungeonFloor<=10?"Adult":game.dungeonFloor<=20?"Perfect":game.dungeonFloor<=30?"Ultimate-Super Ultimate":"Special";if(!prepareOpponent(wanted,true)){playError("NEED SD");return;}state.energy-=10;dungeonSettlementPending=false;uiAnimate("BattleIntro","",PLAY,4000);pendingAction=10;}
void dungeonResult(int hit){dungeonFloorPlayed=game.dungeonFloor;dungeonSettlementPending=true;startBattleFight(hit,true);}
void awardDungeonLoot(const int* pool,size_t count){int item=pool[random(count)];giveItem(item);dungeonLoot.push_back(item);}
void settleDungeon(bool win){
 if(!dungeonSettlementPending)return;
 dungeonSettlementPending=false;dungeonWon=win;dungeonFloorPlayed=game.dungeonFloor;dungeonBitsAwarded=0;dungeonLoot.clear();
 if(!win){game.dungeonHP=0;game.dungeonStatus=2;}
 else{
  int floor=game.dungeonFloor;dungeonBitsAwarded=floor<=10?2000:floor<=20?3000:floor<=30?5000:200000;
  game.bits=std::min(99999999,game.bits+dungeonBitsAwarded);
  game.dungeonHP=floor%10==0?10:std::min(10,std::max(1,dungeonAfterHP));
  if(floor==31)awardDungeonLoot(FINAL_LOOT,FINAL_LOOT_COUNT);
  else{
   int drops=floor>=21?2:floor>=11?1:0;
   for(int n=0;n<drops;n++)awardDungeonLoot(REGULAR_LOOT,REGULAR_LOOT_COUNT);
   if(floor%10==0){
    awardDungeonLoot(MATERIAL_LOOT,MATERIAL_LOOT_COUNT);
    if(random(100)<BOSS_AID_CHANCE)awardDungeonLoot(AID_LOOT,AID_LOOT_COUNT);
    if(floor>=20&&random(100)<BOSS_FRUIT_CHANCE)awardDungeonLoot(FRUIT_LOOT,FRUIT_LOOT_COUNT);
   }
  }
  if(floor==31){game.dungeonHP=10;game.dungeonStatus=1;}else game.dungeonFloor++;
 }
 playMode=P_DUNGEON_RESULT;playIndex=0;
}
void renderDungeonPanel(bool expanded=false){
 canvas.setFont(&fonts::Font0);canvas.setTextDatum(top_left);canvas.setTextSize(2);canvas.setTextColor(ink);
 std::vector<String> rows;
 if(playMode==P_DUNGEON_RESULT){
  if(playIndex==0){rows={"F"+String(dungeonFloorPlayed)+(dungeonWon?" CLEAR":" LOST"),"HP "+String(game.dungeonHP)+"/10","+"+String(dungeonBitsAwarded)+" BITS","LOOT "+String(dungeonLoot.size()),game.dungeonStatus==1?String("ALL 31 CLEARED"):game.dungeonStatus==2?String("RUN ENDED"):"NEXT F"+String(game.dungeonFloor),dungeonLoot.empty()?String("B RETURN"):String("B VIEW LOOT")};}
  else{int item=dungeonLoot[(playIndex-1)%dungeonLoot.size()];careBitmap(playItems[item].icon,12,12,0,2);roomText(playItems[item].name,14,0,18);roomText("+1 OWN "+String(game.inventory[item]),14,9,18);canvas.setTextSize(1);canvas.drawString("LOOT "+String(playIndex)+"/"+String(dungeonLoot.size())+"  B NEXT",108,166);return;}
 }else if(playIndex==1){rows={"REWARDS","F1-10  +2000","F11-20 +3000 +1","F21-30 +5000 +2","BOSS: EXTRA LOOT","F31 +200000 RARE"};}
 else if(playIndex==2){rows={"ITEM GUIDE","X: SHOP 15000B","OR BOSS REWARD","EGGS/SPIRITS:","BOSS F10/20/30","USE: PLAY > ITEMS"};}
 else if(playIndex==3){rows={"RARE EVO FRUIT","BOSS F20 / F30","10% EXTRA CHANCE","RANDOM 1 OF 3","NOT SOLD IN SHOP","A MORE   B ENTER"};}
 else{String enemy=game.dungeonFloor<=10?"ADULT":game.dungeonFloor<=20?"PERFECT":game.dungeonFloor<=30?"ULTIMATE":"SPECIAL";rows={"FLOOR "+String(game.dungeonFloor)+"/31","HP "+String(game.dungeonHP)+"/10",game.dungeonStatus==1?String("ALL CLEARED"):game.dungeonStatus==2?String("RUN FAILED"):"VS "+enemy,"ENTRY 10 EP",(game.dungeonFloor%10==0||game.dungeonFloor==31)?String("BOSS FLOOR"):String("HP CARRIES OVER"),game.dungeonStatus?String("B RESTART"):String("A GUIDE  B ENTER")};}
 for(unsigned i=0;i<rows.size();i++)canvas.drawString(rows[i],16,(expanded?42:63)+i*(expanded?26:19));
}

void pressPlay(char key,bool hold){if(key=='C'){if(playMode==P_DUNGEON_RESULT){openPlay(P_DUNGEON);return;}if(playMode==P_EGGS&&pendingStoreSlot>=0){pendingStoreSlot=-1;openPlay(P_BANK);return;}if(playMode==P_CONFIRM||playMode==P_NOTICE){playMode=playParent;playIndex=playParentIndex;}else if(playMode==P_MENU){uiMode=HOME;cursor=-1;}else openPlay();return;}
 int count=playMode==P_DUNGEON?4:playMode==P_DUNGEON_RESULT?(int)dungeonLoot.size()+1:playMode==P_MENU?9:playMode==P_EGGS?EGG_COUNT:playMode==P_BANK||playMode==P_JOGRESS?game.bankLimit:playMode==P_BANK_ACTION?3:playMode==P_TARGETS?playTargets.size():1;
 if(playMode==P_BAG||playMode==P_SHOP)count=visibleItems(playMode==P_SHOP).size()+(playMode==P_SHOP||rewindItems>0);if(playMode==P_SPECIAL)count=visibleSpecial().size();if(key=='A'){if(playMode==P_CONFIRM)playChoice^=1;else playIndex=(playIndex+(hold?10:1))%std::max(1,count);if(playMode==P_EGGS)previewEgg();return;}if(key!='B')return;
 if(playMode==P_DUNGEON_RESULT){if(playIndex<(int)dungeonLoot.size()){playIndex++;playSound("e21");}else openPlay(P_DUNGEON);return;}
 if(playMode==P_NOTICE){playMode=playParent;playIndex=playParentIndex;return;}
 if(playMode==P_MENU){if(playIndex==5){uiMode=COMBAT_STATS;combatPage=0;return;}if(playIndex==8){uiMode=SETTINGS;page=0;return;}PlayMode modes[]={P_BAG,P_SHOP,P_BANK,P_JOGRESS,P_SPECIAL,P_DUNGEON,P_EGGS};openPlay(modes[playIndex>5?playIndex-1:playIndex]);return;}
 if(playMode==P_BAG||playMode==P_SHOP){auto list=visibleItems(playMode==P_SHOP);if(playIndex==(int)list.size()&&(playMode==P_SHOP||rewindItems>0)){
   if(playMode==P_SHOP){confirmPlay(10,"Rewind Chip");return;}
   playTargets=rewindTargets();
   if(playTargets.empty()){playError("NO PREVIOUS FORM");return;}
   if(playTargets.size()==1){confirmPlay(9,pathName(playTargets[0]),-1,playTargets[0]);return;}
   playItem=-1;playMode=P_TARGETS;playIndex=0;return;
  }if(list.empty()){playError("EMPTY");return;}int item=list[playIndex%list.size()];if(playMode==P_SHOP){confirmPlay(1,playItems[item].name,item);return;}if(itemIs(item,"jogres_disk")){openPlay(P_JOGRESS);return;}playTargets=targetsFor(item);if(playTargets.size()>1){playItem=item;playMode=P_TARGETS;playIndex=0;return;}confirmPlay(2,playItems[item].name,item,playTargets.empty()?"":playTargets[0]);return;}
 if(playMode==P_EGGS){confirmPlay(3,"NEW EGG?",-1,eggRoutes[playIndex].egg);return;}
 if(playMode==P_BANK){playSlot=playIndex;playMode=P_BANK_ACTION;playIndex=0;return;}
 if(playMode==P_BANK_ACTION){if(playIndex==0&&game.bank[playSlot].path[0]){playError("SLOT USED");return;}if(playIndex>0&&!game.bank[playSlot].path[0]){playError("EMPTY SLOT");return;}confirmPlay(4+playIndex,playIndex==0?"STORE+EGG?":playIndex==1?"SWAP?":"RELEASE?",-1,"",playSlot);return;}
 if(playMode==P_JOGRESS){String target=jogressTarget(playIndex);if(!target.length()){playError("NO DNA PAIR");return;}int disk=itemIndex("jogres_disk");if(!game.inventory[disk]){playError("NEED DISK");return;}confirmPlay(7,pathName(target),disk,target,playIndex);return;}
 if(playMode==P_SPECIAL){auto list=visibleSpecial();if(list.empty()){playError("NO RECIPE");return;}auto&r=specialRoutes[list[playIndex%list.size()]];confirmPlay(2,pathName(r.to),itemIndex(r.item),r.to);return;}
 if(playMode==P_TARGETS){confirmPlay(playItem<0?9:2,pathName(playTargets[playIndex]),playItem,playTargets[playIndex]);return;}
 if(playMode==P_DUNGEON){beginDungeon();return;}
 if(playMode==P_CONFIRM){if(!playChoice){playMode=playParent;playIndex=playParentIndex;return;}if(playOperation==1){if(itemIs(playItem,"storage_drive")&&game.bankLimit>=10){playError("BANK FULL");return;}if(game.bits<playItems[playItem].price){playError("NEED BITS");return;}if(game.inventory[playItem]>=9999){playError("ITEM LIMIT");return;}game.bits-=playItems[playItem].price;giveItem(playItem);playMode=playParent;playIndex=playParentIndex;}
 else if(playOperation==2)useItem(playItem,playTarget);else if(playOperation==3){auto previous=captureBank();int previousTraining=lifetimeTraining[0];auto previousHistory=evolutionHistory[0];if(!beginEgg(playTarget))playError("NEED SD");else if(pendingStoreSlot>=0){game.bank[pendingStoreSlot]=previous;lifetimeTraining[pendingStoreSlot+1]=previousTraining;evolutionHistory[pendingStoreSlot+1]=previousHistory;pendingStoreSlot=-1;}}else if(playOperation==4){if(activeStage=="Digitama"){playError("HATCH FIRST");return;}pendingStoreSlot=playSlot;openPlay(P_EGGS);}
 else if(playOperation==5){auto old=captureBank();int oldTraining=lifetimeTraining[0];if(restoreBank(game.bank[playSlot])){std::swap(evolutionHistory[0],evolutionHistory[playSlot+1]);lifetimeTraining[0]=lifetimeTraining[playSlot+1];lifetimeTraining[playSlot+1]=oldTraining;game.bank[playSlot]=old;}else playError("NEED SD");}else if(playOperation==6){game.bank[playSlot]={};lifetimeTraining[playSlot+1]=0;evolutionHistory[playSlot+1]={};openPlay(P_BANK);playIndex=playSlot;}
 else if(playOperation==7){if(!game.inventory[playItem]||playTarget!=jogressTarget(playSlot)){playError("NO DNA PAIR");return;}if(!specialEvolve(playTarget)){playError(game.levelPlus?"LEVEL+ LOCK":"NEED SD");return;}game.inventory[playItem]--;game.bank[playSlot]={};lifetimeTraining[playSlot+1]=0;evolutionHistory[playSlot+1]={};state.energy=std::max(0,state.energy-5);state.weight=std::max(1,state.weight-4);evolutionHistory[0]={};evolutionHistory[0].fusion=true;action="Jogress";}
 else if(playOperation==9){
  if(!rewindItems){playError("NEED ITEM");return;}
  auto targets=rewindTargets();
  if(std::find(targets.begin(),targets.end(),playTarget)==targets.end()){playError("BAD TARGET");return;}
  if(!selectPet(playTarget)){playError("NEED SD");return;}
  auto& history=evolutionHistory[0];
  if(history.count)history.routes[--history.count]=0;
  rewindItems--;
  state.stageMinutes=state.training=state.care=0;
  game.levelPlus=0;
  evolutionMissing=false;
  uiAnimate("Evolve","DEVOLVE!",HOME,5000);
 }
 else if(playOperation==10){
  if(game.bits<REWIND_PRICE){playError("NEED BITS");return;}
  if(rewindItems>=9999){playError("ITEM LIMIT");return;}
  game.bits-=REWIND_PRICE;
  rewindItems++;
  playMode=playParent;
  playIndex=playParentIndex;
 }
 else if(playOperation==8){game.dungeonFloor=1;game.dungeonHP=10;game.dungeonStatus=0;openPlay(P_DUNGEON);} }
}
// Extended features use the whole left panel; the pet's classic menu stays separate.
void drawPlayLine(String text,int x,int y,int characters=18){
 int overflow=std::max(0,(int)text.length()-characters);
 int offset=overflow?std::min(overflow,(int)((millis()/400)%(overflow+6))):0;
 canvas.drawString(text.substring(offset,offset+characters),x,y);
}
void renderExtendedPlay(){
 canvas.fillRect(0,0,256,240,bg);canvas.setFont(&fonts::Font0);canvas.setTextDatum(top_left);canvas.setTextColor(ink);canvas.setTextSize(2);
 String heading="EXTRA MENU",footer="A SELECT  B OPEN  C BACK";
 if(playMode==P_DUNGEON||playMode==P_DUNGEON_RESULT){
  heading=playMode==P_DUNGEON?"DUNGEON":"DUNGEON RESULT";drawPlayLine(heading,16,10);renderDungeonPanel(true);
  if(playMode==P_DUNGEON){for(int i=1;i<=31;i++){uint16_t color=i<game.dungeonFloor||game.dungeonStatus==1?ink:ghost;canvas.fillRect(16+(i-1)*7,205,5,10,color);if(i==game.dungeonFloor)canvas.drawRect(15+(i-1)*7,203,7,14,ink);}}
  footer=playMode==P_DUNGEON?"A GUIDE   B ENTER   C BACK":"A PAGE   B NEXT   C BACK";
 }else if(playMode==P_MENU){
  const char* labels[]={"BACKPACK","SHOP","PARTNERS","JOGRESS","SPECIAL EVO","COMBAT STATS","DUNGEON","NEW EGG","SETTINGS"};
  int first=playIndex>=7?1:0;for(int row=0;row<8;row++){int i=first+row,y=37+row*23;if(i==playIndex)canvas.fillRect(12,y-3,232,22,ghost);drawPlayLine(String(i==playIndex?"> ":"  ")+labels[i],16,y);}
 }else if(playMode==P_BAG||playMode==P_SHOP){
  heading=playMode==P_SHOP?"SHOP":"BACKPACK";auto list=visibleItems(playMode==P_SHOP);
  if(playMode==P_SHOP||rewindItems>0)list.push_back(-1);
  if(list.empty())drawPlayLine("EMPTY",16,82);
  else{
   int first=std::max(0,std::min(playIndex-2,(int)list.size()-6));
   for(int row=0;row<6&&first+row<(int)list.size();row++){
    int index=first+row,item=list[index],y=39+row*25;if(index==playIndex)canvas.fillRect(12,y-2,232,25,ghost);
    if(item<0){
     canvas.drawRect(18,y+3,18,18,ink);
     drawPlayLine("Rewind Chip",46,y+4,12);
     drawPlayLine(String(rewindItems),202,y+4,4);
     continue;
    }
    for(int yy=0;yy<12;yy++)for(int xx=0;xx<12;xx++)if(playItems[item].icon[yy*12+xx])canvas.fillRect(16+xx*2,y+yy*2,2,2,ink);
    drawPlayLine(playItems[item].name,46,y+4,12);drawPlayLine(String(game.inventory[item]),202,y+4,4);
   }
   int item=list[playIndex%list.size()];canvas.setTextSize(1);
   if(item<0){
    canvas.drawString(playMode==P_SHOP?String(REWIND_PRICE)+" BITS  OWN "+String(rewindItems):String("RETURNS TO PREVIOUS FORM"),16,195);
   }else{
   canvas.drawString(playMode==P_SHOP?String(playItems[item].price)+" BITS  OWN "+String(game.inventory[item]):String(playIndex+1)+"/"+String(list.size())+"  HOLD A: +10",16,195);
   String source=lootPoolHas(FINAL_LOOT,item)?"SOURCE: F31 FINAL REWARD":lootPoolHas(FRUIT_LOOT,item)?"RARE: BOSS F20/30 (10%)":lootPoolHas(AID_LOOT,item)?"RARE: BOSS F10/20/30 (20%)":lootPoolHas(MATERIAL_LOOT,item)?"SOURCE: SHOP / BOSS LOOT":lootPoolHas(REGULAR_LOOT,item)?"SOURCE: SHOP / F11-30 DROP":"SOURCE: SHOP ONLY";if(lootPoolHas(MATERIAL_LOOT,item)&&playItems[item].price<0)source="SOURCE: BOSS F10/20/30";canvas.drawString(source,16,210);
   }
   canvas.setTextSize(2);
  }
 }else if(playMode==P_BANK||playMode==P_JOGRESS){
  heading=playMode==P_BANK?"PARTNERS":"JOGRESS PARTNER";int first=std::max(0,std::min(playIndex-2,game.bankLimit-6));
  for(int row=0;row<6&&first+row<game.bankLimit;row++){int index=first+row,y=43+row*26;if(index==playIndex)canvas.fillRect(12,y-3,232,24,ghost);drawPlayLine(String(index+1)+" "+(game.bank[index].path[0]?pathName(game.bank[index].path):String("EMPTY")),16,y);}
  canvas.setTextSize(1);canvas.drawString(playMode==P_JOGRESS?"B: CHECK JOGRESS RECIPE":game.bank[playIndex].path[0]?"B: SWAP / RELEASE":"B: STORE + NEW EGG",16,210);canvas.setTextSize(2);
 }else if(playMode==P_BANK_ACTION){
  heading="PARTNER SLOT "+String(playSlot+1);const char* labels[]={"STORE + NEW EGG","SWAP","RELEASE"};
  for(int i=0;i<3;i++){int y=58+i*36;if(i==playIndex)canvas.fillRect(12,y-4,232,27,ghost);drawPlayLine(String(i==playIndex?"> ":"  ")+labels[i],16,y);}
  canvas.setTextSize(1);canvas.drawString(playIndex==0?"KEEP CURRENT PET, THEN PICK AN EGG":playIndex==1?"EXCHANGE ACTIVE AND STORED PET":"REMOVE THE STORED PET",16,190);canvas.setTextSize(2);
 }else if(playMode==P_EGGS){
  heading="NEW EGG";drawPlayLine(pathName(eggRoutes[playIndex].egg),16,38);previewSprite(8,0,"egg 1",false);
  canvas.setTextSize(1);canvas.drawString("REPLACES YOUR ACTIVE PET",16,195);canvas.drawString("TO KEEP IT: PARTNERS > STORE",16,210);canvas.setTextSize(2);
 }else if(playMode==P_SPECIAL||playMode==P_TARGETS){
  heading=playMode==P_SPECIAL?"SPECIAL EVOLUTION":playItem<0?"DEVOLVE TO":"EVOLVE TO";auto list=visibleSpecial();int count=playMode==P_SPECIAL?(int)list.size():(int)playTargets.size();
  if(!count)drawPlayLine("NO RECIPE",16,80);
  else{
   int first=std::max(0,std::min(playIndex-2,count-6));
   for(int row=0;row<6&&first+row<count;row++){int index=first+row,y=43+row*26;if(index==playIndex)canvas.fillRect(12,y-3,232,24,ghost);String path=playMode==P_SPECIAL?String(specialRoutes[list[index]].to):playTargets[index];drawPlayLine(pathName(path),16,y);}
   int item=playMode==P_SPECIAL?itemIndex(specialRoutes[list[playIndex]].item):playItem;canvas.setTextSize(1);if(item>=0)canvas.drawString(String(playItems[item].name)+"  OWN "+String(game.inventory[item]),16,210);canvas.setTextSize(2);
  }
 }else if(playMode==P_CONFIRM){
  heading=playOperation==9?"DEVOLVE?":"CONFIRM";drawPlayLine(playTitle,16,62);if(playOperation==3){canvas.setTextSize(1);canvas.drawString(pendingStoreSlot>=0?"CURRENT PET WILL BE STORED":"CURRENT PET WILL BE REPLACED",16,94);canvas.setTextSize(2);}
  for(int i=0;i<2;i++){int y=120+i*38;if(i==playChoice)canvas.fillRect(12,y-4,232,29,ghost);drawPlayLine(String(i==playChoice?"> ":"  ")+(i?"YES":"NO"),16,y);}footer="A CHOOSE   B CONFIRM   C BACK";
 }else if(playMode==P_NOTICE){heading="NOTICE";drawPlayLine(playTitle,16,86);footer="B / C BACK";}
 if(playMode!=P_DUNGEON&&playMode!=P_DUNGEON_RESULT)drawPlayLine(heading,16,10);
 canvas.setTextSize(1);if(playMode==P_MENU||playMode==P_SHOP)canvas.drawString("BITS "+String(game.bits),146,14);canvas.drawString(footer,16,228);if(playMode==P_MENU)canvas.drawString(String(playIndex+1)+"/9",218,228);
}
