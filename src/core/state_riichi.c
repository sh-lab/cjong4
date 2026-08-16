#include "state_riichi.h"

#include "state_discard.h"
#include "state_ops.h"
#include "state_query.h"
#include "state_score.h"

#include <assert.h>
#include <stddef.h>

static bool
cj4_state_is_closed_for_riichi(
    const cj4_mahjong *state,
    cj4_player player)
{
    cj4_meld melds[CJ4_MAX_MELDS];
    uint8_t count = cj4_collect_melds(state, player, melds);
    for (uint8_t i = 0; i < count; ++i)
    {
        if (melds[i].type != CJ4_MELD_ANKAN)
            return false;
    }

    return true;
}

bool
cj4_can_riichi(
    const cj4_mahjong *state,
    cj4_tile_id tile)
{
    cj4_mahjong tmp;
    cj4_player player;

    if (cj4_state_phase(state) != CJ4_PHASE_DRAW)
        return false;

    player = cj4_state_current_player(state);

    if (cj4_state_is_riichi(state, player) ||
        cj4_state_has_pending_riichi(state))
        return false;

    if (state->scores[player] < 1000)
        return false;

    if (cj4_state_live_wall_remaining(state) < CJ4_PLAYER_COUNT)
        return false;

    if (!cj4_state_is_closed_for_riichi(state, player))
        return false;

    if (!cj4_can_discard(*state, tile))
        return false;

    tmp = *state;
    cj4_state_record_discard(
        &tmp,
        tile,
        (uint8_t)(tile == state->draw_tile),
        1);
    cj4_state_clear_draw_tile(&tmp);
    cj4_state_set_chankan(&tmp, 0);
    tmp.pending_kakan_tile = CJ4_TILE_ID_INVALID;
    cj4_state_set_phase(&tmp, CJ4_PHASE_DISCARD);

    return cj4_player_is_shape_tenpai(&tmp, player);
}

cj4_mahjong
cj4_do_riichi(
    const cj4_mahjong state,
    cj4_tile_id tile)
{
    assert(cj4_can_riichi(&state, tile));

    cj4_mahjong next = state;
    cj4_player player = cj4_state_current_player(&state);

    cj4_state_record_discard(
        &next,
        tile,
        (uint8_t)(tile == state.draw_tile),
        1);
    cj4_state_clear_draw_tile(&next);
    cj4_state_set_chankan(&next, 0);
    next.pending_kakan_tile = CJ4_TILE_ID_INVALID;
    next.pending_ankan_tile = CJ4_TILE_ID_INVALID;
    cj4_state_set_pending_riichi(
        &next,
        player,
        cj4_state_first_turn(&state) &&
            cj4_state_draw_turn(&state, player) == 1);

    cj4_state_set_phase(&next, CJ4_PHASE_DISCARD);

    return next;
}
