#ifndef CJ4M_PLAYER_VIEW_H
#define CJ4M_PLAYER_VIEW_H

#include <stdint.h>

#include "cjong4/core/state.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        cj4_location locations[CJ4_TILE_ID_COUNT];

        int32_t scores[CJ4_PLAYER_COUNT];
        cj4_phase phase;

        cj4_player player;
        cj4_player current_player;
        cj4_player dealer;
        cj4_wind round_wind;

        uint8_t honba;
        uint8_t riichi_sticks;

        uint8_t is_riichi[CJ4_PLAYER_COUNT];
        uint8_t temporary_furiten;
        uint8_t riichi_furiten;
        uint8_t first_turn_uninterrupted;
        uint8_t live_wall_remaining;

        cj4_tile_id draw_tile;
        cj4_tile_id last_discard;
        cj4_tile_id kan_tile;
    } cj4_player_view;

#ifdef __cplusplus
}
#endif

#endif /* CJ4M_PLAYER_VIEW_H */
