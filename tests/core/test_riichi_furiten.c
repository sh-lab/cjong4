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
test_riichi_uses_shape_tenpai(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_tile_id draw = tile(26, 0);
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
        draw};

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    state.draw_tile = draw;

    assert(cj4_can_riichi(&state, draw));
}

static void
test_riichi_requires_four_live_wall_tiles(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_tile_id draw = tile(26, 0);
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
        draw};

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    state.draw_tile = draw;

    state.wall_pos = 118;
    assert(cj4_can_riichi(&state, draw));

    state.wall_pos = 119;
    assert(!cj4_can_riichi(&state, draw));
}

static void
test_riichi_establishes_after_pass(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_mahjong declared;
    cj4_mahjong established;
    cj4_tile_id draw = tile(26, 0);
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
        draw};

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    state.draw_tile = draw;
    cj4_state_set_first_turn(&state, 1);
    cj4_state_set_draw_turn(&state, CJ4_PLAYER_0, 1);
    set_test_wall(&state, (uint8_t)(0), tile(30, 0));

    declared = cj4_do_riichi(state, draw);

    assert(cj4_state_has_pending_riichi(&declared));
    assert(cj4_state_is_riichi(&declared, CJ4_PLAYER_0) == 0);
    assert(declared.scores[CJ4_PLAYER_0] == 25000);
    assert(declared.riichi_sticks == 0);

    established = cj4_do_pass(declared, NULL);

    assert(!cj4_state_has_pending_riichi(&established));
    assert(cj4_state_is_riichi(&established, CJ4_PLAYER_0) == 1);
    assert(cj4_state_is_ippatsu(&established, CJ4_PLAYER_0) == 1);
    assert(cj4_state_double_riichi(&established, CJ4_PLAYER_0) == 1);
    assert(established.scores[CJ4_PLAYER_0] == 24000);
    assert(established.riichi_sticks == 1);
    assert(!cj4_can_discard_with_rules(established, NULL, tile(0, 0)));
}

static void
test_riichi_ron_clears_pending_without_payment(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_mahjong declared;
    cj4_mahjong ron;
    cj4_player winner = CJ4_PLAYER_1;
    cj4_tile_id draw = tile(26, 0);
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
        draw};

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    state.draw_tile = draw;

    declared = cj4_do_riichi(state, draw);
    ron = cj4_do_ron_multi(declared, &winner, 1, NULL);

    assert(cj4_state_phase(&ron) == CJ4_PHASE_ROUND_END);
    assert(!cj4_state_has_pending_riichi(&ron));
    assert(cj4_state_is_riichi(&ron, CJ4_PLAYER_0) == 0);
    assert(ron.scores[CJ4_PLAYER_0] == 25000);
    assert(ron.riichi_sticks == 0);
}

static void
test_riichi_called_discard_still_establishes(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_mahjong declared;
    cj4_mahjong called;
    cj4_tile_id draw = tile(26, 0);
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
        draw};
    const cj4_tile_id pon_tiles[] = {tile(26, 1), tile(26, 2), tile(25, 0)};

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    set_hand(&state, CJ4_PLAYER_2, pon_tiles, 3);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    state.draw_tile = draw;

    declared = cj4_do_riichi(state, draw);
    called = cj4_do_pon(declared, NULL, CJ4_PLAYER_2, pon_tiles[0], pon_tiles[1]);

    assert(cj4_state_is_riichi(&called, CJ4_PLAYER_0) == 1);
    assert(!cj4_state_has_pending_riichi(&called));
    assert(called.scores[CJ4_PLAYER_0] == 24000);
    assert(called.riichi_sticks == 1);
    assert(cj4_state_is_ippatsu(&called, CJ4_PLAYER_0) == 0);
}

static void
test_temporary_furiten_blocks_ron(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong after_pass;
    cj4_mahjong later;
    const cj4_tile_id hand[] = {
        tile(1, 0),
        tile(2, 0),
        tile(3, 0),
        tile(10, 0),
        tile(11, 0),
        tile(12, 0),
        tile(19, 0),
        tile(20, 0),
        tile(21, 0),
        tile(4, 0),
        tile(5, 0),
        tile(14, 0),
        tile(14, 1)};

    rules.kuitan = 1;

    set_hand(&state, CJ4_PLAYER_2, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    add_discard(&state, CJ4_PLAYER_0, tile(6, 0));

    assert(cj4_can_ron(&state, CJ4_PLAYER_2, &rules));

    after_pass = cj4_do_pass(state, &rules);
    assert(cj4_state_temporary_furiten(&after_pass, CJ4_PLAYER_2) == 1);
    assert(cj4_state_riichi_furiten(&after_pass, CJ4_PLAYER_2) == 0);

    later = after_pass;
    cj4_state_set_phase(&later, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&later, CJ4_PLAYER_1);
    later.draw_tile = CJ4_TILE_ID_INVALID;
    add_discard(&later, CJ4_PLAYER_1, tile(3, 1));

    assert(!cj4_can_ron(&later, CJ4_PLAYER_2, &rules));
}

static void
test_riichi_furiten_is_recorded(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong after_pass;
    cj4_mahjong later;
    const cj4_tile_id hand[] = {
        tile(1, 0),
        tile(2, 0),
        tile(3, 0),
        tile(10, 0),
        tile(11, 0),
        tile(12, 0),
        tile(19, 0),
        tile(20, 0),
        tile(21, 0),
        tile(4, 0),
        tile(5, 0),
        tile(14, 0),
        tile(14, 1)};

    rules.kuitan = 1;

    set_hand(&state, CJ4_PLAYER_2, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    cj4_state_set_riichi(&state, CJ4_PLAYER_2, 1);
    add_discard(&state, CJ4_PLAYER_0, tile(6, 0));

    assert(cj4_can_ron(&state, CJ4_PLAYER_2, &rules));

    after_pass = cj4_do_pass(state, &rules);
    assert(cj4_state_temporary_furiten(&after_pass, CJ4_PLAYER_2) == 0);
    assert(cj4_state_riichi_furiten(&after_pass, CJ4_PLAYER_2) == 1);

    later = after_pass;
    cj4_state_set_phase(&later, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&later, CJ4_PLAYER_1);
    later.draw_tile = CJ4_TILE_ID_INVALID;
    add_discard(&later, CJ4_PLAYER_1, tile(3, 1));

    assert(!cj4_can_ron(&later, CJ4_PLAYER_2, &rules));
}

static void
test_permanent_furiten_blocks_other_wait(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    const cj4_tile_id hand[] = {
        tile(1, 0),
        tile(2, 0),
        tile(3, 0),
        tile(10, 0),
        tile(11, 0),
        tile(12, 0),
        tile(19, 0),
        tile(20, 0),
        tile(21, 0),
        tile(4, 0),
        tile(5, 0),
        tile(14, 0),
        tile(14, 1)};

    rules.kuitan = 1;

    set_hand(&state, CJ4_PLAYER_2, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    add_discard(&state, CJ4_PLAYER_2, tile(6, 0));
    add_discard(&state, CJ4_PLAYER_0, tile(3, 1));

    assert(!cj4_can_ron(&state, CJ4_PLAYER_2, &rules));
}

static void
test_riichi_restricts_actions(
    void)
{
    cj4_mahjong discard_state = make_empty_state();
    cj4_mahjong chi_state = make_empty_state();
    cj4_mahjong pon_state = make_empty_state();
    cj4_mahjong minkan_state = make_empty_state();
    cj4_mahjong kakan_state = make_empty_state();
    cj4_tile_id draw = tile(8, 0);
    const cj4_tile_id discard_hand[] = {
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
        draw};
    const cj4_tile_id chi_hand[] = {
        tile(1, 0),
        tile(2, 0)};
    const cj4_tile_id pon_hand[] = {
        tile(3, 1),
        tile(3, 2)};
    const cj4_tile_id minkan_hand[] = {
        tile(3, 1),
        tile(3, 2),
        tile(3, 3)};
    const cj4_tile_id kakan_hand[] = {
        tile(4, 3),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(12, 0),
        tile(13, 0),
        tile(14, 0),
        tile(21, 0),
        tile(22, 0),
        tile(23, 0),
        tile(24, 0)};

    set_hand(&discard_state, CJ4_PLAYER_0, discard_hand, (uint8_t)(sizeof(discard_hand) / sizeof(discard_hand[0])));
    cj4_state_set_phase(&discard_state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&discard_state, CJ4_PLAYER_0);
    discard_state.draw_tile = draw;
    cj4_state_set_riichi(&discard_state, CJ4_PLAYER_0, 1);

    assert(cj4_can_discard_with_rules(discard_state, NULL, draw));
    assert(!cj4_can_discard_with_rules(discard_state, NULL, tile(0, 0)));

    set_hand(&chi_state, CJ4_PLAYER_1, chi_hand, (uint8_t)(sizeof(chi_hand) / sizeof(chi_hand[0])));
    cj4_state_set_phase(&chi_state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&chi_state, CJ4_PLAYER_0);
    cj4_state_set_riichi(&chi_state, CJ4_PLAYER_1, 1);
    add_discard(&chi_state, CJ4_PLAYER_0, tile(3, 0));

    assert(!cj4_can_chi(&chi_state, NULL));
    assert(!cj4_can_chi_with_tile(&chi_state, NULL, chi_hand[0], chi_hand[1]));

    set_hand(&pon_state, CJ4_PLAYER_2, pon_hand, (uint8_t)(sizeof(pon_hand) / sizeof(pon_hand[0])));
    cj4_state_set_phase(&pon_state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&pon_state, CJ4_PLAYER_0);
    cj4_state_set_riichi(&pon_state, CJ4_PLAYER_2, 1);
    add_discard(&pon_state, CJ4_PLAYER_0, tile(3, 0));

    assert(!cj4_can_pon(&pon_state, NULL, CJ4_PLAYER_2));
    assert(!cj4_can_pon_with_tile(&pon_state, NULL, CJ4_PLAYER_2, pon_hand[0], pon_hand[1]));

    set_hand(&minkan_state, CJ4_PLAYER_2, minkan_hand, (uint8_t)(sizeof(minkan_hand) / sizeof(minkan_hand[0])));
    cj4_state_set_phase(&minkan_state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&minkan_state, CJ4_PLAYER_0);
    cj4_state_set_riichi(&minkan_state, CJ4_PLAYER_2, 1);
    add_discard(&minkan_state, CJ4_PLAYER_0, tile(3, 0));

    assert(!cj4_can_minkan(&minkan_state, CJ4_PLAYER_2));
    assert(!cj4_can_minkan_with_tile(&minkan_state, CJ4_PLAYER_2, minkan_hand[0], minkan_hand[1], minkan_hand[2]));

    set_hand(&kakan_state, CJ4_PLAYER_0, kakan_hand, (uint8_t)(sizeof(kakan_hand) / sizeof(kakan_hand[0])));
    cj4_state_set_phase(&kakan_state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&kakan_state, CJ4_PLAYER_0);
    cj4_state_set_riichi(&kakan_state, CJ4_PLAYER_0, 1);
    kakan_state.draw_tile = kakan_hand[0];
    set_test_meld(&kakan_state, CJ4_PLAYER_0, 0, &(cj4_meld){.tiles = {tile(4, 0), tile(4, 1), tile(4, 2)}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_1, .called_index = 0});

    assert(!cj4_can_kakan(&kakan_state));
    assert(!cj4_can_kakan_with_tile(&kakan_state, kakan_hand[0]));
}

static void
test_seven_pairs_rejects_quad(
    void)
{
    cj4_mahjong state = make_empty_state();
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(0, 1),
        tile(0, 2),
        tile(0, 3),
        tile(9, 0),
        tile(9, 1),
        tile(10, 0),
        tile(10, 1),
        tile(18, 0),
        tile(18, 1),
        tile(19, 0),
        tile(19, 1),
        tile(27, 0),
        tile(27, 1)};

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));

    assert(!cj4_is_complete_hand(&state, CJ4_PLAYER_0));
}

static void
test_riichi_ankan_keeps_waits(
    void)
{
    cj4_mahjong state = make_empty_state();
    const cj4_tile_id kan_tiles[] = {
        tile(0, 0),
        tile(0, 1),
        tile(0, 2),
        tile(0, 3)};
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(0, 1),
        tile(0, 2),
        tile(0, 3),
        tile(10, 0),
        tile(11, 0),
        tile(12, 0),
        tile(19, 0),
        tile(20, 0),
        tile(21, 0),
        tile(4, 0),
        tile(5, 0),
        tile(15, 0),
        tile(15, 1)};

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    cj4_state_set_riichi(&state, CJ4_PLAYER_0, 1);
    state.draw_tile = tile(0, 3);

    assert(cj4_can_ankan_with_tile(
        &state,
        kan_tiles[0],
        kan_tiles[1],
        kan_tiles[2],
        kan_tiles[3]));
}

void
cj4_test_riichi_furiten(
    void)
{
    test_riichi_uses_shape_tenpai();
    test_riichi_requires_four_live_wall_tiles();
    test_riichi_establishes_after_pass();
    test_riichi_ron_clears_pending_without_payment();
    test_riichi_called_discard_still_establishes();
    test_temporary_furiten_blocks_ron();
    test_riichi_furiten_is_recorded();
    test_permanent_furiten_blocks_other_wait();
    test_riichi_restricts_actions();
    test_seven_pairs_rejects_quad();
    test_riichi_ankan_keeps_waits();
}
