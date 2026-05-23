#include "state_pon.h"
#include "state_query.h"
#include "state_ops.h"

bool cj4_can_pon(const cj4_mahjong *state, cj4_player player)
{
    if (!cj4_state_can_claim_discard(state, player))
    {
        return false;
    }

    cj4_tile_type last_discard_tile_type = cj4_tile_get_type(cj4_get_last_discard_tile(state));

    if (cj4_count_hand(state, player, last_discard_tile_type) < 2)
    {
        return false;
    }

    return true;
}

bool cj4_can_pon_with_tile(const cj4_mahjong *state, cj4_player player, cj4_tile_id tile1, cj4_tile_id tile2)
{
    if (!cj4_can_pon(state, player))
    {
        return false;
    }

    cj4_tile_id last = cj4_get_last_discard_tile(state);
    cj4_tile_type type = cj4_tile_get_type(last);

    if (cj4_tile_get_type(tile1) != type ||
        cj4_tile_get_type(tile2) != type)
    {
        return false;
    }

    if (!cj4_state_tile_is_in_hand(state, player, tile1) ||
        !cj4_state_tile_is_in_hand(state, player, tile2))
    {
        return false;
    }

    return true;
}

cj4_mahjong
cj4_do_pon(const cj4_mahjong state, cj4_player player, cj4_tile_id tile1, cj4_tile_id tile2)
{
    assert(cj4_can_pon_with_tile(&state, player, tile1, tile2));

    cj4_mahjong next = state;

    cj4_tile_id last = cj4_get_last_discard_tile(&state);
    const cj4_tile_id meld_tiles[3] = { last, tile1, tile2 };

    cj4_state_add_meld(
        &next,
        player,
        CJ4_MELD_PON,
        meld_tiles,
        3,
        state.current_player,
        0);
    cj4_state_finish_open_call(&next, player, CJ4_PHASE_AFTER_CALL);

    return next;
}
