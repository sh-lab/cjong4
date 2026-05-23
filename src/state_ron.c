#include "state_ron.h"
#include "state_query.h"
#include "hand_check.h"
#include "state_yaku.h"
#include "tile.h"

#include <assert.h>

bool
cj4_can_ron(
    const cj4_mahjong* state,
    cj4_player player,
    const cj4_rules* rules
)
{
    /* 1. phase must be DISCARD */
    if (state->phase != CJ4_PHASE_DISCARD)
        return false;

    /* 2. cannot ron own discard */
    if (player == state->current_player)
        return false;

    /* 3. last discard must be valid */
    cj4_tile_id tile = cj4_get_last_discard_tile(state);
    if (tile == CJ4_TILE_ID_INVALID)
        return false;

    /* 4. create temporary state and add tile to player's hand */
    cj4_mahjong tmp = *state;
    tmp.locations[tile].zone = CJ4_ZONE_HAND;
    tmp.locations[tile].owner = player;

    /* 6. check hand shape */
    if (!cj4_is_complete_hand(&tmp, player))
        return false;

    /* 7. furiten check: if enabled in rules and player has previously discarded the same tile type, ron is not allowed */
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

    /* 8. yaku check using cj4_has_yaku */
    if (!cj4_has_yaku(&tmp, player, rules))
        return false;

    return true;
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

    /* Record loser (discarder) and winners for multi-ron. */
    next.loser = state.current_player;
    next.winner_count = (uint8_t)(count <= CJ4_PLAYER_COUNT ? count : CJ4_PLAYER_COUNT);
    for (int i = 0; i < next.winner_count; ++i)
        next.winners[i] = players[i];

    /* For backward compatibility, set winner to the first winner if any. */
    next.winner = next.winners[0];
    next.winning_tile = cj4_get_last_discard_tile(&state);

    /* Transition to round end. */
    next.phase = CJ4_PHASE_ROUND_END;

    return next;
}
