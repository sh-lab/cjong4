#ifndef CJ4_STATE_ANKAN_H
#define CJ4_STATE_ANKAN_H

#include <stdint.h>
#include <stdbool.h>

#include "tile.h"
#include "state.h"
#include "player.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Ankan (closed kan) state functions.
 */
bool
cj4_can_ankan(const cj4_mahjong *state, cj4_player player);

bool
cj4_can_ankan_with_tile(const cj4_mahjong *state, cj4_player player, cj4_tile_id tile1, cj4_tile_id tile2, cj4_tile_id tile3, cj4_tile_id tile4);

cj4_mahjong
cj4_do_ankan(const cj4_mahjong state, cj4_player player, cj4_tile_id tile1, cj4_tile_id tile2, cj4_tile_id tile3, cj4_tile_id tile4);

#ifdef __cplusplus
}
#endif

#endif /* CJ4_STATE_ANKAN_H */
