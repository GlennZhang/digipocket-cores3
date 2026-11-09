#pragma once
#include <stdint.h>
// Shield center travels between the two lane centers in 2.4 real seconds.
inline int shotShieldCenter(uint32_t elapsed,int startLow){uint32_t phase=elapsed%2400;int offset=(phase<=1200?phase:2400-phase)*42/1200;return 99+(startLow?42-offset:offset);}
// Match the rendered 18px shield against the 24px projectile; touching edges do not block.
inline bool shotShieldBlocks(int lane,int center){if(lane<0||lane>1)return false;int delta=(lane?141:99)-center;return delta>-21&&delta<21;}
