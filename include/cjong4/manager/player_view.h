#ifndef CJ4M_PLAYER_VIEW_H
#define CJ4M_PLAYER_VIEW_H

#include <stdint.h>

#include "cjong4/core/state.h"

#ifdef __cplusplus
extern "C"
{
#endif

    enum
    {
        CJ4M_MAX_HAND_TILES = 14
    };

    typedef struct
    {
        cj4_player player;
        cj4_phase phase;
        cj4_player current_player;
        cj4_player dealer;
        cj4_wind round_wind;
        int32_t scores[CJ4_PLAYER_COUNT];
        uint8_t is_riichi[CJ4_PLAYER_COUNT];
        uint8_t temporary_furiten;
        uint8_t riichi_furiten;
        uint8_t first_turn_uninterrupted;

        cj4_tile_id hand[CJ4M_MAX_HAND_TILES];
        uint8_t hand_count;
        cj4_tile_id draw_tile;
        cj4_tile_id last_discard;
        cj4_tile_id pending_kakan_tile;

        cj4_discard discards[CJ4_MAX_DISCARDS];
        uint8_t discard_count;

        cj4_meld melds[CJ4_PLAYER_COUNT][CJ4_MAX_MELDS];
        uint8_t meld_count[CJ4_PLAYER_COUNT];
    } cj4_player_view;

#ifdef __cplusplus
}
#endif

#endif /* CJ4M_PLAYER_VIEW_H */
