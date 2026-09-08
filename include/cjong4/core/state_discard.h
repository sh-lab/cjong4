#ifndef CJ4_STATE_DISCARD_H
#define CJ4_STATE_DISCARD_H

#include "rules.h"
#include "state.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* Use the same rules for calls and their following discard.
     * NULL explicitly allows kuikae; it is not cj4_rules_default(). */
    bool
    cj4_can_discard(
        const cj4_mahjong state,
        const cj4_rules *rules,
        cj4_tile_id tile);

    cj4_mahjong
    cj4_do_discard(
        const cj4_mahjong state,
        const cj4_rules *rules,
        cj4_tile_id tile);

#ifdef __cplusplus
}
#endif

#endif
