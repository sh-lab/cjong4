#include "state_discard.h"
#include "state_ops.h"

bool
cj4_can_discard(
    const cj4_mahjong state,
    cj4_tile_id tile)
{
    cj4_player player = state.current_player;

    if (state.phase != CJ4_PHASE_DRAW && state.phase != CJ4_PHASE_AFTER_CALL)
    {
        return false;
    }

    if (tile > CJ4_TILE_ID_MAX)
    {
        return false;
    }

    if (!cj4_state_tile_is_in_hand(&state, player, tile))
    {
        return false;
    }

    if (state.is_riichi[player] && tile != state.draw_tile)
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
    next.winning_from_chankan = 0;
    next.pending_kakan_tile = CJ4_TILE_ID_INVALID;
    if (state.is_ippatsu[state.current_player])
        next.is_ippatsu[state.current_player] = 0;

    if (state.is_riichi[state.current_player] &&
        state.first_turn_uninterrupted &&
        state.draw_turn_count[state.current_player] == 1)
    {
        next.riichi_declared_on_first_turn[state.current_player] = 1;
    }

    next.phase = CJ4_PHASE_DISCARD;

    return next;
}
