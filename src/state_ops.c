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


cj4_mahjong
cj4_state_discard_tile(
    const cj4_mahjong state,
    cj4_player player,
    cj4_tile_id tile)
{
    cj4_mahjong next = state;

    next.locations[tile].zone  = CJ4_ZONE_RIVER;
    next.locations[tile].owner = player;

    cj4_discard *d = &next.discards[next.discard_count++];
    d->tile = tile;
    d->player = player;
    d->is_active = 1;
    d->is_tsumogiri = (tile == next.draw_tile);

    next.draw_tile = CJ4_TILE_ID_INVALID;

    next.phase = CJ4_PHASE_DISCARD;

    return next;
}