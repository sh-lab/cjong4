#ifndef CJ4_STATE_CHI_H
#define CJ4_STATE_CHI_H

#include "rules.h"
#include "state.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    bool
    cj4_can_chi(const cj4_mahjong *state);

    bool
    cj4_can_chi_with_tile(const cj4_mahjong *state, cj4_tile_id tile1, cj4_tile_id tile2);

    cj4_mahjong
    cj4_do_chi(const cj4_mahjong state, cj4_tile_id tile1, cj4_tile_id tile2);

#ifdef __cplusplus
}
#endif

#endif /* CJ4_STATE_CHI_H */
