#include "state_pass.h"
#include "state_query.h"
#include "state_ops.h"

bool
cj4_can_pass(const cj4_mahjong state)
{
    if (state.phase != CJ4_PHASE_DISCARD)
    {
        return false;
    }

    return true;
}

cj4_mahjong
cj4_do_pass(const cj4_mahjong state)
{
    cj4_mahjong next = state;

    if (next.discard_count >= CJ4_MAX_DRAWS)
    {
        next.phase = CJ4_PHASE_ROUND_END;

        return next;
    }

    next.current_player = cj4_next_player(&state);
    if (next.draw_turn_count[next.current_player] > 0)
        next.first_turn_uninterrupted = 0;
    next.draw_tile = cj4_state_draw_tile(&next, next.current_player);
    next.draw_turn_count[next.current_player]++;
    next.winning_from_chankan = 0;
    next.pending_kakan_tile = CJ4_TILE_ID_INVALID;

    next.phase = CJ4_PHASE_DRAW;

    return next;
}