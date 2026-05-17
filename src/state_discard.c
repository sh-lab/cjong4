#include "state_discard.h"
#include "state_ops.h"

bool
cj4_can_discard(
    const cj4_mahjong state,
    cj4_tile_id tile)
{
    if (state.phase != CJ4_PHASE_DRAW)
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
cj4_discard(
    const cj4_mahjong state,
    cj4_tile_id tile)
{
    return cj4_state_discard_tile(
        state,
        state.current_player,
        tile);
}
