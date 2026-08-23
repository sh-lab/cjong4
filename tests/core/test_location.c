#include <assert.h>
#include <string.h>

#include "../../src/core/hand_check.h"
#include "../../src/core/state_ops.h"
#include "../../src/core/state_score.h"
#include "../test_support.h"
#include "cjong4/core/rules.h"
#include "cjong4/core/state.h"
#include "cjong4/core/state_abortive.h"
#include "cjong4/core/state_chi.h"
#include "cjong4/core/state_discard.h"
#include "cjong4/core/state_init.h"
#include "cjong4/core/state_kan.h"
#include "cjong4/core/state_pass.h"
#include "cjong4/core/state_pon.h"
#include "cjong4/core/state_query.h"
#include "cjong4/core/state_riichi.h"
#include "cjong4/core/state_ron.h"
#include "cjong4/core/state_round.h"
#include "cjong4/core/state_settle.h"
#include "cjong4/core/state_tsumo.h"
#include "cjong4/manager/manager.h"

static void
test_v3_packed_location_and_state_layout(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_discard_list discards;
    cj4_location unknown;

    assert(sizeof(cj4_location) == 4);
    assert(sizeof(cj4_mahjong) >= sizeof(cj4_location) * CJ4_TILE_ID_COUNT);
    assert(sizeof(cj4_mahjong) <= 700);
    assert(cj4_tile_id_is_valid(CJ4_TILE_ID_MIN));
    assert(cj4_tile_id_is_valid(CJ4_TILE_ID_MAX));
    assert(!cj4_tile_id_is_valid(136));
    assert(!cj4_tile_id_is_valid(CJ4_TILE_ID_INVALID));

    memset(&unknown, CJ4_LOCATION_NONE, sizeof(unknown));
    assert(cj4_location_is_unknown(&unknown));
    assert(!cj4_location_is_unknown(NULL));
    unknown = cj4_location_get(NULL, 0);
    assert(cj4_location_is_unknown(&unknown));
    unknown = cj4_location_get(state.locations, CJ4_TILE_ID_INVALID);
    assert(cj4_location_is_unknown(&unknown));

    assert(cj4_location_make_discard(CJ4_PLAYER_3, 30, true) == 0xfe);
    assert(cj4_location_is_discard(0x00));
    assert(cj4_location_is_discard(0x1e));
    assert(cj4_location_is_discard(0x80));
    assert(cj4_location_is_discard(0xfe));
    assert(!cj4_location_is_discard(0x1f));
    assert(!cj4_location_is_discard(0x7f));
    assert(!cj4_location_is_discard(CJ4_LOCATION_NONE));
    assert(cj4_location_discard_player(CJ4_LOCATION_NONE) == CJ4_PLAYER_COUNT);
    assert(cj4_location_discard_index(CJ4_LOCATION_NONE) == 31);
    assert(!cj4_location_discard_is_tsumogiri(CJ4_LOCATION_NONE));
    assert(cj4_location_make_hand(CJ4_PLAYER_3) == 0x60);
    assert(cj4_location_make_meld(
               CJ4_PLAYER_3,
               3,
               CJ4_MELD_KAKAN) == 0xfc);
    assert(cj4_location_make_discard_history(85, true) == 0xd5);
    assert(cj4_location_is_hand(0x00));
    assert(cj4_location_is_hand(0x60));
    assert(!cj4_location_is_hand(0x01));
    assert(!cj4_location_is_hand(0x7f));
    assert(!cj4_location_is_hand(CJ4_LOCATION_NONE));
    assert(cj4_location_placement_player(0x01) == CJ4_PLAYER_COUNT);
    assert(cj4_location_is_meld(0x80));
    assert(cj4_location_is_meld(0xfc));
    assert(!cj4_location_is_meld(0x85));
    assert(!cj4_location_is_meld(0x86));
    assert(!cj4_location_is_meld(0x87));
    assert(!cj4_location_is_meld(CJ4_LOCATION_NONE));
    assert(cj4_location_meld_group(CJ4_LOCATION_NONE) == 4);
    assert(cj4_location_meld_type(CJ4_LOCATION_NONE) == CJ4_MELD_INVALID);
    assert(cj4_location_is_discard_history(0x00));
    assert(cj4_location_is_discard_history(0xd5));
    assert(!cj4_location_is_discard_history(0x56));
    assert(!cj4_location_is_discard_history(CJ4_LOCATION_NONE));
    assert(cj4_location_discard_history_index(CJ4_LOCATION_NONE) == 86);
    assert(!cj4_location_discard_is_riichi(CJ4_LOCATION_NONE));
    assert(cj4_get_wall_tile(&state, CJ4_LOCATION_NONE) ==
           CJ4_TILE_ID_INVALID);

    state.locations[0].discard = 0x1f;
    state.locations[0].discard_history =
        cj4_location_make_discard_history(0, false);
    state.discard_count = 1;
    discards = cj4_location_collect_discards(state.locations);
    assert(discards.count == 0);
    assert(discards.items[0].tile == CJ4_TILE_ID_INVALID);
    assert(discards.items[0].player == CJ4_PLAYER_COUNT);

    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_2);
    cj4_state_set_first_turn(&state, true);
    cj4_state_set_chankan(&state, true);
    assert(state.progress == 0xe4);

    cj4_state_set_riichi(&state, CJ4_PLAYER_1, true);
    cj4_state_set_ippatsu(&state, CJ4_PLAYER_3, true);
    assert(state.riichi_ippatsu == 0x82);
    cj4_state_set_temporary_furiten(&state, CJ4_PLAYER_0, true);
    cj4_state_set_riichi_furiten(&state, CJ4_PLAYER_2, true);
    assert(state.furiten == 0x41);
    cj4_state_set_draw_turn(&state, CJ4_PLAYER_0, 1);
    cj4_state_set_draw_turn(&state, CJ4_PLAYER_1, 2);
    assert(state.draw_turns == 0x09);

    cj4_state_set_pao(
        &state,
        CJ4_PLAYER_2,
        CJ4_PLAYER_1,
        CJ4_PAO_DAISUUSHII);
    assert(state.pao[1] == 0x09);
    cj4_state_set_round_result(
        &state,
        CJ4_ROUND_END_ABORTIVE_DRAW,
        CJ4_ABORTIVE_DRAW_FOUR_KANS);
    assert(state.round_result == 0x24);
}

static void
test_v3_location_reconstruction_and_player_mask(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_tile_id own = tile(0, 0);
    cj4_tile_id hidden = tile(1, 0);
    cj4_tile_id called = tile(2, 0);
    cj4_tile_id hand1 = tile(2, 1);
    cj4_tile_id hand2 = tile(2, 2);
    cj4_tile_id dora = tile(3, 0);
    cj4_player_view view;
    cj4_hand hand;
    cj4_discard_list discards;
    cj4_discard_list player_discards;
    cj4_meld_list melds;
    cj4_dora_indicator_list indicators;

    set_hand(&state, CJ4_PLAYER_0, &own, 1);
    set_hand(&state, CJ4_PLAYER_1, &hidden, 1);
    add_discard(&state, CJ4_PLAYER_3, called);
    set_hand(&state, CJ4_PLAYER_2, &hand1, 1);
    set_hand(&state, CJ4_PLAYER_2, &hand2, 1);
    set_test_wall(&state, 130, dora);
    state.dora_count = 1;
    set_test_meld(&state, CJ4_PLAYER_2, 0, &(cj4_meld){.tiles = {called, hand1, hand2}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_3, .called_index = 0});

    hand = cj4_location_collect_hand(state.locations, CJ4_PLAYER_0);
    assert(hand.count == 1);
    assert(hand.items[0] == own);
    assert(hand.items[1] == CJ4_TILE_ID_INVALID);
    discards = cj4_location_collect_discards(state.locations);
    assert(discards.count == 1);
    assert(discards.items[0].tile == called);
    assert(!discards.items[0].is_active);
    player_discards = cj4_location_collect_player_discards(
        state.locations,
        CJ4_PLAYER_3);
    assert(player_discards.count == 1);
    assert(player_discards.items[0].tile == called);
    melds = cj4_location_collect_melds(state.locations, CJ4_PLAYER_2);
    assert(melds.count == 1);
    assert(melds.items[0].type == CJ4_MELD_PON);
    assert(melds.items[0].from_player == CJ4_PLAYER_3);
    assert(melds.items[0].tiles[melds.items[0].called_index] == called);
    indicators = cj4_location_collect_dora_indicators(state.locations);
    assert(indicators.count == CJ4_MAX_DORA_INDICATORS);
    assert(indicators.items[0] == dora);

    hand = cj4_location_collect_hand(NULL, CJ4_PLAYER_0);
    assert(hand.count == 0);
    assert(hand.items[0] == CJ4_TILE_ID_INVALID);
    melds = cj4_location_collect_melds(
        state.locations,
        CJ4_PLAYER_COUNT);
    assert(melds.count == 0);
    assert(melds.items[0].tiles[0] == CJ4_TILE_ID_INVALID);
    assert(melds.items[0].type == CJ4_MELD_INVALID);
    assert(melds.items[0].from_player == CJ4_PLAYER_COUNT);
    assert(melds.items[0].called_index == CJ4_CALLED_INDEX_NONE);
    player_discards = cj4_location_collect_player_discards(NULL, CJ4_PLAYER_0);
    assert(player_discards.count == 0);
    assert(player_discards.items[0].tile == CJ4_TILE_ID_INVALID);
    indicators = cj4_location_collect_dora_indicators(NULL);
    assert(indicators.count == 0);
    assert(indicators.items[0] == CJ4_TILE_ID_INVALID);

    view = cj4m_make_player_view(&state, CJ4_PLAYER_0);
    assert(view.locations[own].wall != CJ4_LOCATION_NONE);
    assert(cj4_location_is_unknown(&view.locations[hidden]));
    assert(view.locations[called].wall == CJ4_LOCATION_NONE);
    assert(view.locations[called].discard != CJ4_LOCATION_NONE);
    assert(cj4_location_is_meld(view.locations[called].placement));
    assert(view.locations[dora].wall == 130);

    state.draw_tile = 200;
    view = cj4m_make_player_view(&state, CJ4_PLAYER_0);
    assert(view.draw_tile == CJ4_TILE_ID_INVALID);
}

static void
test_v3_initial_state_uses_locations_as_canonical_wall(
    void)
{
    cj4_tile_id wall[CJ4_TILE_ID_COUNT];
    cj4_rules rules = cj4_rules_default();
    cj4_mahjong unchanged;
    uint8_t hand_count = 0;

    for (uint16_t i = 0; i < CJ4_TILE_ID_COUNT; ++i)
        wall[i] = (cj4_tile_id)i;

    assert(cj4_wall_is_valid(wall));
    cj4_mahjong state = cj4_create_initial_state(wall, &rules);
    for (uint16_t i = 0; i < CJ4_TILE_ID_COUNT; ++i)
    {
        assert(state.locations[i].wall == i);
        if (cj4_location_is_hand(state.locations[i].placement))
            hand_count++;
    }

    assert(hand_count == 53);
    assert(state.wall_pos == 53);
    assert(state.draw_tile != CJ4_TILE_ID_INVALID);
    assert(state.last_discard_tile == CJ4_TILE_ID_INVALID);
    assert(state.discard_count == 0);
    assert(state.dora_count == 1);

    wall[0] = wall[1];
    assert(!cj4_wall_is_valid(wall));
    state = cj4_create_initial_state(wall, &rules);
    assert(cj4_state_phase(&state) == CJ4_PHASE_GAME_END);
    assert(state.draw_tile == CJ4_TILE_ID_INVALID);

    wall[0] = CJ4_TILE_ID_INVALID;
    assert(!cj4_wall_is_valid(wall));
    assert(!cj4_wall_is_valid(NULL));

    state = make_empty_state();
    cj4_state_set_phase(&state, CJ4_PHASE_SETTLE);
    state.settlement_should_end = 0;
    unchanged = cj4_do_next_round(state, wall, &rules);
    assert(memcmp(&state, &unchanged, sizeof(state)) == 0);
}

static void
test_v2_rejects_out_of_range_winning_tile(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_win_result result;
    uint8_t count = 1;

    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    cj4_state_set_round_result(&state, CJ4_ROUND_END_RON, CJ4_ABORTIVE_DRAW_NONE);
    state.winner_mask = 1;
    state.winning_tile = 200;

    assert(!cj4_collect_winning_results(&state, NULL, &result, 1, &count));
    assert(count == 0);
}

void
cj4_test_location(
    void)
{
    test_v3_packed_location_and_state_layout();
    test_v3_location_reconstruction_and_player_mask();
    test_v3_initial_state_uses_locations_as_canonical_wall();
    test_v2_rejects_out_of_range_winning_tile();
}
