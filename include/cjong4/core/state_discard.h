#ifndef CJ4_STATE_DISCARD_H
#define CJ4_STATE_DISCARD_H

#include "rules.h"
#include "state.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    bool
    cj4_can_discard(
        const cj4_mahjong state,
        cj4_tile_id tile);

    bool
    cj4_can_discard_with_rules(
        const cj4_mahjong state,
        const cj4_rules *rules,
        cj4_tile_id tile);

    cj4_mahjong
    cj4_do_discard(
        const cj4_mahjong state,
        cj4_tile_id tile);

    cj4_mahjong
    cj4_do_discard_with_rules(
        const cj4_mahjong state,
        const cj4_rules *rules,
        cj4_tile_id tile);

#ifdef __cplusplus
}
#endif

#endif