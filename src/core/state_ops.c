#include "state_ops.h"
#include "state_internal.h"
#include "state_query.h"
#include "tile_const.h"

#include <assert.h>

bool
cj4_state_has_last_discard(
    const cj4_mahjong *state)
{
    return state->discard_count > 0 &&
           state->last_discard_tile != CJ4_TILE_ID_INVALID;
}

bool
cj4_state_can_claim_discard(
    const cj4_mahjong *state,
    cj4_player player)
{
    return cj4_state_phase(state) == CJ4_PHASE_DISCARD &&
           player != cj4_state_current_player(state) &&
           cj4_state_has_last_discard(state);
}

bool
cj4_state_tile_is_in_hand(
    const cj4_mahjong *state,
    cj4_player player,
    cj4_tile_id tile)
{
    uint8_t placement;
    if (tile > CJ4_TILE_ID_MAX)
        return false;
    placement = state->locations[tile].placement;
    return cj4_location_is_hand(placement) &&
           cj4_location_placement_player(placement) == player;
}

uint8_t
cj4_state_live_wall_remaining(
    const cj4_mahjong *state)
{
    uint8_t dead_wall_draws = state->dead_wall_draw_count;
    uint8_t live_wall_end;
    if (dead_wall_draws > 4)
        dead_wall_draws = 4;
    live_wall_end = (uint8_t)(CJ4_LIVE_WALL_END - dead_wall_draws);
    return state->wall_pos >= live_wall_end
               ? 0
               : (uint8_t)(live_wall_end - state->wall_pos);
}

void
cj4_state_set_hand_location(
    cj4_mahjong *state,
    cj4_tile_id tile,
    cj4_player owner)
{
    state->locations[tile].placement = cj4_location_make_hand(owner);
}

uint8_t
cj4_state_count_total_kans(
    const cj4_mahjong *state)
{
    uint8_t total = 0;
    for (uint8_t player = 0; player < CJ4_PLAYER_COUNT; ++player)
    {
        cj4_meld melds[CJ4_MAX_MELDS];
        uint8_t count = cj4_collect_melds(state, (cj4_player)player, melds);
        for (uint8_t i = 0; i < count; ++i)
            if (melds[i].type == CJ4_MELD_MINKAN ||
                melds[i].type == CJ4_MELD_ANKAN ||
                melds[i].type == CJ4_MELD_KAKAN)
                total++;
    }
    return total;
}

uint8_t
cj4_state_all_kans_by_one_player(
    const cj4_mahjong *state)
{
    uint8_t owner = CJ4_PLAYER_COUNT;
    for (uint8_t player = 0; player < CJ4_PLAYER_COUNT; ++player)
    {
        cj4_meld melds[CJ4_MAX_MELDS];
        uint8_t count = cj4_collect_melds(state, (cj4_player)player, melds);
        for (uint8_t i = 0; i < count; ++i)
        {
            cj4_meld_type type = melds[i].type;
            if (type != CJ4_MELD_MINKAN && type != CJ4_MELD_ANKAN &&
                type != CJ4_MELD_KAKAN)
                continue;
            if (owner == CJ4_PLAYER_COUNT)
                owner = player;
            else if (owner != player)
                return 0;
        }
    }
    return 1;
}

static uint8_t
cj4_state_count_meld_triplets_in_range(
    const cj4_mahjong *state,
    cj4_player player,
    cj4_tile_type start,
    cj4_tile_type end)
{
    cj4_meld melds[CJ4_MAX_MELDS];
    uint8_t count = 0;
    uint8_t meld_count = cj4_collect_melds(state, player, melds);
    for (uint8_t i = 0; i < meld_count; ++i)
    {
        cj4_tile_type type;
        if (melds[i].type == CJ4_MELD_CHI)
            continue;
        type = cj4_tile_get_type(melds[i].tiles[0]);
        if (type >= start && type <= end)
            count++;
    }
    return count;
}

static uint8_t
cj4_state_count_player_kans(
    const cj4_mahjong *state,
    cj4_player player)
{
    cj4_meld melds[CJ4_MAX_MELDS];
    uint8_t count = 0;
    uint8_t meld_count = cj4_collect_melds(state, player, melds);
    for (uint8_t i = 0; i < meld_count; ++i)
        if (melds[i].type == CJ4_MELD_MINKAN ||
            melds[i].type == CJ4_MELD_ANKAN ||
            melds[i].type == CJ4_MELD_KAKAN)
            count++;
    return count;
}

void
cj4_state_update_pao(
    cj4_mahjong *state,
    cj4_player player,
    cj4_player from_player)
{
    if (from_player == player || cj4_state_pao_type(state, player) != CJ4_PAO_NONE)
        return;
    if (cj4_state_count_meld_triplets_in_range(
            state,
            player,
            CJ4_TILE_TYPE_HAKU,
            CJ4_TILE_TYPE_CHUN) >= 3)
    {
        cj4_state_set_pao(state, player, from_player, CJ4_PAO_DAISANGEN);
        return;
    }
    if (cj4_state_count_meld_triplets_in_range(
            state,
            player,
            CJ4_TILE_TYPE_EAST,
            CJ4_TILE_TYPE_NORTH) >= 4)
    {
        cj4_state_set_pao(state, player, from_player, CJ4_PAO_DAISUUSHII);
        return;
    }
    if (cj4_state_count_player_kans(state, player) >= 4)
        cj4_state_set_pao(state, player, from_player, CJ4_PAO_SUUKANTSU);
}

cj4_tile_id
cj4_state_draw_tile(
    cj4_mahjong *state,
    cj4_player player)
{
    cj4_tile_id tile;
    assert(cj4_state_live_wall_remaining(state) > 0);
    tile = cj4_get_wall_tile(state, state->wall_pos++);
    assert(tile != CJ4_TILE_ID_INVALID);
    cj4_state_set_hand_location(state, tile, player);
    cj4_state_set_temporary_furiten(state, player, false);
    return tile;
}

cj4_tile_id
cj4_state_draw_dead_wall_tile(
    cj4_mahjong *state,
    cj4_player player)
{
    cj4_tile_id tile;
    assert(state->dead_wall_draw_count < 4);
    tile = cj4_get_wall_tile(
        state,
        CJ4_RINSHAN_INDICES[state->dead_wall_draw_count++]);
    assert(tile != CJ4_TILE_ID_INVALID);
    cj4_state_set_hand_location(state, tile, player);
    cj4_state_set_temporary_furiten(state, player, false);
    return tile;
}

void
cj4_state_clear_draw_tile(
    cj4_mahjong *state)
{
    state->draw_tile = CJ4_TILE_ID_INVALID;
}

void
cj4_state_clear_all_ippatsu(
    cj4_mahjong *state)
{
    state->riichi_ippatsu &= 0x0fu;
}

void
cj4_state_establish_pending_riichi(
    cj4_mahjong *state)
{
    cj4_player player = cj4_state_pending_riichi_player(state);
    if (!cj4_state_has_pending_riichi(state) || player >= CJ4_PLAYER_COUNT)
        return;
    if (!cj4_state_is_riichi(state, player))
    {
        cj4_state_set_riichi(state, player, true);
        cj4_state_set_ippatsu(state, player, true);
        state->riichi_sticks++;
        state->scores[player] -= 1000;
        if (cj4_state_pending_riichi_is_double(state))
            cj4_state_set_double_riichi(state, player, true);
    }
    cj4_state_clear_pending_riichi(state);
}

void
cj4_state_clear_pending_riichi(
    cj4_mahjong *state)
{
    cj4_state_clear_pending_riichi_bits(state);
}

void
cj4_state_reveal_pending_kan_dora(
    cj4_mahjong *state)
{
    while (state->pending_kan_dora)
    {
        cj4_state_add_dora_indicator(state);
        state->pending_kan_dora--;
    }
}

void
cj4_state_record_discard(
    cj4_mahjong *state,
    cj4_tile_id tile,
    uint8_t is_tsumogiri,
    uint8_t is_riichi)
{
    cj4_player player = cj4_state_current_player(state);
    uint8_t player_index = 0;
    assert(state->discard_count <= CJ4_DISCARD_HISTORY_MAX);
    for (uint16_t id = 0; id < CJ4_TILE_ID_COUNT; ++id)
    {
        uint8_t discard = state->locations[id].discard;
        if (cj4_location_is_discard(discard) &&
            cj4_location_discard_player(discard) == player)
            player_index++;
    }
    assert(player_index <= CJ4_DISCARD_INDEX_MAX);
    state->locations[tile].discard =
        cj4_location_make_discard(player, player_index, is_tsumogiri != 0);
    state->locations[tile].placement = CJ4_LOCATION_NONE;
    state->locations[tile].discard_history =
        cj4_location_make_discard_history(state->discard_count, is_riichi != 0);
    state->last_discard_tile = tile;
    state->discard_count++;
}

void
cj4_state_consume_last_discard(
    cj4_mahjong *state)
{
    (void)state;
    /* A called discard remains in history; meld placement marks it inactive. */
}

void
cj4_state_add_meld(
    cj4_mahjong *state,
    cj4_player player,
    cj4_meld_type type,
    const cj4_tile_id *tiles,
    uint8_t size,
    cj4_player from_player,
    uint8_t called_index)
{
    uint8_t group = cj4_count_melds(state, player);
    assert(size == 3 || size == 4);
    assert(group < CJ4_MAX_MELDS);
    (void)from_player;
    (void)called_index;
    for (uint8_t i = 0; i < size; ++i)
        state->locations[tiles[i]].placement =
            cj4_location_make_meld(player, group, type);
    cj4_state_update_pao(state, player, from_player);
}

void
cj4_state_finish_open_call(
    cj4_mahjong *state,
    cj4_player player,
    cj4_phase next_phase)
{
    cj4_state_clear_draw_tile(state);
    cj4_state_clear_all_ippatsu(state);
    cj4_state_set_current_player(state, player);
    cj4_state_set_phase(state, next_phase);
}

void
cj4_state_add_dora_indicator(
    cj4_mahjong *state)
{
    if (state->dora_count < CJ4_MAX_DORA)
        state->dora_count++;
}

static void
cj4_state_clear_round_pending(
    cj4_mahjong *state)
{
    cj4_state_clear_pending_riichi(state);
    state->pending_kakan_tile = CJ4_TILE_ID_INVALID;
    state->pending_ankan_tile = CJ4_TILE_ID_INVALID;
    for (uint8_t i = 0; i < 4; ++i)
        state->pending_ankan_tiles[i] = CJ4_TILE_ID_INVALID;
    state->pending_kan_dora = 0;
}

void
cj4_state_finish_tsumo(
    cj4_mahjong *state,
    cj4_player winner,
    cj4_tile_id winning_tile)
{
    state->winner_mask = (uint8_t)(1u << winner);
    state->winning_tile = winning_tile;
    cj4_state_set_round_result(state, CJ4_ROUND_END_TSUMO, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_clear_round_pending(state);
    cj4_state_set_phase(state, CJ4_PHASE_ROUND_END);
}

void
cj4_state_finish_multi_ron(
    cj4_mahjong *state,
    const cj4_player *players,
    uint8_t count,
    cj4_tile_id winning_tile)
{
    state->winner_mask = 0;
    if (count > CJ4_PLAYER_COUNT)
        count = CJ4_PLAYER_COUNT;
    for (uint8_t i = 0; i < count; ++i)
        state->winner_mask |= (uint8_t)(1u << players[i]);
    state->winning_tile = winning_tile;
    cj4_state_set_round_result(state, CJ4_ROUND_END_RON, CJ4_ABORTIVE_DRAW_NONE);
    /* Keep the pending kan target for round-end score reconstruction. */
    cj4_state_clear_pending_riichi(state);
    state->pending_kan_dora = 0;
    cj4_state_set_phase(state, CJ4_PHASE_ROUND_END);
}

void
cj4_state_finish_draw_round(
    cj4_mahjong *state,
    cj4_round_end_type round_end_type)
{
    state->winner_mask = 0;
    state->winning_tile = CJ4_TILE_ID_INVALID;
    cj4_state_set_round_result(state, round_end_type, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_clear_round_pending(state);
    cj4_state_set_phase(state, CJ4_PHASE_ROUND_END);
}

void
cj4_state_finish_abortive_draw(
    cj4_mahjong *state,
    cj4_abortive_draw_reason reason)
{
    cj4_state_finish_draw_round(state, CJ4_ROUND_END_ABORTIVE_DRAW);
    cj4_state_set_round_result(state, CJ4_ROUND_END_ABORTIVE_DRAW, reason);
}
