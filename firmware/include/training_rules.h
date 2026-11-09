#pragma once
#include <algorithm>
#include "pet_engine.h"
struct TrainingWindow{int min,max,width;};
inline TrainingWindow trainingWindow(const PetState&s,int maxEnergy){double score=.2*(s.battles>0?(double)s.wins/s.battles:0)+.2*s.hunger/4+.2*s.strength/4+.2*std::min((double)(s.totalMinutes/1440)/5,1.0)+.2*(maxEnergy>0?(double)s.energy/maxEnergy:0);int width=1+(int)(std::min(1.0,score)*3)*2,half=width/2;return {12-half,12+half,width};}
inline int trainingPosition(uint32_t elapsed){int steps=elapsed<40?0:1+(elapsed-40)/50,phase=steps%48;return phase<=24?phase:48-phase;}
inline int trainingHit(int position,const PetState&s,int maxEnergy){auto w=trainingWindow(s,maxEnergy);return s.battles>=999||(position>=w.min&&position<=w.max)?2:position>=w.min-5&&position<=w.max+5?1:0;}
