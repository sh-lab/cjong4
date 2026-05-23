#include "state_ron.h"
#include "state_query.h"
#include "state_yaku.h"
#include "state_ops.h"
#include "tile.h"

#include <assert.h>

bool
cj4_can_ron(
    const cj4_mahjong* state,
    cj4_player player,
    const cj4_rules* rules
)
{
    cj4_tile_id tile;

    if (state->phase == CJ4_PHASE_DISCARD)
    {
        if (!cj4_state_can_claim_discard(state, player))
            return false;

        tile = cj4_get_last_discard_tile(state);
    }
    else if (state->phase == CJ4_PHASE_KAKAN_RESOLVE)
    {
        if (player == state->current_player ||
            state->pending_kakan_tile == CJ4_TILE_ID_INVALID)
            return false;

        tile = state->pending_kakan_tile;
    }
    else
    {
        return false;
    }

    /* 4. create temporary state and add tile to player's hand */
    cj4_mahjong tmp = *state;
    cj4_state_set_location(&tmp, tile, CJ4_ZONE_HAND, player);
    tmp.winning_from_chankan = (uint8_t)(state->phase == CJ4_PHASE_KAKAN_RESOLVE);

    /* Furiten is evaluated on the original discard history before yaku/shape checks. */
    if (rules && rules->furiten) {
        cj4_tile_type last_type = cj4_tile_get_type(tile);
        for (uint8_t i = 0; i < state->discard_count; ++i) {
            const cj4_discard *d = &state->discards[i];
            if (d->player == player) {
                if (cj4_tile_get_type(d->tile) == last_type)
                    return false;
            }
        }
    }

    return cj4_has_yaku(&tmp, player, rules);
}

cj4_mahjong
cj4_do_ron_multi(
    const cj4_mahjong state,
    const cj4_player* players,
    int count
)
{
    assert(count > 0);

    cj4_mahjong next = state;
    cj4_tile_id winning_tile =
        state.phase == CJ4_PHASE_KAKAN_RESOLVE
            ? state.pending_kakan_tile
            : cj4_get_last_discard_tile(&state);

    cj4_state_finish_multi_ron(
        &next,
        players,
        (uint8_t)count,
        winning_tile);

    return next;
}
