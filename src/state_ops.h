#ifndef CJ4_STATE_OPS_H
#define CJ4_STATE_OPS_H

#include "tile.h"
#include "state.h"
#include "rules.h"

cj4_tile_id
cj4_state_draw_tile(
    cj4_mahjong *state,
    cj4_player player
);


cj4_tile_id
cj4_state_draw_dead_wall_tile(
    cj4_mahjong *state,
    cj4_player player
);

void
cj4_state_call_post_process(cj4_mahjong *state);

void
cj4_state_add_dora_indicator(cj4_mahjong *state);
#endif /* CJ4_STATE_OPS_H */