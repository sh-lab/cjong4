#include <assert.h>
#include <string.h>

#include "../../src/core/hand_check.h"
#include "../../src/core/state_ops.h"
#include "../test_support.h"
#include "cjong4/core/hand_analysis.h"
#include "cjong4/core/state_query.h"
#include "cjong4/manager/manager.h"
#include "cjong4/player/hand_analysis.h"

static void
test_standard_shanten_and_wait_counts(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_shanten_result shanten;
    cj4_waiting_tile_types waits;
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(3, 0),
        tile(4, 0),
        tile(15, 0),
        tile(15, 1)};

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));

    assert(cj4_calculate_shanten(&state, CJ4_PLAYER_0, &shanten));
    assert(shanten.standard == 0);
    assert(cj4_is_shape_tenpai(&state, CJ4_PLAYER_0));
    assert(cj4_collect_waiting_tile_types(&state, CJ4_PLAYER_0, &waits));
    assert(waits.types[2] == 1);
    assert(waits.count[2] == 3);
    assert(waits.types[5] == 1);
    assert(waits.count[5] == 4);

    add_discard(&state, CJ4_PLAYER_1, tile(5, 0));
    add_discard(&state, CJ4_PLAYER_2, tile(5, 1));
    state.dora_count = 1;
    set_test_wall(&state, 130, tile(5, 2));
    set_hand(&state, CJ4_PLAYER_3, &(cj4_tile_id){tile(5, 3)}, 1);

    assert(cj4_collect_waiting_tile_types(&state, CJ4_PLAYER_0, &waits));
    assert(waits.types[5] == 1);
    assert(waits.count[5] == 1);

    add_discard(&state, CJ4_PLAYER_3, tile(5, 3));
    assert(cj4_collect_waiting_tile_types(&state, CJ4_PLAYER_0, &waits));
    assert(waits.types[5] == 1);
    assert(waits.count[5] == 0);
}

static void
test_standard_shanten_distance(
    void)
{
    cj4_mahjong one_away = make_empty_state();
    cj4_mahjong two_away = make_empty_state();
    cj4_shanten_result shanten;
    const cj4_tile_id one_away_hand[] = {
        tile(0, 0), tile(1, 0), tile(2, 0),
        tile(9, 0), tile(10, 0), tile(11, 0),
        tile(18, 0), tile(19, 0), tile(20, 0),
        tile(3, 0), tile(4, 0),
        tile(15, 0), tile(27, 0)};
    const cj4_tile_id two_away_hand[] = {
        tile(0, 0), tile(1, 0), tile(2, 0),
        tile(9, 0), tile(10, 0), tile(11, 0),
        tile(21, 0), tile(22, 0),
        tile(24, 0), tile(25, 0),
        tile(27, 0), tile(28, 0), tile(29, 0)};

    set_hand(
        &one_away,
        CJ4_PLAYER_0,
        one_away_hand,
        (uint8_t)(sizeof(one_away_hand) / sizeof(one_away_hand[0])));
    assert(cj4_calculate_shanten(&one_away, CJ4_PLAYER_0, &shanten));
    assert(shanten.standard == 1);

    set_hand(
        &two_away,
        CJ4_PLAYER_0,
        two_away_hand,
        (uint8_t)(sizeof(two_away_hand) / sizeof(two_away_hand[0])));
    assert(cj4_calculate_shanten(&two_away, CJ4_PLAYER_0, &shanten));
    assert(shanten.standard == 2);
}

static void
test_special_hand_shanten(
    void)
{
    cj4_mahjong chiitoitsu_state = make_empty_state();
    cj4_mahjong kokushi_state = make_empty_state();
    cj4_shanten_result shanten;
    cj4_waiting_tile_types waits;
    const cj4_tile_id chiitoitsu[] = {
        tile(0, 0), tile(0, 1),
        tile(1, 0), tile(1, 1),
        tile(2, 0), tile(2, 1),
        tile(9, 0), tile(9, 1),
        tile(10, 0), tile(10, 1),
        tile(18, 0), tile(18, 1),
        tile(27, 0)};
    const cj4_tile_id kokushi[] = {
        tile(0, 0),
        tile(8, 0),
        tile(9, 0),
        tile(17, 0),
        tile(18, 0),
        tile(26, 0),
        tile(27, 0),
        tile(28, 0),
        tile(29, 0),
        tile(30, 0),
        tile(31, 0),
        tile(32, 0),
        tile(33, 0)};

    set_hand(
        &chiitoitsu_state,
        CJ4_PLAYER_0,
        chiitoitsu,
        (uint8_t)(sizeof(chiitoitsu) / sizeof(chiitoitsu[0])));
    assert(cj4_calculate_shanten(&chiitoitsu_state, CJ4_PLAYER_0, &shanten));
    assert(shanten.chiitoitsu == 0);
    assert(cj4_collect_waiting_tile_types(
        &chiitoitsu_state,
        CJ4_PLAYER_0,
        &waits));
    assert(waits.types[27] == 1);
    assert(waits.count[27] == 3);

    chiitoitsu_state.locations[tile(27, 1)].placement =
        cj4_location_make_hand(CJ4_PLAYER_0);
    assert(cj4_calculate_shanten(
        &chiitoitsu_state,
        CJ4_PLAYER_0,
        &shanten));
    assert(shanten.chiitoitsu == 0);
    assert(cj4_calculate_shanten_after_discard(
        &chiitoitsu_state,
        CJ4_PLAYER_0,
        tile(27, 1),
        &shanten));
    assert(shanten.chiitoitsu == 0);

    set_hand(
        &kokushi_state,
        CJ4_PLAYER_0,
        kokushi,
        (uint8_t)(sizeof(kokushi) / sizeof(kokushi[0])));
    assert(cj4_calculate_shanten(&kokushi_state, CJ4_PLAYER_0, &shanten));
    assert(shanten.kokushi == 0);
    assert(cj4_collect_waiting_tile_types(&kokushi_state, CJ4_PLAYER_0, &waits));
    for (uint8_t type = 0; type < CJ4_TILE_TYPE_COUNT; ++type)
    {
        if (type == 0 || type == 8 || type == 9 || type == 17 ||
            type == 18 || type == 26 || type >= 27)
        {
            assert(waits.types[type] == 1);
            assert(waits.count[type] == 3);
        }
        else
        {
            assert(waits.types[type] == 0);
        }
    }

    kokushi_state.locations[tile(0, 1)].placement =
        cj4_location_make_hand(CJ4_PLAYER_0);
    assert(cj4_calculate_shanten(&kokushi_state, CJ4_PLAYER_0, &shanten));
    assert(shanten.kokushi == 0);
    assert(cj4_calculate_shanten_after_discard(
        &kokushi_state,
        CJ4_PLAYER_0,
        tile(0, 1),
        &shanten));
    assert(shanten.kokushi == 0);
}

static void
test_open_hand_uses_standard_shanten_only(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_shanten_result shanten;
    const cj4_meld meld = {
        .tiles = {tile(0, 0), tile(1, 0), tile(2, 0)},
        .size = 3,
        .type = CJ4_MELD_CHI,
        .from_player = CJ4_PLAYER_1,
        .called_index = 0};
    const cj4_tile_id hand[] = {
        tile(9, 0), tile(10, 0), tile(11, 0),
        tile(18, 0), tile(19, 0), tile(20, 0),
        tile(3, 0), tile(4, 0),
        tile(27, 0), tile(27, 1)};

    set_test_meld(&state, CJ4_PLAYER_0, 0, &meld);
    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));

    assert(cj4_calculate_shanten(&state, CJ4_PLAYER_0, &shanten));
    assert(shanten.standard == 0);
    assert(shanten.chiitoitsu == CJ4_SHANTEN_NOT_APPLICABLE);
    assert(shanten.kokushi == CJ4_SHANTEN_NOT_APPLICABLE);
}

static void
test_fourteen_tile_analysis_and_after_discard_waits(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_shanten_result automatic;
    cj4_shanten_result after_discard;
    cj4_waiting_tile_types waits;
    cj4_player_view view;
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(3, 0),
        tile(4, 0),
        tile(15, 0),
        tile(15, 1),
        tile(33, 0)};
    cj4_tile_id discard = tile(33, 0);

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));

    assert(cj4_calculate_shanten(&state, CJ4_PLAYER_0, &automatic));
    assert(automatic.standard == 0);
    assert(cj4_calculate_shanten_after_discard(
        &state,
        CJ4_PLAYER_0,
        discard,
        &after_discard));
    assert(after_discard.standard == 0);
    assert(!cj4_collect_waiting_tile_types(&state, CJ4_PLAYER_0, &waits));
    assert(cj4_collect_waiting_tile_types_after_discard(
        &state,
        CJ4_PLAYER_0,
        discard,
        &waits));
    assert(waits.types[2] == 1);
    assert(waits.types[5] == 1);
    assert(!cj4_calculate_shanten_after_discard(
        &state,
        CJ4_PLAYER_0,
        tile(32, 0),
        &after_discard));

    view = cj4m_make_player_view(&state, CJ4_PLAYER_0);
    assert(cj4p_calculate_shanten_after_discard(
        &view,
        discard,
        &after_discard));
    assert(after_discard.standard == 0);
    assert(cj4p_collect_waiting_tile_types_after_discard(
        &view,
        discard,
        &waits));
    assert(waits.types[2] == 1);
    assert(waits.types[5] == 1);
}

static void
test_waits_match_existing_shape_check(
    void)
{
    uint32_t random = 0x4c4a3401u;

    for (uint8_t sample = 0; sample < 24; ++sample)
    {
        cj4_mahjong state = make_empty_state();
        cj4_waiting_tile_types waits;
        cj4_tile_id hand[13];

        for (uint8_t i = 0; i < 13;)
        {
            bool duplicate = false;
            cj4_tile_id candidate;

            random = random * 1664525u + 1013904223u;
            candidate = (cj4_tile_id)(random % CJ4_TILE_ID_COUNT);
            for (uint8_t j = 0; j < i; ++j)
                if (hand[j] == candidate)
                    duplicate = true;
            if (duplicate)
                continue;
            hand[i++] = candidate;
        }

        set_hand(&state, CJ4_PLAYER_0, hand, 13);
        assert(cj4_collect_waiting_tile_types(
            &state,
            CJ4_PLAYER_0,
            &waits));

        for (cj4_tile_type type = CJ4_TILE_TYPE_MIN;
             type <= CJ4_TILE_TYPE_MAX;
             ++type)
        {
            cj4_mahjong completed = state;
            cj4_tile_id candidate = CJ4_TILE_ID_INVALID;

            for (uint8_t index = 0; index < CJ4_TILE_PER_TYPE; ++index)
            {
                cj4_tile_id current = tile(type, index);

                if (!cj4_location_is_hand(
                        completed.locations[current].placement))
                {
                    candidate = current;
                    break;
                }
            }

            if (candidate == CJ4_TILE_ID_INVALID)
            {
                assert(waits.types[type] == 0);
                continue;
            }

            completed.locations[candidate].placement =
                cj4_location_make_hand(CJ4_PLAYER_0);
            assert(waits.types[type] ==
                   (uint8_t)cj4_is_complete_hand(
                       &completed,
                       CJ4_PLAYER_0));
        }
    }
}

static void
test_player_view_analysis_matches_player_information(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_player_view view;
    cj4_shanten_result state_shanten;
    cj4_shanten_result view_shanten;
    cj4_waiting_tile_types state_waits;
    cj4_waiting_tile_types view_waits;
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(3, 0),
        tile(4, 0),
        tile(15, 0),
        tile(15, 1)};

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    add_discard(&state, CJ4_PLAYER_1, tile(5, 0));
    set_hand(&state, CJ4_PLAYER_2, &(cj4_tile_id){tile(5, 1)}, 1);
    state.wall_pos = 10;
    state.dead_wall_draw_count = 2;

    view = cj4m_make_player_view(&state, CJ4_PLAYER_0);

    assert(cj4_calculate_shanten(&state, CJ4_PLAYER_0, &state_shanten));
    assert(cj4p_calculate_shanten(&view, &view_shanten));
    assert(memcmp(&state_shanten, &view_shanten, sizeof(state_shanten)) == 0);
    assert(cj4_collect_waiting_tile_types(&state, CJ4_PLAYER_0, &state_waits));
    assert(cj4p_collect_waiting_tile_types(&view, &view_waits));
    assert(memcmp(&state_waits, &view_waits, sizeof(state_waits)) == 0);
    assert(cj4p_is_shape_tenpai(&view));
    assert(cj4_live_wall_remaining(&state) == 110);
    assert(view.live_wall_remaining == 110);

    state.dead_wall_draw_count = 9;
    assert(cj4_live_wall_remaining(&state) == 108);
}

static void
test_hand_analysis_rejects_invalid_inputs(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_shanten_result shanten;
    cj4_waiting_tile_types waits;

    assert(!cj4_calculate_shanten(NULL, CJ4_PLAYER_0, &shanten));
    assert(!cj4_calculate_shanten(&state, CJ4_PLAYER_COUNT, &shanten));
    assert(!cj4_calculate_shanten(&state, CJ4_PLAYER_0, &shanten));
    assert(!cj4_collect_waiting_tile_types(&state, CJ4_PLAYER_0, &waits));
    assert(!cj4p_calculate_shanten(NULL, &shanten));
    assert(!cj4p_collect_waiting_tile_types(NULL, &waits));
    assert(cj4_live_wall_remaining(NULL) == 0);
}

void
cj4_test_hand_analysis(
    void)
{
    test_standard_shanten_and_wait_counts();
    test_standard_shanten_distance();
    test_special_hand_shanten();
    test_open_hand_uses_standard_shanten_only();
    test_fourteen_tile_analysis_and_after_discard_waits();
    test_waits_match_existing_shape_check();
    test_player_view_analysis_matches_player_information();
    test_hand_analysis_rejects_invalid_inputs();
}
