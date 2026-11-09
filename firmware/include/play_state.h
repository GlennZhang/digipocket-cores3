#pragma once
#include <stdint.h>
#include "special_data.h"
#include "game_balance.h"
struct PetSave {uint32_t version;char path[160];int32_t values[14];};
struct PetSaveV1 {uint32_t version;char path[160];int32_t values[13];};
struct BankPet {char path[160];int32_t values[19];};
struct GameState {int32_t bits=0,inventory[PLAY_ITEM_COUNT]={},bankLimit=5,energyBonus=0,evoBlocked=0,levelPlus=0,fullMinutes=0,autoCleanMinutes=0,dungeonFloor=1,dungeonHP=10,dungeonStatus=0;BankPet bank[10]={};};
struct FullSaveV3 {uint32_t version;PetSave pet;GameState game;};
struct EvolutionHistory {uint16_t routes[16]={}; uint8_t count=0; bool fusion=false;};
struct FullSave {uint32_t version;PetSave pet;GameState game;int32_t lifetimeTraining[11]; EvolutionHistory history[11]; int32_t rewindItems;};
