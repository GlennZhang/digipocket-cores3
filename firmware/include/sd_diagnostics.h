#pragma once
// CoreS3 LCD and SD share SPI2 and GPIO35 (LCD D/C versus SD MISO).
// Never SPI.end() here: it would tear down the display's shared bus.
uint32_t sdFrequency=0;
void reportSD(){
 Serial.printf("SDINFO {\"mounted\":%s,\"clockHz\":%u,\"cardType\":%u,\"cardBytes\":%llu,\"filesystemBytes\":%llu,\"usedBytes\":%llu,\"catalog\":%s}\n",sdReady?"true":"false",sdFrequency,sdReady?SD.cardType():0,sdReady?SD.cardSize():0ULL,sdReady?SD.totalBytes():0ULL,sdReady?SD.usedBytes():0ULL,sdReady&&SD.exists("/digipet/catalog.tsv")?"true":"false");
}
bool mountSD(){
 if(sdReady){reportSD();return true;}
 M5.Display.waitDisplay();
 // No LCD transfers while mounting; deselect both slaves and wait for card power.
 pinMode(3,OUTPUT);digitalWrite(3,HIGH);
 pinMode(4,OUTPUT);digitalWrite(4,HIGH);
 delay(150);
 SPI.begin(36,35,37,4);
 const uint32_t speeds[]={25000000,10000000,4000000,1000000};
 for(uint32_t speed:speeds){
  Serial.printf("SD TRY clock=%u cs=4 sck=36 miso=35 mosi=37\n",speed);
  // format_if_empty must remain false: diagnosis must never format a user's card.
  if(SD.begin(4,SPI,speed,"/sd",5,false)){
   File root=SD.open("/");bool readable=root&&root.isDirectory();if(root)root.close();
   if(SD.cardType()!=CARD_NONE&&readable&&SD.totalBytes()>0){sdReady=true;sdFrequency=speed;reportSD();return true;}
   Serial.println("SD mount returned success but filesystem root/size check failed");
  }else Serial.printf("SD mount failed at %u Hz\n",speed);
  SD.end();digitalWrite(4,HIGH);delay(150);
 }
 Serial.println("SD FAILED: no readable FAT volume; embedded pet assets remain available");reportSD();return false;
}
