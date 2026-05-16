#ifndef CJ4_STATE_H
#define CJ4_STATE_H

#include <stdint.h>

#include "player.h"
#include "tile.h"
#include "phase.h"
#include "location.h"

#define CJ4_MAX_DISCARDS 70

typedef struct
{
    cj4_tile_id tile;
    cj4_player player;
    uint8_t is_active;    // 1: still in river, 0: consumed (meld)
    uint8_t is_tsumogiri; // 1: tsumogiri, 0: from hand
} cj4_discard;

typedef struct
{

    cj4_location locations[136];
    
    cj4_phase phase;
    cj4_player current_player;


    cj4_discard discards[CJ4_MAX_DISCARDS];
    uint16_t discard_count;
} cj4_mahjong;

#endif /* CJ4_STATE_H */