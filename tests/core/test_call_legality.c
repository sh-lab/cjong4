#include <assert.h>
#include <string.h>

#include "cjong4/core/state_chi.h"
#include "cjong4/core/state_discard.h"
#include "cjong4/core/state_pon.h"
#include "state_ops.h"
#include "test_support.h"

static cj4_mahjong
make_call_state(
    cj4_player caller,
    const cj4_tile_id *hand,
    uint8_t count,
    cj4_tile_id last)
{
    cj4_mahjong state = make_empty_state();
    cj4_player discarder = (cj4_player)((caller + 3) % CJ4_PLAYER_COUNT);

    /* Three existing melds plus four concealed tiles is a normal call position. */
    for (uint8_t group = 0; group < 3; ++group)
        set_test_meld(&state, caller, group, &(cj4_meld){.tiles = {tile((cj4_tile_type)(27 + group), 0), tile((cj4_tile_type)(27 + group), 1), tile((cj4_tile_type)(27 + group), 2)}, .size = 3, .type = CJ4_MELD_PON, .from_player = discarder, .called_index = 0});
    set_hand(&state, caller, hand, count);
    cj4_state_set_current_player(&state, discarder);
    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    add_discard(&state, discarder, last);
    return state;
}

static void
test_chi_rules_and_suit_edges(
    void)
{
    cj4_rules rules = cj4_rules_default();

    for (uint8_t suit = 0; suit < 3; ++suit)
        for (uint8_t start = 0; start < 7; ++start)
            for (uint8_t called_index = 0; called_index < 3; ++called_index)
                for (cj4_player caller = 0; caller < CJ4_PLAYER_COUNT; ++caller)
                    for (uint8_t escape = 0; escape < 2; ++escape)
                    {
                        cj4_tile_type first = (cj4_tile_type)(suit * 9 + start);
                        cj4_tile_type called_type = (cj4_tile_type)(first + called_index);
                        cj4_tile_id last = tile(called_type, 0);
                        cj4_tile_id hand[4];
                        uint8_t count = 0;
                        for (uint8_t i = 0; i < 3; ++i)
                            if (i != called_index)
                                hand[count++] = tile((cj4_tile_type)(first + i), 0);
                        hand[2] = tile(called_type, 1);
                        hand[3] = tile(called_type, 2);
                        uint8_t blocked = !escape;
                        if (escape)
                            hand[3] = tile(32, 0);
                        else if (called_index == 0)
                        {
                            /* At 789 the next type belongs to another suit (or honors). */
                            hand[3] = tile((cj4_tile_type)(first + 3), first + 3 == 27 ? 3 : 0);
                            blocked = start < 6;
                        }
                        else if (called_index == 2)
                        {
                            /* At 123 the previous type is outside the suit. */
                            hand[3] = tile(first > 0 ? (cj4_tile_type)(first - 1) : 32, 0);
                            blocked = start > 0;
                        }
                        cj4_mahjong state = make_call_state(caller, hand, 4, last);
                        cj4_mahjong original = state;
                        for (uint8_t mode = 0; mode < 3; ++mode)
                        {
                            rules.kuikae_forbidden = mode == 0;
                            const cj4_rules *active = mode == 2 ? NULL : &rules;
                            uint8_t expected = mode != 0 || !blocked;
                            assert(cj4_can_chi(&state, active) == expected);
                            assert(cj4_can_chi_with_tile(&state, active, hand[0], hand[1]) == expected);
                            assert(cj4_can_chi_with_tile(&state, active, hand[1], hand[0]) == expected);
                            cj4_mahjong next = cj4_do_chi(state, active, hand[0], hand[1]);
                            assert(memcmp(&original, &state, sizeof(state)) == 0);
                            if (!expected)
                            {
                                assert(memcmp(&state, &next, sizeof(state)) == 0);
                                continue;
                            }
                            assert(cj4_state_phase(&next) == CJ4_PHASE_AFTER_CALL);
                            assert(cj4_state_current_player(&next) == caller);
                            assert(cj4_location_collect_melds(next.locations, caller).count == 4);
                            assert(cj4_can_discard(next, active, hand[2]) ||
                                   cj4_can_discard(next, active, hand[3]));
                            if (mode == 0)
                            {
                                cj4_mahjong rejected = cj4_do_discard(next, active, hand[2]);
                                assert(memcmp(&rejected, &next, sizeof(next)) == 0);
                            }
                        }
                    }
}

static void
test_pon_discard_requirement(
    void)
{
    cj4_rules rules = cj4_rules_default();
    const cj4_tile_id hand[] = {tile(4, 1), tile(4, 2), tile(4, 3), tile(32, 0)};
    for (cj4_player player = 1; player < CJ4_PLAYER_COUNT; ++player)
        for (uint8_t count = 2; count <= 4; ++count)
            for (uint8_t mode = 0; mode < 3; ++mode)
            {
                cj4_mahjong state = make_call_state(player, hand, count, tile(4, 0));
                /* Exercise pon claims from all three directions. */
                cj4_state_set_current_player(&state, CJ4_PLAYER_0);
                state.locations[tile(4, 0)].discard = cj4_location_make_discard(CJ4_PLAYER_0, 0, false);
                rules.kuikae_forbidden = mode == 0;
                const cj4_rules *active = mode == 2 ? NULL : &rules;
                uint8_t expected = count == 4 || (count == 3 && mode != 0);
                assert(cj4_can_pon(&state, active, player) == expected);
                assert(cj4_can_pon_with_tile(&state, active, player, hand[0], hand[1]) == expected);
                cj4_mahjong next = cj4_do_pon(state, active, player, hand[0], hand[1]);
                if (!expected)
                    assert(memcmp(&next, &state, sizeof(state)) == 0);
                else
                {
                    assert(cj4_state_current_player(&next) == player);
                    assert(cj4_state_phase(&next) == CJ4_PHASE_AFTER_CALL);
                    assert(cj4_can_discard(next, active, hand[count - 1]));
                }
            }
}

static void
test_chi_rejects_cross_suit_sequences_and_empty_hand(
    void)
{
    cj4_rules rules = cj4_rules_default();
    for (uint8_t suit = 0; suit < 2; ++suit)
    {
        cj4_tile_type first = (cj4_tile_type)(suit * 9 + 8);
        const cj4_tile_id hand[] = {
            tile((cj4_tile_type)(first + 1), 0),
            tile((cj4_tile_type)(first + 2), 0),
            tile(32, 0),
            tile(32, 1)};
        cj4_mahjong state = make_call_state(CJ4_PLAYER_1, hand, 4, tile(first, 0));
        assert(!cj4_can_chi(&state, &rules));
        assert(!cj4_can_chi_with_tile(&state, &rules, hand[0], hand[1]));
        cj4_mahjong next = cj4_do_chi(state, &rules, hand[0], hand[1]);
        assert(memcmp(&next, &state, sizeof(state)) == 0);
    }
    const cj4_tile_id hand[] = {tile(3, 0), tile(4, 0)};
    cj4_mahjong state = make_call_state(CJ4_PLAYER_1, hand, 2, tile(2, 0));
    assert(!cj4_can_chi(&state, &rules));
    assert(!cj4_can_chi(&state, NULL));
    assert(!cj4_can_chi_with_tile(&state, NULL, hand[0], hand[1]));
}

static void
test_calls_reject_invalid_tiles_and_full_melds(
    void)
{
    cj4_rules rules = cj4_rules_default();
    const cj4_tile_id hand[] = {tile(2, 1), tile(2, 2), tile(3, 0), tile(4, 0)};
    cj4_mahjong state = make_call_state(CJ4_PLAYER_1, hand, 4, tile(2, 0));
    assert(!cj4_can_chi_with_tile(&state, &rules, hand[2], CJ4_TILE_ID_INVALID));
    assert(!cj4_can_pon_with_tile(&state, &rules, CJ4_PLAYER_1, hand[0], CJ4_TILE_ID_INVALID));
    assert(!cj4_can_pon(&state, &rules, CJ4_PLAYER_COUNT));
    assert(!cj4_can_pon_with_tile(&state, &rules, CJ4_PLAYER_1, hand[0], hand[0]));
    assert(!cj4_can_chi_with_tile(&state, &rules, hand[2], hand[2]));
    set_test_meld(&state, CJ4_PLAYER_1, 3, &(cj4_meld){.tiles = {tile(30, 0), tile(30, 1), tile(30, 2)}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_0, .called_index = 0});
    assert(!cj4_can_chi(&state, &rules));
    assert(!cj4_can_pon(&state, &rules, CJ4_PLAYER_1));
    cj4_mahjong next = cj4_do_pon(state, &rules, CJ4_PLAYER_1, hand[0], hand[1]);
    assert(memcmp(&next, &state, sizeof(state)) == 0);
}

void
cj4_test_call_legality(
    void)
{
    test_chi_rules_and_suit_edges();
    test_pon_discard_requirement();
    test_chi_rejects_cross_suit_sequences_and_empty_hand();
    test_calls_reject_invalid_tiles_and_full_melds();
}
