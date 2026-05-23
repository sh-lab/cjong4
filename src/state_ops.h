#ifndef CJ4_STATE_OPS_H
#define CJ4_STATE_OPS_H

#include <stdbool.h>

#include "tile.h"
#include "state.h"
#include "rules.h"

bool
cj4_state_has_last_discard(const cj4_mahjong *state);

bool
cj4_state_can_claim_discard(
    const cj4_mahjong *state,
    cj4_player player
);

bool
cj4_state_tile_is_in_hand(
    const cj4_mahjong *state,
    cj4_player player,
    cj4_tile_id tile
);

cj4_tile_id
cj4_state_draw_tile(
    cj4_mahjong *state,
    cj4_player player
);


cj4_tile_id
cj4_state_draw_dead_wall_tile(
    cj4_mahjong *state,
    cj4_player player
);

void
cj4_state_set_location(
    cj4_mahjong *state,
    cj4_tile_id tile,
    cj4_zone zone,
    cj4_player owner
);

void
cj4_state_clear_draw_tile(cj4_mahjong *state);

void
cj4_state_clear_all_ippatsu(cj4_mahjong *state);

void
cj4_state_record_discard(
    cj4_mahjong *state,
    cj4_tile_id tile,
    uint8_t is_tsumogiri
);

void
cj4_state_consume_last_discard(cj4_mahjong *state);

void
cj4_state_add_meld(
    cj4_mahjong *state,
    cj4_player player,
    cj4_meld_type type,
    const cj4_tile_id *tiles,
    uint8_t size,
    cj4_player from_player,
    uint8_t called_index
);

void
cj4_state_finish_open_call(
    cj4_mahjong *state,
    cj4_player player,
    cj4_phase next_phase
);

void
cj4_state_add_dora_indicator(cj4_mahjong *state);

void
cj4_state_finish_tsumo(
    cj4_mahjong *state,
    cj4_player winner,
    cj4_tile_id winning_tile
);

void
cj4_state_finish_multi_ron(
    cj4_mahjong *state,
    const cj4_player *players,
    uint8_t count,
    cj4_tile_id winning_tile
);

#endif /* CJ4_STATE_OPS_H */