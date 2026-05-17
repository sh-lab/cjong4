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

    state->draw_count++;

    return t;
}

cj4_tile_id
cj4_state_draw_dead_wall_tile(
    cj4_mahjong *state,
    cj4_player player)
{
    cj4_tile_id t = state->wall[state->dead_wall_draw_pos--];
    state->locations[t].zone  = CJ4_ZONE_HAND;
    state->locations[t].owner = player;

    state->draw_count++;

    return t;
}

void
cj4_state_call_post_process(cj4_mahjong *state)
{
    // Reset draw tile after a call (chi/pon/minkan)
    state->draw_tile = CJ4_TILE_ID_INVALID;

    state->discards[state->discard_count - 1].is_active = 0;
}