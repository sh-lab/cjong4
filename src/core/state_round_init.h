#ifndef CJ4_STATE_ROUND_INIT_H
#define CJ4_STATE_ROUND_INIT_H

#include "rules.h"
#include "state.h"

cj4_mahjong
cj4_state_create_round(
    const cj4_tile_id wall[CJ4_TILE_ID_COUNT],
    const cj4_rules *rules,
    const int32_t scores[CJ4_PLAYER_COUNT],
    cj4_wind round_wind,
    cj4_player dealer,
    uint8_t honba,
    uint8_t riichi_sticks);

#endif /* CJ4_STATE_ROUND_INIT_H */
