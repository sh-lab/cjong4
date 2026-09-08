#include "state_call.h"

#include "state_discard.h"
#include "state_ops.h"
#include "state_query.h"

cj4_mahjong
cj4_state_apply_chi_or_pon(
    const cj4_mahjong state,
    cj4_player player,
    cj4_meld_type type,
    cj4_tile_id tile1,
    cj4_tile_id tile2)
{
    cj4_mahjong next = state;
    const cj4_tile_id meld_tiles[3] = {
        cj4_get_last_discard_tile(&state),
        tile1,
        tile2};

    cj4_state_add_meld(
        &next,
        player,
        type,
        meld_tiles,
        3,
        cj4_state_current_player(&state),
        0);
    cj4_state_establish_pending_riichi(&next);
    cj4_state_finish_open_call(&next, player, CJ4_PHASE_AFTER_CALL);
    cj4_state_set_first_turn(&next, 0);
    cj4_state_set_chankan(&next, 0);
    next.pending_kakan_tile = CJ4_TILE_ID_INVALID;
    next.pending_ankan_tile = CJ4_TILE_ID_INVALID;

    return next;
}

bool
cj4_state_call_has_legal_discard(
    const cj4_mahjong *called,
    const cj4_rules *rules)
{
    cj4_hand hand = cj4_location_collect_hand(
        called->locations,
        cj4_state_current_player(called));

    for (uint8_t i = 0; i < hand.count; ++i)
        if (cj4_can_discard(*called, rules, hand.items[i]))
            return true;
    return false;
}
