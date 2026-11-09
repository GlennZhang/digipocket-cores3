#pragma once
#include <esp_heap_caps.h>

// Two PSRAM slots retain the active pet and the gallery/battle opponent.
struct DimSprite {
 String path;
 uint16_t* pixels=nullptr;
 int width=0,height=0,count=0;
 char labels[32][32]={};
};
DimSprite dimSprites[2];
int nextDimSlot=0;
bool drawingPreview=false;
String previewPath;

bool drawDimSprite(const String& path,int left,int top,int boxWidth,int boxHeight,const String& wanted,bool flip,int frameIndex){
 if(petStyle!=2||!sdReady)return false;
 DimSprite* sprite=nullptr;
 for(auto& entry:dimSprites)if(entry.path==path){sprite=&entry;break;}
 if(!sprite){
  sprite=&dimSprites[nextDimSlot];nextDimSlot^=1;
  if(sprite->pixels)heap_caps_free(sprite->pixels);
  sprite->pixels=nullptr;sprite->path=path;sprite->count=0;
  String file="/digipet/dim/"+path;file.replace(".json",".bin");
  File source=SD.open(file);
  if(!source)return false;
  uint8_t header[10];
  if(source.read(header,10)!=10||memcmp(header,"DGP1",4))return false;
  int width=header[4]|header[5]<<8,height=header[6]|header[7]<<8,count=header[8]|header[9]<<8;
  if(width<1||width>64||height<1||height>56||count<1||count>32||source.size()!=10+count*(32+width*height*2))return false;
  size_t frameBytes=width*height*2;
  sprite->pixels=(uint16_t*)heap_caps_malloc(frameBytes*count,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
  if(!sprite->pixels)return false;
  for(int index=0;index<count;index++){
   if(source.read((uint8_t*)sprite->labels[index],32)!=32||!memchr(sprite->labels[index],0,32)||source.read((uint8_t*)(sprite->pixels+index*width*height),frameBytes)!=frameBytes){
    heap_caps_free(sprite->pixels);sprite->pixels=nullptr;return false;
   }
  }
  sprite->width=width;sprite->height=height;sprite->count=count;
  Serial.printf("DIM loaded %s %dx%d\n",path.c_str(),width,height);
 }
 if(!sprite->pixels)return false;
 int options[32],count=0;
 for(int index=0;index<sprite->count;index++)if(String(sprite->labels[index]).startsWith(wanted))options[count++]=index;
 if(!count)for(int index=0;index<sprite->count;index++)if(String(sprite->labels[index]).startsWith("Walk"))options[count++]=index;
 if(!count)return false;
 int frame=options[(frameIndex<0?frameNo:frameIndex)%count];
 int scaleNumerator=std::min(boxWidth*56,boxHeight*64);
 int width=std::max(1,sprite->width*scaleNumerator/(64*56));
 int height=std::max(1,sprite->height*scaleNumerator/(64*56));
 left+=(boxWidth-width)/2;top+=boxHeight-height;
 const uint16_t* pixels=sprite->pixels+frame*sprite->width*sprite->height;
 for(int y=0;y<height;y++)for(int x=0;x<width;x++){
  int sx=x*sprite->width/width,sy=y*sprite->height/height;
  uint16_t color=pixels[sy*sprite->width+(flip?sprite->width-1-sx:sx)];
  if(color!=0xF81F&&left+x>=16&&left+x<240&&top+y>=64&&top+y<176)canvas.drawPixel(left+x,top+y,color);
 }
 return true;
}
