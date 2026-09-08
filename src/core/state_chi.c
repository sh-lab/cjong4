#include "state_chi.h"

#include "state_call.h"
#include "state_ops.h"
#include "state_query.h"

static bool
cj4_can_start_chi(
    const cj4_mahjong *state)
{
    if (!state)
        return false;

    cj4_player player = cj4_next_player(state);
    return cj4_state_can_claim_discard(state, player) &&
           !cj4_state_four_kans_abort_is_pending(state) &&
           cj4_state_live_wall_remaining(state) > 0 &&
           !cj4_state_is_riichi(state, player) &&
           cj4_location_collect_melds(state->locations, player).count < CJ4_MAX_MELDS &&
           cj4_tile_get_suit(cj4_get_last_discard_tile(state)) != CJ4_TILE_SUIT_HONOR;
}

bool
cj4_can_chi(
    const cj4_mahjong *state,
    const cj4_rules *rules)
{
    if (!cj4_can_start_chi(state))
        return false;

    cj4_hand hand = cj4_location_collect_hand(state->locations, cj4_next_player(state));
    for (uint8_t i = 0; i < hand.count; ++i)
        for (uint8_t j = (uint8_t)(i + 1); j < hand.count; ++j)
            if (cj4_can_chi_with_tile(state, rules, hand.items[i], hand.items[j]))
                return true;
    return false;
}

bool
cj4_can_chi_with_tile(
    const cj4_mahjong *state,
    const cj4_rules *rules,
    cj4_tile_id tile1,
    cj4_tile_id tile2)
{
    if (!cj4_can_start_chi(state) || tile1 == tile2)
        return false;

    cj4_player player = cj4_next_player(state);
    if (!cj4_state_tile_is_in_hand(state, player, tile1) ||
        !cj4_state_tile_is_in_hand(state, player, tile2))
        return false;

    cj4_tile_id last = cj4_get_last_discard_tile(state);
    if (cj4_tile_get_suit(tile1) != cj4_tile_get_suit(last) ||
        cj4_tile_get_suit(tile2) != cj4_tile_get_suit(last))
        return false;

    cj4_tile_type types[3] = {
        cj4_tile_get_type(last),
        cj4_tile_get_type(tile1),
        cj4_tile_get_type(tile2)};
    for (uint8_t i = 0; i < 3; ++i)
        for (uint8_t j = (uint8_t)(i + 1); j < 3; ++j)
            if (types[i] > types[j])
            {
                cj4_tile_type tmp = types[i];
                types[i] = types[j];
                types[j] = tmp;
            }
    if (types[0] + 1 != types[1] || types[1] + 1 != types[2])
        return false;

    cj4_mahjong called = cj4_state_apply_chi_or_pon(
        *state,
        player,
        CJ4_MELD_CHI,
        tile1,
        tile2);
    return cj4_state_call_has_legal_discard(&called, rules);
}

cj4_mahjong
cj4_do_chi(
    const cj4_mahjong state,
    const cj4_rules *rules,
    cj4_tile_id tile1,
    cj4_tile_id tile2)
{
    if (!cj4_can_chi_with_tile(&state, rules, tile1, tile2))
        return state;
    return cj4_state_apply_chi_or_pon(
        state,
        cj4_next_player(&state),
        CJ4_MELD_CHI,
        tile1,
        tile2);
}
