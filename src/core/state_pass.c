#include "state_pass.h"
#include "state_ops.h"
#include "state_query.h"
#include "state_ron.h"

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
cj4_do_pass(
    const cj4_mahjong state,
    const cj4_rules *rules)
{
    cj4_mahjong next = state;

    for (uint8_t player = 0; player < CJ4_PLAYER_COUNT; ++player)
    {
        if (player == state.current_player)
            continue;

        if (!cj4_can_ron(&state, (cj4_player)player, rules))
            continue;

        if (state.is_riichi[player])
            next.riichi_furiten[player] = 1;
        else
            next.temporary_furiten[player] = 1;
    }

    if (next.discard_count >= CJ4_MAX_DRAWS)
    {
        cj4_state_finish_draw_round(&next, CJ4_ROUND_END_EXHAUSTIVE_DRAW);

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