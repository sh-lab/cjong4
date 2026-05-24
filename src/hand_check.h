#ifndef CJ4_HAND_CHECK_H
#define CJ4_HAND_CHECK_H

#include <stdbool.h>
#include "state.h"
#include "rules.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Returns true if player's hand (including drawn tile present in locations)
 * is a complete winning hand shape (standard 4 melds + pair, seven pairs,
 * or kokushi). This does NOT validate yaku.
 */
bool cj4_is_complete_hand(const cj4_mahjong* state, cj4_player player);

uint8_t
cj4_collect_waiting_tile_types(
    const cj4_mahjong *state,
    cj4_player player,
    uint8_t waits[CJ4_TILE_TYPE_COUNT]
);

#ifdef __cplusplus
}
#endif

#endif /* CJ4_HAND_CHECK_H */
