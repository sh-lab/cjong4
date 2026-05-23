#include "state_tsumo.h"
#include "state_query.h"
#include "hand_check.h"
#include <assert.h>

bool cj4_can_tsumo(const cj4_mahjong *state, const cj4_rules *rules)
{
    if (state->phase != CJ4_PHASE_DRAW)
        return false;

    if (state->draw_tile == CJ4_TILE_ID_INVALID)
        return false;

    /* Validate hand shape (yaku validation is separate). */
    if (!cj4_is_complete_hand(state, state->current_player))
        return false;
    /* 8. yaku check using cj4_has_yaku */
    if (!cj4_has_yaku(state, state->current_player, rules))
        return false;

    return true;
}

cj4_mahjong cj4_do_tsumo(const cj4_mahjong state)
{

    cj4_mahjong next = state;

    next.winner = state.current_player;
    next.winning_tile = state.draw_tile;

    /* Per draft: do NOT modify wall, do NOT clear draw_tile, do NOT change hand/melds. */
    next.phase = CJ4_PHASE_ROUND_END;

    return next;
}
