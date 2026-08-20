#include "state_init.h"
#include "state_ops.h"
#include "state_round_init.h"
#include <string.h>

bool
cj4_wall_is_valid(
    const cj4_tile_id wall[CJ4_TILE_ID_COUNT])
{
    uint8_t seen[CJ4_TILE_ID_COUNT] = {0};

    if (!wall)
        return false;

    for (uint16_t position = 0; position < CJ4_TILE_ID_COUNT; ++position)
    {
        cj4_tile_id tile = wall[position];

        if (!cj4_tile_id_is_valid(tile) || seen[tile])
            return false;
        seen[tile] = 1;
    }

    return true;
}

static cj4_mahjong
cj4_state_create_invalid(
    void)
{
    cj4_mahjong state;

    memset(&state, 0, sizeof(state));
    memset(state.locations, CJ4_LOCATION_NONE, sizeof(state.locations));
    state.draw_tile = CJ4_TILE_ID_INVALID;
    state.last_discard_tile = CJ4_TILE_ID_INVALID;
    state.pending_kakan_tile = CJ4_TILE_ID_INVALID;
    state.pending_ankan_tile = CJ4_TILE_ID_INVALID;
    memset(state.pending_ankan_tiles, CJ4_TILE_ID_INVALID, sizeof(state.pending_ankan_tiles));
    state.winning_tile = CJ4_TILE_ID_INVALID;
    cj4_state_clear_pending_riichi_bits(&state);
    cj4_state_set_phase(&state, CJ4_PHASE_GAME_END);

    return state;
}

cj4_mahjong
cj4_state_create_round(
    const cj4_tile_id wall[CJ4_TILE_ID_COUNT],
    const cj4_rules *rules,
    const int32_t scores[CJ4_PLAYER_COUNT],
    cj4_wind round_wind,
    cj4_player dealer,
    uint8_t honba,
    uint8_t riichi_sticks)
{
    cj4_mahjong state;

    if (!cj4_wall_is_valid(wall))
        return cj4_state_create_invalid();

    memset(&state, 0, sizeof(state));
    memset(state.locations, CJ4_LOCATION_NONE, sizeof(state.locations));
    for (uint16_t position = 0; position < CJ4_TILE_ID_COUNT; ++position)
    {
        cj4_tile_id tile = wall[position];
        state.locations[tile].wall = (uint8_t)position;
    }

    int32_t initial_score = 25000;

    if (rules)
    {
        initial_score = rules->initial_score;
    }

    if (scores)
        memcpy(state.scores, scores, sizeof(state.scores));
    else
    {
        for (int i = 0; i < CJ4_PLAYER_COUNT; ++i)
            state.scores[i] = initial_score;
    }

    state.round_wind = round_wind;
    state.dealer = dealer;
    state.honba = honba;
    state.riichi_sticks = riichi_sticks;

    state.wall_pos = 0;

    for (int i = 0; i < 3; ++i)
    {
        for (int p = 0; p < CJ4_PLAYER_COUNT; ++p)
        {
            for (int j = 0; j < 4; ++j)
            {
                cj4_state_draw_tile(&state, (cj4_player)p);
            }
        }
    }

    for (int p = 0; p < CJ4_PLAYER_COUNT; ++p)
    {
        cj4_state_draw_tile(&state, (cj4_player)p);
    }

    state.draw_tile = cj4_state_draw_tile(&state, state.dealer);
    cj4_state_set_draw_turn(&state, state.dealer, 1);

    state.dead_wall_draw_count = 0;

    state.dora_count = 1;
    cj4_state_set_first_turn(&state, true);
    cj4_state_set_chankan(&state, false);
    state.pending_kakan_tile = CJ4_TILE_ID_INVALID;
    state.pending_ankan_tile = CJ4_TILE_ID_INVALID;
    for (int i = 0; i < 4; ++i)
        state.pending_ankan_tiles[i] = CJ4_TILE_ID_INVALID;
    cj4_state_clear_pending_riichi_bits(&state);
    state.pending_kan_dora_count = 0;
    state.winning_tile = CJ4_TILE_ID_INVALID;
    state.last_discard_tile = CJ4_TILE_ID_INVALID;
    cj4_state_set_round_result(&state, CJ4_ROUND_END_NONE, CJ4_ABORTIVE_DRAW_NONE);
    state.next_round_wind = CJ4_WIND_EAST;
    state.next_dealer = CJ4_PLAYER_0;
    state.settlement_should_end = 0;

    cj4_state_set_current_player(&state, state.dealer);
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);

    return state;
}

cj4_mahjong
cj4_create_initial_state(
    const cj4_tile_id wall[CJ4_TILE_ID_COUNT],
    const cj4_rules *rules)
{
    return cj4_state_create_round(
        wall,
        rules,
        NULL,
        CJ4_WIND_EAST,
        CJ4_PLAYER_0,
        0,
        0);
}
