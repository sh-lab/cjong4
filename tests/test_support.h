#ifndef CJ4_TEST_SUPPORT_H
#define CJ4_TEST_SUPPORT_H

#include "cjong4/core/state.h"
#include "cjong4/core/state_query.h"

cj4_tile_id
tile(
    cj4_tile_type type,
    uint8_t index);

cj4_mahjong
make_empty_state(
    void);

void
set_hand(
    cj4_mahjong *state,
    cj4_player player,
    const cj4_tile_id *tiles,
    uint8_t count);

void
set_test_wall(
    cj4_mahjong *state,
    uint8_t position,
    cj4_tile_id tile_id);

void
add_discard(
    cj4_mahjong *state,
    cj4_player player,
    cj4_tile_id discarded);

void
set_test_meld(
    cj4_mahjong *state,
    cj4_player player,
    uint8_t group,
    const cj4_meld *meld);

cj4_meld
get_test_meld(
    const cj4_mahjong *state,
    cj4_player player,
    uint8_t group);

void
set_test_kan(
    cj4_mahjong *state,
    cj4_player player,
    uint8_t group,
    cj4_meld_type type,
    cj4_tile_id first);

#endif /* CJ4_TEST_SUPPORT_H */
