#ifndef CJ4_STATE_KAKAN_H
#define CJ4_STATE_KAKAN_H

#include <stdint.h>
#include <stdbool.h>

#include "tile.h"
#include "state.h"
#include "player.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Kakan (added kan to existing pon) state functions.
 */
bool
cj4_can_kakan(const cj4_mahjong *state);

bool
cj4_can_kakan_with_tile(const cj4_mahjong *state, cj4_tile_id tile);

cj4_mahjong
cj4_do_kakan(const cj4_mahjong state, cj4_tile_id tile);

cj4_mahjong
cj4_resolve_kan(const cj4_mahjong state);

#ifdef __cplusplus
}
#endif

#endif /* CJ4_STATE_KAKAN_H */
