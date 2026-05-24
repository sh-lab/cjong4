#ifndef CJ4_STATE_KAN_H
#define CJ4_STATE_KAN_H

#include <stdint.h>
#include <stdbool.h>

#include "tile.h"
#include "state.h"
#include "player.h"
#include "rules.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Consolidated Kan (minkan / ankan / kakan) state functions.
 */

/* Minkan */
bool
cj4_can_minkan(const cj4_mahjong *state, cj4_player player);

bool
cj4_can_minkan_with_tile(const cj4_mahjong *state, cj4_player player, cj4_tile_id tile1, cj4_tile_id tile2, cj4_tile_id tile3);

cj4_mahjong
cj4_do_minkan(const cj4_mahjong state, cj4_player player, cj4_tile_id tile1, cj4_tile_id tile2, cj4_tile_id tile3);

/* Ankan */
bool
cj4_can_ankan(const cj4_mahjong *state);

bool
cj4_can_ankan_with_tile(const cj4_mahjong *state, cj4_tile_id tile1, cj4_tile_id tile2, cj4_tile_id tile3, cj4_tile_id tile4);

cj4_mahjong
cj4_do_ankan(
    const cj4_mahjong state,
    cj4_tile_id tile1,
    cj4_tile_id tile2,
    cj4_tile_id tile3,
    cj4_tile_id tile4
);

/* Kakan */
bool
cj4_can_kakan(const cj4_mahjong *state);

bool
cj4_can_kakan_with_tile(const cj4_mahjong *state, cj4_tile_id tile);

cj4_mahjong
cj4_do_kakan(const cj4_mahjong state, cj4_tile_id tile);

bool
cj4_can_rinshan_draw(const cj4_mahjong* state);

/* Kan resolution (draw rinshan and add dora) */
cj4_mahjong
cj4_do_rinshan_draw(const cj4_mahjong state);

#ifdef __cplusplus
}
#endif

#endif /* CJ4_STATE_KAN_H */
