#ifndef CJ4_STATE_OPS_H
#define CJ4_STATE_OPS_H

#include "state.h"

cj4_tile_id
cj4_state_draw_tile(
    cj4_mahjong *state,
    cj4_player player
);

cj4_mahjong
cj4_state_discard_tile(
    const cj4_mahjong state,
    cj4_player player,
    cj4_tile_id tile
);

#endif /* CJ4_STATE_OPS_H */