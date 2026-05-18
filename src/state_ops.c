#include "state_ops.h"
#include "state_internal.h"

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
    cj4_tile_id t = state->wall[CJ4_RINSHAN_INDICES[state->dead_wall_draw_count++]];
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

void
cj4_state_add_dora_indicator(cj4_mahjong *state)
{
    if (state->dora_indicators_count < CJ4_MAX_DORA)
        state->dora_indicators_count++;
}