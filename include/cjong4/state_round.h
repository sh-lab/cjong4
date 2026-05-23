#ifndef CJ4_STATE_ROUND_H
#define CJ4_STATE_ROUND_H

#include <stdbool.h>

#include "rules.h"
#include "state.h"

#ifdef __cplusplus
extern "C" {
#endif

bool
cj4_can_next_round(const cj4_mahjong state);

cj4_mahjong
cj4_do_next_round(
    const cj4_mahjong state,
    const cj4_tile_id wall[CJ4_TILE_ID_COUNT],
    const cj4_rules *rules);

bool
cj4_can_game_end(const cj4_mahjong state);

cj4_mahjong
cj4_do_game_end(const cj4_mahjong state);

#ifdef __cplusplus
}
#endif

#endif /* CJ4_STATE_ROUND_H */
