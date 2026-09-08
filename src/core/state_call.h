#ifndef CJ4_STATE_CALL_H
#define CJ4_STATE_CALL_H

#include "rules.h"
#include "state.h"

/* Internal chi/pon transition. The caller must validate the tiles first. */
cj4_mahjong
cj4_state_apply_chi_or_pon(
    const cj4_mahjong state,
    cj4_player player,
    cj4_meld_type type,
    cj4_tile_id tile1,
    cj4_tile_id tile2);

bool
cj4_state_call_has_legal_discard(
    const cj4_mahjong *called,
    const cj4_rules *rules);

#endif /* CJ4_STATE_CALL_H */
