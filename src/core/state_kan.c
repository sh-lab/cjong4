#include "state_kan.h"
#include "hand_check.h"
#include "state_internal.h"
#include "state_ops.h"
#include "state_query.h"
#include "state_ron.h"

#include <assert.h>
#include <string.h>

static uint8_t
cj4_state_can_declare_more_kans(
    const cj4_mahjong *state)
{
    uint8_t total = cj4_state_count_total_kans(state);

    if (total >= 5)
        return 0;

    /* The fifth declaration ends the round and needs no rinshan tile. */
    if (total == 4)
        return 1;

    return state->dead_wall_draw_count < 4 &&
           cj4_state_live_wall_remaining(state) > 0;
}

static uint8_t
cj4_state_should_abort_on_four_kans(
    const cj4_mahjong *state,
    const cj4_rules *rules)
{
    uint8_t total = cj4_state_count_total_kans(state);

    if (total >= 5)
        return 1;

    if (total < 4 || cj4_state_all_kans_by_one_player(state))
        return 0;

    return !rules ||
           rules->four_kans_abort_timing == CJ4_FOUR_KANS_ABORT_IMMEDIATE;
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

static void
cj4_state_commit_pending_ankan(
    cj4_mahjong *state)
{
    cj4_player player = cj4_state_current_player(state);
    const cj4_tile_id meld_tiles[4] = {
        state->pending_ankan_tiles[0],
        state->pending_ankan_tiles[1],
        state->pending_ankan_tiles[2],
        state->pending_ankan_tiles[3]};

    if (state->pending_ankan_tile == CJ4_TILE_ID_INVALID)
        return;

    cj4_state_add_meld(
        state,
        player,
        CJ4_MELD_ANKAN,
        meld_tiles,
        4,
        player,
        CJ4_CALLED_INDEX_NONE);

    cj4_state_add_dora_indicator(state);
    state->pending_ankan_tile = CJ4_TILE_ID_INVALID;
    for (uint8_t i = 0; i < 4; ++i)
        state->pending_ankan_tiles[i] = CJ4_TILE_ID_INVALID;
}

static uint8_t
cj4_can_ankan_after_riichi(
    const cj4_mahjong *state,
    const cj4_tile_id tiles[4])
{
    cj4_player player = cj4_state_current_player(state);
    uint8_t waits_before[CJ4_TILE_TYPE_COUNT];
    uint8_t waits_after[CJ4_TILE_TYPE_COUNT];
    cj4_mahjong before;
    cj4_mahjong after;

    if (!cj4_state_is_riichi(state, player))
        return 1;

    if (state->draw_tile == CJ4_TILE_ID_INVALID ||
        !cj4_kan_tiles_include(tiles, state->draw_tile))
        return 0;

    before = *state;
    before.locations[state->draw_tile].placement = CJ4_LOCATION_NONE;
    before.draw_tile = CJ4_TILE_ID_INVALID;

    if (cj4_collect_shape_wait_flags(&before, player, waits_before) == 0)
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

    if (cj4_collect_shape_wait_flags(&after, player, waits_after) == 0)
        return 0;

    return cj4_wait_sets_equal(waits_before, waits_after);
}

/* Minkan implementation (open kan using last discard) */
bool
cj4_can_minkan(
    const cj4_mahjong *state,
    cj4_player player)
{
    if (cj4_state_four_kans_abort_is_pending(state))
        return false;

    if (!cj4_state_can_declare_more_kans(state))
        return false;

    if (cj4_state_is_riichi(state, player))
        return false;

    if (!cj4_state_can_claim_discard(state, player))
        return false;

    cj4_tile_type last_discard_tile_type = cj4_tile_get_type(cj4_get_last_discard_tile(state));

    if (cj4_count_hand(state, player, last_discard_tile_type) < 3)
        return false;

    return true;
}

bool
cj4_can_minkan_with_tile(
    const cj4_mahjong *state,
    cj4_player player,
    cj4_tile_id tile1,
    cj4_tile_id tile2,
    cj4_tile_id tile3)
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
cj4_do_minkan(
    const cj4_mahjong state,
    const cj4_rules *rules,
    cj4_player player,
    cj4_tile_id tile1,
    cj4_tile_id tile2,
    cj4_tile_id tile3)
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
        cj4_state_current_player(&state),
        0);
    cj4_state_establish_pending_riichi(&next);
    cj4_state_finish_open_call(&next, player, CJ4_PHASE_DRAW);
    cj4_state_set_first_turn(&next, 0);
    cj4_state_set_chankan(&next, 0);
    next.pending_kakan_tile = CJ4_TILE_ID_INVALID;
    next.pending_ankan_tile = CJ4_TILE_ID_INVALID;

    if (!rules || rules->kan_dora_timing == CJ4_KAN_DORA_EARLY)
        cj4_state_add_dora_indicator(&next);
    else if (next.pending_kan_dora_count < CJ4_MAX_DORA)
        next.pending_kan_dora_count++;

    if (cj4_state_should_abort_on_four_kans(&next, rules))
    {
        cj4_state_finish_abortive_draw(&next, CJ4_ABORTIVE_DRAW_FOUR_KANS);
        return next;
    }

    next.draw_tile = cj4_state_draw_dead_wall_tile(&next, player);

    return next;
}

/* Ankan (closed kan) */
bool
cj4_can_ankan(
    const cj4_mahjong *state)
{
    cj4_player player = cj4_state_current_player(state);
    if (cj4_state_phase(state) != CJ4_PHASE_DRAW)
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

bool
cj4_can_ankan_with_tile(
    const cj4_mahjong *state,
    cj4_tile_id tile1,
    cj4_tile_id tile2,
    cj4_tile_id tile3,
    cj4_tile_id tile4)
{
    if (!cj4_can_ankan(state))
        return false;

    cj4_player player = cj4_state_current_player(state);

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
    cj4_player player = cj4_state_current_player(&state);

    cj4_state_reveal_pending_kan_dora(&next);
    cj4_state_set_current_player(&next, player);
    cj4_state_set_first_turn(&next, 0);
    cj4_state_set_chankan(&next, 0);
    next.pending_kakan_tile = CJ4_TILE_ID_INVALID;
    next.pending_ankan_tile = tile1;
    next.pending_ankan_tiles[0] = tile1;
    next.pending_ankan_tiles[1] = tile2;
    next.pending_ankan_tiles[2] = tile3;
    next.pending_ankan_tiles[3] = tile4;

    cj4_state_set_phase(&next, CJ4_PHASE_ANKAN_RESOLVE);

    if (cj4_state_count_total_kans(&state) == 4)
    {
        cj4_state_commit_pending_ankan(&next);
        cj4_state_clear_draw_tile(&next);
        cj4_state_clear_all_ippatsu(&next);
        cj4_state_finish_abortive_draw(&next, CJ4_ABORTIVE_DRAW_FOUR_KANS);
    }

    return next;
}

/* Kakan (added kan to existing pon) */
bool
cj4_can_kakan(
    const cj4_mahjong *state)
{
    cj4_player player = cj4_state_current_player(state);

    if (cj4_state_phase(state) != CJ4_PHASE_DRAW)
        return false;

    if (!cj4_state_can_declare_more_kans(state))
        return false;

    if (cj4_state_is_riichi(state, player))
        return false;

    cj4_meld_list melds =
        cj4_location_collect_melds(state->locations, player);
    for (uint8_t i = 0; i < melds.count; ++i)
    {
        const cj4_meld *m = &melds.items[i];
        if (m->type == CJ4_MELD_PON)
        {
            cj4_tile_type ttype = cj4_tile_get_type(m->tiles[0]);
            if (cj4_count_hand(state, player, ttype) >= 1)
                return true;
        }
    }

    return false;
}

bool
cj4_can_kakan_with_tile(
    const cj4_mahjong *state,
    cj4_tile_id tile)
{
    cj4_player player = cj4_state_current_player(state);

    if (cj4_state_phase(state) != CJ4_PHASE_DRAW)
        return false;

    if (!cj4_state_can_declare_more_kans(state))
        return false;

    if (cj4_state_is_riichi(state, player))
        return false;

    if (!cj4_state_tile_is_in_hand(state, player, tile))
        return false;

    cj4_tile_type type = cj4_tile_get_type(tile);

    cj4_meld_list melds =
        cj4_location_collect_melds(state->locations, player);
    for (uint8_t i = 0; i < melds.count; ++i)
    {
        const cj4_meld *m = &melds.items[i];
        if (m->type == CJ4_MELD_PON && cj4_tile_get_type(m->tiles[0]) == type)
            return true;
    }

    return false;
}

cj4_mahjong
cj4_do_kakan(
    const cj4_mahjong state,
    cj4_tile_id tile)
{
    assert(cj4_can_kakan_with_tile(&state, tile));
    cj4_mahjong next = state;
    cj4_player player = cj4_state_current_player(&state);

    cj4_state_reveal_pending_kan_dora(&next);

    /* Find the matching pon group and convert all its tiles to kakan. */
    cj4_meld_list melds =
        cj4_location_collect_melds(next.locations, player);
    for (uint8_t i = 0; i < melds.count; ++i)
    {
        const cj4_meld *meld = &melds.items[i];
        uint8_t group;

        if (meld->type != CJ4_MELD_PON ||
            cj4_tile_get_type(meld->tiles[0]) != cj4_tile_get_type(tile))
            continue;

        group = cj4_location_meld_group(
            next.locations[meld->tiles[0]].placement);
        for (uint8_t j = 0; j < meld->size; ++j)
            next.locations[meld->tiles[j]].placement =
                cj4_location_make_meld(player, group, CJ4_MELD_KAKAN);
        next.locations[tile].placement =
            cj4_location_make_meld(player, group, CJ4_MELD_KAKAN);
        cj4_state_update_pao(&next, player, meld->from_player);
        break;
    }

    /* Do not add dora or draw here; resolve in separate phase */
    /* current_player stays unchanged for kakan */
    cj4_state_clear_draw_tile(&next);
    cj4_state_clear_all_ippatsu(&next);
    cj4_state_set_first_turn(&next, 0);
    cj4_state_set_chankan(&next, 0);
    next.pending_kakan_tile = tile;
    next.pending_ankan_tile = CJ4_TILE_ID_INVALID;
    cj4_state_set_phase(&next, CJ4_PHASE_KAKAN_RESOLVE);

    if (cj4_state_count_total_kans(&state) == 4)
        cj4_state_finish_abortive_draw(&next, CJ4_ABORTIVE_DRAW_FOUR_KANS);

    return next;
}

bool
cj4_can_rinshan_draw(
    const cj4_mahjong *state)
{
    return ((cj4_state_phase(state) == CJ4_PHASE_ANKAN_RESOLVE &&
             cj4_tile_id_is_valid(state->pending_ankan_tile)) ||
            (cj4_state_phase(state) == CJ4_PHASE_KAKAN_RESOLVE &&
             cj4_tile_id_is_valid(state->pending_kakan_tile))) &&
           state->dead_wall_draw_count < 4 &&
           cj4_state_live_wall_remaining(state) > 0;
}

/* Kan resolution after ankan/kakan chankan reactions. */
cj4_mahjong
cj4_do_rinshan_draw(
    const cj4_mahjong state,
    const cj4_rules *rules)
{
    assert(cj4_can_rinshan_draw(&state));
    cj4_mahjong next = state;
    cj4_player player = cj4_state_current_player(&state);

    if (cj4_state_phase(&state) == CJ4_PHASE_KAKAN_RESOLVE ||
        (cj4_state_phase(&state) == CJ4_PHASE_ANKAN_RESOLVE &&
         state.pending_ankan_tile != CJ4_TILE_ID_INVALID))
    {
        for (uint8_t other = 0; other < CJ4_PLAYER_COUNT; ++other)
        {
            if (other == player)
                continue;

            if (!cj4_can_ron(&state, (cj4_player)other, rules))
                continue;

            if (cj4_state_is_riichi(&state, (cj4_player)other))
                cj4_state_set_riichi_furiten(&next, (cj4_player)other, true);
            else
                cj4_state_set_temporary_furiten(&next, (cj4_player)other, true);
        }
    }

    if (cj4_state_should_abort_on_four_kans(&state, rules))
    {
        next.pending_kakan_tile = CJ4_TILE_ID_INVALID;
        cj4_state_set_chankan(&next, 0);
        cj4_state_finish_abortive_draw(&next, CJ4_ABORTIVE_DRAW_FOUR_KANS);
        return next;
    }

    if (cj4_state_phase(&state) == CJ4_PHASE_ANKAN_RESOLVE &&
        next.pending_ankan_tile != CJ4_TILE_ID_INVALID)
    {
        cj4_state_commit_pending_ankan(&next);
        cj4_state_clear_draw_tile(&next);
        cj4_state_clear_all_ippatsu(&next);
    }

    if (cj4_state_should_abort_on_four_kans(&next, rules))
    {
        next.pending_kakan_tile = CJ4_TILE_ID_INVALID;
        cj4_state_set_chankan(&next, 0);
        cj4_state_finish_abortive_draw(&next, CJ4_ABORTIVE_DRAW_FOUR_KANS);
        return next;
    }

    if (cj4_state_phase(&state) == CJ4_PHASE_KAKAN_RESOLVE)
    {
        if (!rules || rules->kan_dora_timing == CJ4_KAN_DORA_EARLY)
            cj4_state_add_dora_indicator(&next);
        else if (next.pending_kan_dora_count < CJ4_MAX_DORA)
            next.pending_kan_dora_count++;
    }

    /* Draw rinshan tile to hand of current player */
    cj4_tile_id t = cj4_state_draw_dead_wall_tile(&next, player);
    next.draw_tile = t;

    cj4_state_set_chankan(&next, 0);
    next.pending_kakan_tile = CJ4_TILE_ID_INVALID;
    next.pending_ankan_tile = CJ4_TILE_ID_INVALID;
    cj4_state_set_phase(&next, CJ4_PHASE_DRAW);

    return next;
}
