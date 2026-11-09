#pragma once
#include <esp_heap_caps.h>
#include "audio_assets.h"
int gentleVolume(int level);
// Playback pointers remain stable for the whole session, including cancelled/queued sounds.
void loadAudio(){int loaded=0;size_t extra=0;for(auto&clip:audioClips){if(!clip.data&&sdReady){File f=SD.open(String("/digipet/sound/")+clip.id+".pcm");if(f&&f.size()==clip.size&&clip.size<=200000&&extra+clip.size<=1000000){auto*p=(uint8_t*)heap_caps_malloc(clip.size,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);if(p){if(f.read(p,clip.size)==clip.size){clip.data=p;extra+=clip.size;}else heap_caps_free(p);}}f.close();}if(clip.data)loaded++;}Serial.printf("AUDIO clips=%d/27 cached=%u\n",loaded,(unsigned)extra);}
bool playSound(const char*id,int channel=0,int level=-1,uint32_t repeat=1){if(level<0)level=soundLevel;if(!level){M5.Speaker.stop(channel);return false;}for(auto&clip:audioClips)if(!strcmp(id,clip.id)){if(!clip.data)return false;M5.Speaker.setVolume(gentleVolume(level));return M5.Speaker.playRaw(clip.data,clip.size,11025,false,repeat,channel,true);}return false;}
