#include "state_init.h"
#include "state_ops.h"
#include <string.h>

cj4_mahjong
cj4_create_initial_state(
    const cj4_tile_id wall[CJ4_TILE_ID_COUNT],
    const cj4_rules *rules)
{
    cj4_mahjong state;

    // Minimal safe initialization
    memset(&state, 0, sizeof(state));

    if (wall)
    {
        memcpy(state.wall, wall, sizeof(state.wall));
    }

    // Initialize scores from rules or default to 25000
    int32_t initial_score = 25000;

    if (rules)
    {
        initial_score = rules->initial_score;
    }

    for (int i = 0; i < CJ4_PLAYER_COUNT; ++i)
    {
        state.scores[i] = initial_score;
    }

    // Default round and dealer
    state.round_wind = CJ4_WIND_EAST;
    state.dealer = CJ4_PLAYER_0;

    // wall_pos starts at 0 (next draw)
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

    state.dead_wall_draw_count = 0;

    state.dora_indicators_count = 1;

    state.current_player = state.dealer;

    state.phase = CJ4_PHASE_DRAW;

    return state;
}