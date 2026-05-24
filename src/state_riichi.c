#include "state_riichi.h"

#include "state_discard.h"
#include "state_ops.h"
#include "state_score.h"

#include <assert.h>
#include <stddef.h>

static bool
cj4_state_is_closed_for_riichi(const cj4_mahjong *state, cj4_player player)
{
    for (uint8_t i = 0; i < state->meld_count[player]; ++i)
    {
        if (state->melds[player][i].type != CJ4_MELD_ANKAN)
            return false;
    }

    return true;
}

bool cj4_can_riichi(
    const cj4_mahjong *state,
    cj4_tile_id tile)
{
    cj4_mahjong tmp;
    cj4_player player;

    if (state->phase != CJ4_PHASE_DRAW)
        return false;

    player = state->current_player;

    if (state->is_riichi[player])
        return false;

    if (state->scores[player] < 1000)
        return false;

    if (!cj4_state_is_closed_for_riichi(state, player))
        return false;

    if (!cj4_can_discard(*state, tile))
        return false;

    tmp = *state;
    cj4_state_record_discard(&tmp, tile, (uint8_t)(tile == state->draw_tile));
    cj4_state_clear_draw_tile(&tmp);
    tmp.winning_from_chankan = 0;
    tmp.pending_kakan_tile = CJ4_TILE_ID_INVALID;
    tmp.phase = CJ4_PHASE_DISCARD;

    return cj4_player_is_shape_tenpai(&tmp, player);
}

cj4_mahjong
cj4_do_riichi(
    const cj4_mahjong state,
    cj4_tile_id tile)
{
    assert(cj4_can_riichi(&state, tile));

    cj4_mahjong next = state;
    cj4_player player = state.current_player;

    next.is_riichi[player] = 1;
    next.is_ippatsu[player] = 1;
    next.riichi_sticks++;
    next.scores[player] -= 1000;

    cj4_state_record_discard(&next, tile, (uint8_t)(tile == state.draw_tile));
    cj4_state_clear_draw_tile(&next);
    next.winning_from_chankan = 0;
    next.pending_kakan_tile = CJ4_TILE_ID_INVALID;

    if (state.first_turn_uninterrupted &&
        state.draw_turn_count[player] == 1)
    {
        next.riichi_declared_on_first_turn[player] = 1;
    }

    next.phase = CJ4_PHASE_DISCARD;

    return next;
}
