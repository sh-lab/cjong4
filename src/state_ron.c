#include "state_ron.h"
#include "hand_check.h"
#include "state_ops.h"
#include "state_query.h"
#include "state_yaku.h"
#include "tile.h"

#include <assert.h>

static uint8_t
cj4_ron_distance_from_discarder(
    cj4_player discarder,
    cj4_player player)
{
    return (uint8_t)((player + CJ4_PLAYER_COUNT - discarder) % CJ4_PLAYER_COUNT);
}

static uint8_t
cj4_ron_max_players(const cj4_rules *rules)
{
    if (!rules || rules->max_ron_players == 0 || rules->max_ron_players > 3)
        return 3;

    return rules->max_ron_players;
}

static uint8_t
cj4_ron_select_winners(
    const cj4_mahjong *state,
    const cj4_player *players,
    uint8_t count,
    const cj4_rules *rules,
    cj4_player selected[CJ4_PLAYER_COUNT])
{
    uint8_t max_players = cj4_ron_max_players(rules);

    if (count > CJ4_PLAYER_COUNT)
        count = CJ4_PLAYER_COUNT;

    for (uint8_t i = 0; i < count; ++i)
        selected[i] = players[i];

    for (uint8_t i = 0; i < count; ++i)
    {
        uint8_t best = i;

        for (uint8_t j = (uint8_t)(i + 1); j < count; ++j)
        {
            uint8_t dist_best =
                cj4_ron_distance_from_discarder(state->current_player, selected[best]);
            uint8_t dist_j =
                cj4_ron_distance_from_discarder(state->current_player, selected[j]);

            if (dist_j < dist_best)
                best = j;
        }

        if (best != i)
        {
            cj4_player tmp = selected[i];
            selected[i] = selected[best];
            selected[best] = tmp;
        }
    }

    return count < max_players ? count : max_players;
}

static uint8_t
cj4_state_player_has_permanent_furiten(
    const cj4_mahjong *state,
    cj4_player player)
{
    uint8_t waits[CJ4_TILE_TYPE_COUNT];

    if (cj4_collect_waiting_tile_types(state, player, waits) == 0)
        return 0;

    for (uint8_t i = 0; i < state->discard_count; ++i)
    {
        const cj4_discard *d = &state->discards[i];

        if (d->player != player)
            continue;

        if (waits[cj4_tile_get_type(d->tile)])
            return 1;
    }

    return 0;
}

bool cj4_can_ron(
    const cj4_mahjong *state,
    cj4_player player,
    const cj4_rules *rules)
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

    if (state->temporary_furiten[player] ||
        state->riichi_furiten[player] ||
        cj4_state_player_has_permanent_furiten(state, player))
        return false;

    return cj4_has_yaku(&tmp, player, rules);
}

cj4_mahjong
cj4_do_ron_multi(
    const cj4_mahjong state,
    const cj4_player *players,
    int count,
    const cj4_rules *rules)
{
    assert(count > 0);

    cj4_mahjong next = state;
    cj4_player winners[CJ4_PLAYER_COUNT];
    uint8_t winner_count;
    cj4_tile_id winning_tile =
        state.phase == CJ4_PHASE_KAKAN_RESOLVE
            ? state.pending_kakan_tile
            : cj4_get_last_discard_tile(&state);

    winner_count = cj4_ron_select_winners(
        &state,
        players,
        (uint8_t)count,
        rules,
        winners);

    cj4_state_finish_multi_ron(
        &next,
        winners,
        winner_count,
        winning_tile);

    return next;
}
