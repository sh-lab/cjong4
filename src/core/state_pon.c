#include "state_pon.h"

#include "state_call.h"
#include "state_ops.h"
#include "state_query.h"

static bool
cj4_can_start_pon(
    const cj4_mahjong *state,
    cj4_player player)
{
    return state && player < CJ4_PLAYER_COUNT &&
           cj4_state_can_claim_discard(state, player) &&
           !cj4_state_four_kans_abort_is_pending(state) &&
           cj4_state_live_wall_remaining(state) > 0 &&
           !cj4_state_is_riichi(state, player) &&
           cj4_location_collect_melds(state->locations, player).count < CJ4_MAX_MELDS;
}

bool
cj4_can_pon(
    const cj4_mahjong *state,
    const cj4_rules *rules,
    cj4_player player)
{
    if (!cj4_can_start_pon(state, player))
        return false;

    cj4_tile_type type = cj4_tile_get_type(cj4_get_last_discard_tile(state));
    for (uint8_t i = 0; i < CJ4_TILE_PER_TYPE; ++i)
        for (uint8_t j = (uint8_t)(i + 1); j < CJ4_TILE_PER_TYPE; ++j)
            if (cj4_can_pon_with_tile(state, rules, player, cj4_tile_make(type, i), cj4_tile_make(type, j)))
                return true;
    return false;
}

bool
cj4_can_pon_with_tile(
    const cj4_mahjong *state,
    const cj4_rules *rules,
    cj4_player player,
    cj4_tile_id tile1,
    cj4_tile_id tile2)
{
    if (!cj4_can_start_pon(state, player) || tile1 == tile2)
        return false;

    if (!cj4_state_tile_is_in_hand(state, player, tile1) ||
        !cj4_state_tile_is_in_hand(state, player, tile2))
        return false;

    cj4_tile_type type = cj4_tile_get_type(cj4_get_last_discard_tile(state));
    if (cj4_tile_get_type(tile1) != type || cj4_tile_get_type(tile2) != type)
        return false;

    cj4_mahjong called = cj4_state_apply_chi_or_pon(
        *state,
        player,
        CJ4_MELD_PON,
        tile1,
        tile2);
    return cj4_state_call_has_legal_discard(&called, rules);
}

cj4_mahjong
cj4_do_pon(
    const cj4_mahjong state,
    const cj4_rules *rules,
    cj4_player player,
    cj4_tile_id tile1,
    cj4_tile_id tile2)
{
    if (!cj4_can_pon_with_tile(&state, rules, player, tile1, tile2))
        return state;
    return cj4_state_apply_chi_or_pon(state, player, CJ4_MELD_PON, tile1, tile2);
}
