#pragma once

String combatPercent(double value,bool bonus=false){char text[24];snprintf(text,sizeof(text),bonus?"%+.1fPP":"%.1f%%",value*100);return text;}
const char* combatAttribute(int attribute){const char* names[]={"FREE","VACCINE","VIRUS","DATA"};return names[attribute];}

void renderCombat(){
 canvas.fillRect(0,0,256,240,bg);canvas.setFont(&fonts::Font0);canvas.setTextDatum(top_left);canvas.setTextSize(2);canvas.setTextColor(ink);
 bool match=uiMode==MATCH_INFO;Fighter own=match?combatOwn:combatPlayer(),foe=match?combatRival:Fighter{};
 if(!match){foe.stage=own.stage;foe.attribute=0;}
 auto bonus=battleBonuses(own,foe);std::vector<String> rows;
 drawPlayLine(String(match?"MATCH INFO ":"COMBAT STATS ")+String(combatPage+1)+(match?"/3":"/4"),16,10);
 if(!match){
  if(combatPage==0){String stage=activeStage=="Ultimate-Super Ultimate"?String("ULTIMATE"):activeStage=="Armor-Hybrid"?String("ARMOR"):activeStage;rows={activeName,stage+" LV"+String(own.stage),String("ATTR ")+combatAttribute(own.attribute),"AGE "+String(own.age)+" DAYS","FIGHTS "+String(own.battles),"WINS "+String(state.wins)+" "+String(state.battles?state.wins*100/state.battles:0)+"%","CUR TRAIN "+String(own.training),"TOTAL "+String(own.total)};}
  else if(combatPage==1){rows={"CUR "+String(own.training)+"/999","GAIN "+combatPercent(bonus.training,true),"TOTAL "+String(own.total),"GAIN "+combatPercent(bonus.total,true),"AGE "+String(own.age)+"D","GAIN "+combatPercent(bonus.age,true),"FIGHTS "+String(own.battles),"GAIN "+combatPercent(bonus.battles,true)};}
  else if(combatPage==2){rows={"HUNGER "+String(own.hunger)+"/4","GAIN "+combatPercent(bonus.hunger,true),"POWER "+String(own.strength)+"/4","GAIN "+combatPercent(bonus.strength,true),"WEIGHT "+String(own.weight)+"G","BASE "+String(own.baseWeight)+"G","GAIN "+combatPercent(bonus.weight,true)};}
  else{rows={"REF "+combatPercent(battleInitialChances(own,foe).own),"SAME LV / FREE FOE","BASE 10.0%","LEVEL +/-10PP/LV","ATTR +/-5PP","MISS +10PP","HIT RESETS PITY","NOT A WIN RATE"};}
 }else{
  if(combatPage==0){const char* opening[]={"MISS","GOOD","MEGA"};rows={pathName(previewPath),battleLog.dungeon?"DUNGEON F"+String(dungeonFloorPlayed):String("PRACTICE"),"YOU HIT "+combatPercent(combatChances.own),"FOE HIT "+combatPercent(combatChances.rival),"YOU HP "+String(battleLog.initialOwn),"FOE HP "+String(battleLog.initialRival),String("OPENING ")+opening[combatOpening],"INITIAL HIT ONLY"};}
  else if(combatPage==1){rows={"YOU LV"+String(own.stage)+" "+combatAttribute(own.attribute),"FOE LV"+String(foe.stage)+" "+combatAttribute(foe.attribute),"LEVEL "+combatPercent(bonus.stage,true),"ATTR "+combatPercent(bonus.attribute,true),"HUNGER "+combatPercent(bonus.hunger,true),"POWER "+combatPercent(bonus.strength,true),"WEIGHT "+combatPercent(bonus.weight,true),battleLog.dungeon&&game.dungeonFloor<=20?String("FOE LV6 OVERRIDE"):String("GAINS ARE ROUNDED")};}
  else{rows={"BASE 10.0%","CUR "+combatPercent(bonus.training,true),"TOTAL "+combatPercent(bonus.total,true),"AGE "+combatPercent(bonus.age,true),"FIGHTS "+combatPercent(bonus.battles,true),"RAW "+combatPercent(battleAccuracy(own,foe)),combatChances.specialAdjusted?String("SPECIAL -> 25.0%"):combatChances.sameStageCapped?String("SAME LV -> 60.0%"):String("HIT RANGE 0-100%"),"MISS +10PP, RESET"};}
 }
 for(unsigned i=0;i<rows.size();i++)drawPlayLine(rows[i],16,38+i*22);
 canvas.setTextSize(1);canvas.drawString(match?"A PAGE  B FIGHT  C CANCEL":"A/B PAGE   C BACK",16,228);
}
