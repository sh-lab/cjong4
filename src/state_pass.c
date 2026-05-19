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
    next.draw_tile = cj4_state_draw_tile(&next, next.current_player);

    next.phase = CJ4_PHASE_DRAW;

    return next;
}