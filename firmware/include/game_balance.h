#pragma once
#include <stddef.h>
#include <cstring>
constexpr int REGULAR_LOOT[]={4,3,8,5,40,36};
constexpr size_t REGULAR_LOOT_COUNT=6;
constexpr int MATERIAL_LOOT[]={12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,7,11,44};
constexpr size_t MATERIAL_LOOT_COUNT=26;
constexpr int AID_LOOT[]={6,9,35};
constexpr size_t AID_LOOT_COUNT=3;
constexpr int FRUIT_LOOT[]={0,1,2};
constexpr size_t FRUIT_LOOT_COUNT=3;
constexpr int FINAL_LOOT[]={41,42};
constexpr size_t FINAL_LOOT_COUNT=2;
constexpr int BOSS_AID_CHANCE=20;
constexpr int BOSS_FRUIT_CHANCE=10;
constexpr int PREMIUM_MEAT_MINUTES=180;
constexpr int AUTO_CLEAN_MINUTES=360;
template<size_t N> inline bool lootPoolHas(const int(&pool)[N],int item){for(int value:pool)if(value==item)return true;return false;}
