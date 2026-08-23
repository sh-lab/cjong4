#include "test_support.h"

#include <assert.h>
#include <string.h>

#include "../src/core/state_ops.h"

cj4_tile_id
tile(
    cj4_tile_type type,
    uint8_t index)
{
    return cj4_tile_make(type, index);
}

cj4_mahjong
make_empty_state(
    void)
{
    cj4_mahjong state;

    memset(&state, 0, sizeof(state));
    memset(state.locations, CJ4_LOCATION_NONE, sizeof(state.locations));
    for (uint16_t i = 0; i < CJ4_TILE_ID_COUNT; ++i)
        state.locations[i].wall = (uint8_t)i;
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    state.dealer = CJ4_PLAYER_0;
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.round_wind = CJ4_WIND_EAST;
    state.draw_tile = CJ4_TILE_ID_INVALID;
    state.pending_kakan_tile = CJ4_TILE_ID_INVALID;
    state.pending_ankan_tile = CJ4_TILE_ID_INVALID;
    for (uint8_t i = 0; i < CJ4_TILE_PER_TYPE; ++i)
        state.pending_ankan_tiles[i] = CJ4_TILE_ID_INVALID;
    cj4_state_clear_pending_riichi_bits(&state);
    state.last_discard_tile = CJ4_TILE_ID_INVALID;
    state.winning_tile = CJ4_TILE_ID_INVALID;
    cj4_state_set_round_result(
        &state,
        CJ4_ROUND_END_NONE,
        CJ4_ABORTIVE_DRAW_NONE);

    for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
        state.scores[i] = 25000;

    return state;
}

void
set_hand(
    cj4_mahjong *state,
    cj4_player player,
    const cj4_tile_id *tiles,
    uint8_t count)
{
    for (uint8_t i = 0; i < count; ++i)
        state->locations[tiles[i]].placement =
            cj4_location_make_hand(player);
}

void
set_test_wall(
    cj4_mahjong *state,
    uint8_t position,
    cj4_tile_id tile_id)
{
    for (uint16_t i = 0; i < CJ4_TILE_ID_COUNT; ++i)
        if (state->locations[i].wall == position)
            state->locations[i].wall = CJ4_LOCATION_NONE;
    state->locations[tile_id].wall = position;
}

void
add_discard(
    cj4_mahjong *state,
    cj4_player player,
    cj4_tile_id discarded)
{
    cj4_player current = cj4_state_current_player(state);
    cj4_state_set_current_player(state, player);
    cj4_state_record_discard(state, discarded, 0, 0);
    cj4_state_set_current_player(state, current);
}

void
set_test_meld(
    cj4_mahjong *state,
    cj4_player player,
    uint8_t group,
    const cj4_meld *meld)
{
    for (uint8_t i = 0; i < meld->size; ++i)
    {
        cj4_tile_id id = meld->tiles[i];
        state->locations[id].placement =
            cj4_location_make_meld(player, group, meld->type);
        if (i == meld->called_index && meld->from_player != player &&
            state->locations[id].discard == CJ4_LOCATION_NONE)
        {
            state->locations[id].discard =
                cj4_location_make_discard(meld->from_player, 0, false);
            state->locations[id].discard_history =
                cj4_location_make_discard_history(
                    state->discard_count++,
                    false);
        }
    }
}

cj4_meld
get_test_meld(
    const cj4_mahjong *state,
    cj4_player player,
    uint8_t group)
{
    cj4_meld_list melds =
        cj4_location_collect_melds(state->locations, player);

    assert(group < melds.count);
    return melds.items[group];
}

void
set_test_kan(
    cj4_mahjong *state,
    cj4_player player,
    uint8_t group,
    cj4_meld_type type,
    cj4_tile_id first)
{
    set_test_meld(
        state,
        player,
        group,
        &(cj4_meld){
            .tiles = {
                first,
                (cj4_tile_id)(first + 1),
                (cj4_tile_id)(first + 2),
                (cj4_tile_id)(first + 3)},
            .size = CJ4_TILE_PER_TYPE,
            .type = type,
            .from_player = player,
            .called_index = CJ4_CALLED_INDEX_NONE});
}

uint8_t
contains_win_yaku(
    const cj4_win_result *result,
    cj4_win_yaku yaku)
{
    for (uint8_t i = 0; i < result->yaku_count; ++i)
    {
        if (result->yaku[i] == yaku)
            return 1;
    }

    return 0;
}
