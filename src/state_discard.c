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

    if (!cj4_state_tile_is_in_hand(&state, state.current_player, tile))
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

    cj4_state_record_discard(&next, tile, (uint8_t)(tile == state.draw_tile));
    cj4_state_clear_draw_tile(&next);

    next.phase = CJ4_PHASE_DISCARD;

    return next;
}
