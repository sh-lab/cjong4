#ifndef CJ4_STATE_PON_H
#define CJ4_STATE_PON_H

#include "rules.h"
#include "state.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* Use the same rules for calls and their following discard.
     * NULL explicitly allows kuikae; it is not cj4_rules_default(). */
    /* Checks the complete call, including at least one legal following discard. */
    bool
    cj4_can_pon(const cj4_mahjong *state, const cj4_rules *rules, cj4_player player);

    bool
    cj4_can_pon_with_tile(const cj4_mahjong *state, const cj4_rules *rules, cj4_player player, cj4_tile_id tile1, cj4_tile_id tile2);

    /* An illegal call returns the input unchanged, in every build configuration. */
    cj4_mahjong
    cj4_do_pon(const cj4_mahjong state, const cj4_rules *rules, cj4_player player, cj4_tile_id tile1, cj4_tile_id tile2);

#ifdef __cplusplus
}
#endif

#endif /* CJ4_STATE_PON_H */
