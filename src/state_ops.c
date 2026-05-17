#include "tile.h"
#include "state_ops.h"

cj4_tile_id
cj4_state_draw_tile(
    cj4_mahjong *state,
    cj4_player player)
{
    cj4_tile_id t = state->wall[state->wall_pos++];

    state->locations[t].zone  = CJ4_ZONE_HAND;
    state->locations[t].owner = player;

    return t;
}