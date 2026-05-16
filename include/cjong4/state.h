#ifndef CJ4_STATE_H
#define CJ4_STATE_H

#include <stdint.h>

#include "tile.h"
#include "player.h"
#include "phase.h"
#include "location.h"

#define CJ4_MAX_DISCARDS 70

#define CJ4_MAX_MELDS 4

typedef struct
{
    cj4_tile_id tile;
    cj4_player player;
    uint8_t is_active;    // 1: still in river, 0: consumed (meld)
    uint8_t is_tsumogiri; // 1: tsumogiri, 0: from hand
} cj4_discard;

typedef enum {
    CJ4_MELD_CHI,
    CJ4_MELD_PON,
    CJ4_MELD_KAN
} cj4_meld_type;

typedef struct {
    cj4_tile_id tiles[4];   // tiles in meld (3 or 4)
    uint8_t     size;       // 3 (chi/pon) or 4 (kan)
    cj4_meld_type type;
    cj4_player  from_player; // player who provided the called tile
    uint8_t     called_index; // index of the called tile in the meld (0-3)
} cj4_meld;


typedef struct
{

    cj4_location locations[136];// indexed by cj4_tile_id

    cj4_phase phase;
    cj4_player current_player;

    cj4_tile_id draw_tile; // valid only when phase == CJ4_PHASE_DRAW

    cj4_tile_id wall[136];
    uint8_t    wall_pos;  // next draw position

    cj4_discard discards[CJ4_MAX_DISCARDS];
    uint8_t discard_count;

    cj4_meld melds[CJ4_PLAYER_COUNT][CJ4_MAX_MELDS];
    uint8_t  meld_count[CJ4_PLAYER_COUNT];

} cj4_mahjong;

#endif /* CJ4_STATE_H */