#include "state_discard.h"
#include "state_ops.h"

bool
cj4_can_discard(
    const cj4_mahjong state,
    cj4_tile_id tile)
{
    if (state.phase != CJ4_PHASE_DRAW && state.phase != CJ4_PHASE_AFTER_CALL)
    {
        return false;
    }

    if (tile > CJ4_TILE_ID_MAX)
    {
        return false;
    }
    
    cj4_location *loc = &state.locations[tile];
    if (loc->zone != CJ4_ZONE_HAND || loc->owner != state.current_player)
    {
        return false;
    }

    return true;
}

cj4_mahjong
cj4_do_discard(
    const cj4_mahjong state,
    cj4_tile_id tile)
{
    cj4_mahjong next = state;

    next.locations[tile].zone  = CJ4_ZONE_RIVER;
    next.locations[tile].owner = state.current_player;

    cj4_discard *d = &next.discards[next.discard_count++];
    d->tile = tile;
    d->player = state.current_player;
    d->is_active = 1;
    d->is_tsumogiri = (tile == state.draw_tile);

    next.draw_tile = CJ4_TILE_ID_INVALID;

    next.phase = CJ4_PHASE_DISCARD;

    return next;
}
