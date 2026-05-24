#include "state_init.h"
#include "state_round_init.h"
#include "state_ops.h"
#include <string.h>

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

    // Minimal safe initialization
    memset(&state, 0, sizeof(state));

    if (wall)
    {
        memcpy(state.wall, wall, sizeof(state.wall));
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
    state.draw_turn_count[state.dealer] = 1;

    state.dead_wall_draw_count = 0;

    state.dora_indicators_count = 1;
    state.first_turn_uninterrupted = 1;
    state.winning_from_chankan = 0;
    state.pending_kakan_tile = CJ4_TILE_ID_INVALID;
    state.winning_tile = CJ4_TILE_ID_INVALID;
    state.round_end_type = CJ4_ROUND_END_NONE;
    state.next_round_wind = CJ4_WIND_EAST;
    state.next_dealer = CJ4_PLAYER_0;
    state.settlement_should_end = 0;

    state.current_player = state.dealer;

    state.phase = CJ4_PHASE_DRAW;

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