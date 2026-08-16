#ifndef CJ4_STATE_INIT_H
#define CJ4_STATE_INIT_H

#include <stdbool.h>

#include "rules.h"
#include "state.h"

#ifdef __cplusplus
extern "C"
{
#endif

    bool
    cj4_wall_is_valid(
        const cj4_tile_id wall[CJ4_TILE_ID_COUNT]);

    /* Returns a GAME_END sentinel state when wall is not a permutation of
     * every physical tile ID from 0 through 135. */
    cj4_mahjong
    cj4_create_initial_state(
        const cj4_tile_id wall[CJ4_TILE_ID_COUNT],
        const cj4_rules *rules);

#ifdef __cplusplus
}
#endif

#endif /* CJ4_STATE_INIT_H */
