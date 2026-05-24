#include "state_kan.h"
#include "hand_check.h"
#include "state_internal.h"
#include "state_ops.h"
#include "state_query.h"
#include "state_ron.h"

#include <assert.h>
#include <string.h>

static uint8_t
cj4_state_can_declare_more_kans(const cj4_mahjong *state)
{
    return cj4_state_count_total_kans(state) < 4;
}

static uint8_t
cj4_state_should_abort_on_four_kans(const cj4_mahjong *state)
{
    return cj4_state_count_total_kans(state) >= 4 &&
           !cj4_state_all_kans_by_one_player(state);
}

static uint8_t
cj4_kan_tiles_include(
    const cj4_tile_id tiles[4],
    cj4_tile_id tile)
{
    for (uint8_t i = 0; i < 4; ++i)
    {
        if (tiles[i] == tile)
            return 1;
    }

    return 0;
}

static uint8_t
cj4_wait_sets_equal(
    const uint8_t lhs[CJ4_TILE_TYPE_COUNT],
    const uint8_t rhs[CJ4_TILE_TYPE_COUNT])
{
    return memcmp(lhs, rhs, CJ4_TILE_TYPE_COUNT) == 0;
}

static uint8_t
cj4_find_hand_tiles_of_type(
    const cj4_mahjong *state,
    cj4_player player,
    cj4_tile_type type,
    cj4_tile_id out[4])
{
    uint8_t count = 0;

    for (uint8_t index = 0; index < CJ4_TILE_PER_TYPE; ++index)
    {
        cj4_tile_id tile = cj4_tile_make(type, index);
        if (!cj4_state_tile_is_in_hand(state, player, tile))
            continue;

        out[count++] = tile;
    }

    return count;
}

static uint8_t
cj4_can_ankan_after_riichi(
    const cj4_mahjong *state,
    const cj4_tile_id tiles[4])
{
    cj4_player player = state->current_player;
    uint8_t waits_before[CJ4_TILE_TYPE_COUNT];
    uint8_t waits_after[CJ4_TILE_TYPE_COUNT];
    cj4_mahjong before;
    cj4_mahjong after;

    if (!state->is_riichi[player])
        return 1;

    if (state->draw_tile == CJ4_TILE_ID_INVALID ||
        !cj4_kan_tiles_include(tiles, state->draw_tile))
        return 0;

    before = *state;
    before.locations[state->draw_tile].zone = CJ4_ZONE_WALL;
    before.draw_tile = CJ4_TILE_ID_INVALID;

    if (cj4_collect_waiting_tile_types(&before, player, waits_before) == 0)
        return 0;

    after = before;
    cj4_state_add_meld(
        &after,
        player,
        CJ4_MELD_ANKAN,
        tiles,
        4,
        player,
        CJ4_CALLED_INDEX_NONE);

    if (cj4_collect_waiting_tile_types(&after, player, waits_after) == 0)
        return 0;

    return cj4_wait_sets_equal(waits_before, waits_after);
}

/* Minkan implementation (open kan using last discard) */
bool cj4_can_minkan(const cj4_mahjong *state, cj4_player player)
{
    if (!cj4_state_can_declare_more_kans(state))
        return false;

    if (state->is_riichi[player])
        return false;

    if (!cj4_state_can_claim_discard(state, player))
        return false;

    cj4_tile_type last_discard_tile_type = cj4_tile_get_type(cj4_get_last_discard_tile(state));

    if (cj4_count_hand(state, player, last_discard_tile_type) < 3)
        return false;

    return true;
}

bool cj4_can_minkan_with_tile(const cj4_mahjong *state, cj4_player player, cj4_tile_id tile1, cj4_tile_id tile2, cj4_tile_id tile3)
{
    if (!cj4_can_minkan(state, player))
        return false;

    if (tile1 == tile2 || tile1 == tile3 || tile2 == tile3)
        return false;

    cj4_tile_id last = cj4_get_last_discard_tile(state);
    cj4_tile_type type = cj4_tile_get_type(last);
    if (cj4_tile_get_type(tile1) != type ||
        cj4_tile_get_type(tile2) != type ||
        cj4_tile_get_type(tile3) != type)
        return false;

    if (!cj4_state_tile_is_in_hand(state, player, tile1) ||
        !cj4_state_tile_is_in_hand(state, player, tile2) ||
        !cj4_state_tile_is_in_hand(state, player, tile3))
        return false;

    return true;
}

cj4_mahjong
cj4_do_minkan(const cj4_mahjong state, cj4_player player, cj4_tile_id tile1, cj4_tile_id tile2, cj4_tile_id tile3)
{
    assert(cj4_can_minkan_with_tile(&state, player, tile1, tile2, tile3));
    cj4_mahjong next = state;
    cj4_tile_id last = cj4_get_last_discard_tile(&state);
    const cj4_tile_id meld_tiles[4] = {last, tile1, tile2, tile3};

    cj4_state_add_meld(
        &next,
        player,
        CJ4_MELD_MINKAN,
        meld_tiles,
        4,
        state.current_player,
        0);
    cj4_state_finish_open_call(&next, player, CJ4_PHASE_ANKAN_RESOLVE);
    next.first_turn_uninterrupted = 0;
    next.winning_from_chankan = 0;
    next.pending_kakan_tile = CJ4_TILE_ID_INVALID;

    return next;
}

/* Ankan (closed kan) */
bool cj4_can_ankan(const cj4_mahjong *state)
{
    cj4_player player = state->current_player;
    if (state->phase != CJ4_PHASE_DRAW)
        return false;

    if (!cj4_state_can_declare_more_kans(state))
        return false;

    for (cj4_tile_type type = 0; type < CJ4_TILE_TYPE_COUNT; ++type)
    {
        cj4_tile_id tiles[4];

        if (cj4_find_hand_tiles_of_type(state, player, type, tiles) < 4)
            continue;

        if (cj4_can_ankan_after_riichi(state, tiles))
            return true;
    }

    return false;
}

bool cj4_can_ankan_with_tile(const cj4_mahjong *state, cj4_tile_id tile1, cj4_tile_id tile2, cj4_tile_id tile3, cj4_tile_id tile4)
{
    if (!cj4_can_ankan(state))
        return false;

    cj4_player player = state->current_player;

    cj4_tile_type type = cj4_tile_get_type(tile1);

    if (cj4_tile_get_type(tile2) != type ||
        cj4_tile_get_type(tile3) != type ||
        cj4_tile_get_type(tile4) != type)
        return false;

    /* ensure distinct tile ids */
    if (tile1 == tile2 || tile1 == tile3 || tile1 == tile4 ||
        tile2 == tile3 || tile2 == tile4 || tile3 == tile4)
        return false;

    if (!cj4_state_tile_is_in_hand(state, player, tile1) ||
        !cj4_state_tile_is_in_hand(state, player, tile2) ||
        !cj4_state_tile_is_in_hand(state, player, tile3) ||
        !cj4_state_tile_is_in_hand(state, player, tile4))
        return false;

    if (!cj4_can_ankan_after_riichi(
            state,
            (const cj4_tile_id[4]){tile1, tile2, tile3, tile4}))
        return false;

    return true;
}

cj4_mahjong
cj4_do_ankan(
    const cj4_mahjong state,
    cj4_tile_id tile1,
    cj4_tile_id tile2,
    cj4_tile_id tile3,
    cj4_tile_id tile4)
{
    assert(cj4_can_ankan_with_tile(&state, tile1, tile2, tile3, tile4));
    cj4_mahjong next = state;
    cj4_player player = state.current_player;
    const cj4_tile_id meld_tiles[4] = {tile1, tile2, tile3, tile4};

    cj4_state_add_meld(
        &next,
        player,
        CJ4_MELD_ANKAN,
        meld_tiles,
        4,
        player,
        CJ4_CALLED_INDEX_NONE);
    cj4_state_clear_draw_tile(&next);
    cj4_state_clear_all_ippatsu(&next);

    next.current_player = player;
    next.first_turn_uninterrupted = 0;
    next.winning_from_chankan = 0;
    next.pending_kakan_tile = CJ4_TILE_ID_INVALID;

    if (cj4_state_should_abort_on_four_kans(&next))
    {
        cj4_state_finish_draw_round(&next, CJ4_ROUND_END_ABORTIVE_DRAW);
        return next;
    }

    next.draw_tile = cj4_state_draw_dead_wall_tile(&next, player);

    cj4_state_add_dora_indicator(&next);

    next.phase = CJ4_PHASE_DRAW;

    return next;
}

/* Kakan (added kan to existing pon) */
bool cj4_can_kakan(const cj4_mahjong *state)
{
    cj4_player player = state->current_player;

    if (state->phase != CJ4_PHASE_DRAW)
        return false;

    if (!cj4_state_can_declare_more_kans(state))
        return false;

    if (state->is_riichi[player])
        return false;

    for (uint8_t i = 0; i < state->meld_count[player]; ++i)
    {
        const cj4_meld *m = &state->melds[player][i];
        if (m->type == CJ4_MELD_PON)
        {
            cj4_tile_type ttype = cj4_tile_get_type(m->tiles[0]);
            if (cj4_count_hand(state, player, ttype) >= 1)
                return true;
        }
    }

    return false;
}

bool cj4_can_kakan_with_tile(const cj4_mahjong *state, cj4_tile_id tile)
{
    cj4_player player = state->current_player;

    if (state->phase != CJ4_PHASE_DRAW)
        return false;

    if (!cj4_state_can_declare_more_kans(state))
        return false;

    if (state->is_riichi[player])
        return false;

    if (!cj4_state_tile_is_in_hand(state, player, tile))
        return false;

    cj4_tile_type type = cj4_tile_get_type(tile);

    for (uint8_t i = 0; i < state->meld_count[player]; ++i)
    {
        const cj4_meld *m = &state->melds[player][i];
        if (m->type == CJ4_MELD_PON && cj4_tile_get_type(m->tiles[0]) == type)
            return true;
    }

    return false;
}

cj4_mahjong
cj4_do_kakan(const cj4_mahjong state, cj4_tile_id tile)
{
    assert(cj4_can_kakan_with_tile(&state, tile));
    cj4_mahjong next = state;
    cj4_player player = state.current_player;

    /* find the pon meld of matching type and convert it */
    for (uint8_t i = 0; i < next.meld_count[player]; ++i)
    {
        cj4_meld *m = &next.melds[player][i];
        if (m->type == CJ4_MELD_PON && cj4_tile_get_type(m->tiles[0]) == cj4_tile_get_type(tile))
        {
            m->tiles[3] = tile;
            m->size = 4;
            m->type = CJ4_MELD_KAKAN; /* convert from PON to KAKAN */
            /* called_index stays unchanged */

            cj4_state_set_location(&next, tile, CJ4_ZONE_MELD, player);

            break;
        }
    }

    /* Do not add dora or draw here; resolve in separate phase */
    /* current_player stays unchanged for kakan */
    cj4_state_clear_draw_tile(&next);
    cj4_state_clear_all_ippatsu(&next);
    next.first_turn_uninterrupted = 0;
    next.winning_from_chankan = 0;
    next.pending_kakan_tile = tile;
    next.phase = CJ4_PHASE_KAKAN_RESOLVE;

    return next;
}

bool cj4_can_rinshan_draw(const cj4_mahjong *state)
{
    return (state->phase == CJ4_PHASE_ANKAN_RESOLVE ||
            state->phase == CJ4_PHASE_KAKAN_RESOLVE) &&
           state->dead_wall_draw_count < 4;
}

/* Kan resolution: run once after any kan that defers draw/dora */
cj4_mahjong
cj4_do_rinshan_draw(const cj4_mahjong state, const cj4_rules *rules)
{
    assert(cj4_can_rinshan_draw(&state));
    cj4_mahjong next = state;
    cj4_player player = state.current_player;

    if (state.phase == CJ4_PHASE_KAKAN_RESOLVE)
    {
        for (uint8_t other = 0; other < CJ4_PLAYER_COUNT; ++other)
        {
            if (other == player)
                continue;

            if (!cj4_can_ron(&state, (cj4_player)other, rules))
                continue;

            if (state.is_riichi[other])
                next.riichi_furiten[other] = 1;
            else
                next.temporary_furiten[other] = 1;
        }
    }

    if (cj4_state_should_abort_on_four_kans(&state))
    {
        next.pending_kakan_tile = CJ4_TILE_ID_INVALID;
        next.winning_from_chankan = 0;
        cj4_state_finish_draw_round(&next, CJ4_ROUND_END_ABORTIVE_DRAW);
        return next;
    }

    /* Draw rinshan tile to hand of current player */
    cj4_tile_id t = cj4_state_draw_dead_wall_tile(&next, player);
    next.draw_tile = t;

    /* Increase dora indicator if possible */
    cj4_state_add_dora_indicator(&next);

    next.winning_from_chankan = 0;
    next.pending_kakan_tile = CJ4_TILE_ID_INVALID;
    next.phase = CJ4_PHASE_DRAW;

    return next;
}
