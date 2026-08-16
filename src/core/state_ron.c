#include "state_ron.h"
#include "hand_check.h"
#include "state_ops.h"
#include "state_query.h"
#include "state_yaku.h"
#include "tile.h"
#include "tile_const.h"

#include <assert.h>

static uint8_t
cj4_ron_distance_from_discarder(
    cj4_player discarder,
    cj4_player player)
{
    return (uint8_t)((player + CJ4_PLAYER_COUNT - discarder) % CJ4_PLAYER_COUNT);
}

static uint8_t
cj4_ron_max_players(
    const cj4_rules *rules)
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
                cj4_ron_distance_from_discarder(cj4_state_current_player(state), selected[best]);
            uint8_t dist_j =
                cj4_ron_distance_from_discarder(cj4_state_current_player(state), selected[j]);

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

    cj4_discard discards[CJ4_MAX_DISCARDS];
    uint8_t count = cj4_collect_discards(state, discards);
    for (uint8_t i = 0; i < count; ++i)
    {
        const cj4_discard *d = &discards[i];

        if (d->player != player)
            continue;

        if (waits[cj4_tile_get_type(d->tile)])
            return 1;
    }

    return 0;
}

static uint8_t
cj4_state_player_can_kokushi_with_tile(
    const cj4_mahjong *state,
    cj4_player player,
    cj4_tile_id tile)
{
    static const cj4_tile_type yaochu[] = {
        CJ4_TILE_TYPE_1M,
        CJ4_TILE_TYPE_9M,
        CJ4_TILE_TYPE_1P,
        CJ4_TILE_TYPE_9P,
        CJ4_TILE_TYPE_1S,
        CJ4_TILE_TYPE_9S,
        CJ4_TILE_TYPE_EAST,
        CJ4_TILE_TYPE_SOUTH,
        CJ4_TILE_TYPE_WEST,
        CJ4_TILE_TYPE_NORTH,
        CJ4_TILE_TYPE_HAKU,
        CJ4_TILE_TYPE_HATSU,
        CJ4_TILE_TYPE_CHUN};
    uint8_t counts[CJ4_TILE_TYPE_COUNT] = {0};
    uint8_t pair_count = 0;

    if (cj4_count_melds(state, player) != 0 ||
        !cj4_tile_is_yaochu(tile))
    {
        return 0;
    }

    for (cj4_tile_id id = CJ4_TILE_ID_MIN; id <= CJ4_TILE_ID_MAX; ++id)
    {
        const cj4_location *loc = cj4_tile_location_const(state, id);

        if (cj4_location_is_hand(loc->placement) &&
            cj4_location_placement_player(loc->placement) == player)
            counts[cj4_tile_get_type(id)]++;
    }

    counts[cj4_tile_get_type(tile)]++;

    for (uint8_t i = 0; i < (uint8_t)(sizeof(yaochu) / sizeof(yaochu[0])); ++i)
    {
        uint8_t count = counts[yaochu[i]];

        if (count == 0)
            return 0;

        if (count >= 2)
            pair_count++;
    }

    return pair_count == 1;
}

bool
cj4_can_ron(
    const cj4_mahjong *state,
    cj4_player player,
    const cj4_rules *rules)
{
    cj4_tile_id tile;

    if (cj4_state_phase(state) == CJ4_PHASE_DISCARD)
    {
        if (!cj4_state_can_claim_discard(state, player))
            return false;

        tile = cj4_get_last_discard_tile(state);
    }
    else if (cj4_state_phase(state) == CJ4_PHASE_KAKAN_RESOLVE)
    {
        if (player == cj4_state_current_player(state) ||
            state->pending_kakan_tile == CJ4_TILE_ID_INVALID)
            return false;

        tile = state->pending_kakan_tile;
    }
    else if (cj4_state_phase(state) == CJ4_PHASE_ANKAN_RESOLVE)
    {
        if (!rules || !rules->kokushi_ron_on_ankan ||
            player == cj4_state_current_player(state) ||
            state->pending_ankan_tile == CJ4_TILE_ID_INVALID)
        {
            return false;
        }

        tile = state->pending_ankan_tile;
    }
    else
    {
        return false;
    }

    /* 4. create temporary state and add tile to player's hand */
    cj4_mahjong tmp = *state;
    cj4_state_set_hand_location(&tmp, tile, player);
    cj4_state_set_chankan(&tmp, (uint8_t)(cj4_state_phase(state) == CJ4_PHASE_KAKAN_RESOLVE));

    if (cj4_state_temporary_furiten(state, player) ||
        cj4_state_riichi_furiten(state, player) ||
        cj4_state_player_has_permanent_furiten(state, player))
        return false;

    if (cj4_state_phase(state) == CJ4_PHASE_ANKAN_RESOLVE)
        return cj4_state_player_can_kokushi_with_tile(state, player, tile);

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
        cj4_state_phase(&state) == CJ4_PHASE_KAKAN_RESOLVE
            ? state.pending_kakan_tile
            : (cj4_state_phase(&state) == CJ4_PHASE_ANKAN_RESOLVE
                   ? state.pending_ankan_tile
                   : cj4_get_last_discard_tile(&state));

    if (rules &&
        rules->triple_ron_abortive_draw &&
        cj4_ron_max_players(rules) == 3 &&
        count >= 3)
    {
        cj4_state_finish_abortive_draw(&next, CJ4_ABORTIVE_DRAW_TRIPLE_RON);
        return next;
    }

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
