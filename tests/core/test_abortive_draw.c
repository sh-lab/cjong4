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
test_kan_flow_aborts_on_fourth_kakan_without_noten_penalty(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong round_end;
    cj4_mahjong settled;

    rules.noten_penalty = 1;
    rules.noten_penalty_points = 3000;

    cj4_state_set_phase(&state, CJ4_PHASE_KAKAN_RESOLVE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.pending_kakan_tile = tile(1, 0);
    state.dead_wall_draw_count = 3;
    state.honba = 0;

    set_test_kan(&state, CJ4_PLAYER_0, 0, CJ4_MELD_ANKAN, tile(0, 0));
    set_test_kan(&state, CJ4_PLAYER_0, 1, CJ4_MELD_KAKAN, tile(1, 0));
    set_test_kan(&state, CJ4_PLAYER_1, 0, CJ4_MELD_MINKAN, tile(2, 0));
    set_test_kan(&state, CJ4_PLAYER_1, 1, CJ4_MELD_ANKAN, tile(3, 0));

    round_end = cj4_do_rinshan_draw(state, &rules);
    assert(cj4_state_phase(&round_end) == CJ4_PHASE_ROUND_END);
    assert(cj4_state_round_end_type(&round_end) == CJ4_ROUND_END_ABORTIVE_DRAW);
    assert(round_end.dead_wall_draw_count == 3);

    settled = cj4_do_settle(round_end, &rules);
    assert(cj4_state_phase(&settled) == CJ4_PHASE_SETTLE);
    assert(settled.scores[0] == 25000);
    assert(settled.scores[1] == 25000);
    assert(settled.scores[2] == 25000);
    assert(settled.scores[3] == 25000);
    assert(settled.next_dealer == CJ4_PLAYER_0);
    assert(settled.honba == 1);
}

static void
test_kan_flow_aborts_on_fourth_ankan(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_mahjong round_end;
    const cj4_tile_id ankan_tiles[] = {
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
        tile(14, 0),
        tile(14, 1)};

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.draw_tile = tile(14, 1);
    set_test_kan(&state, CJ4_PLAYER_0, 0, CJ4_MELD_ANKAN, tile(5, 0));
    set_test_kan(&state, CJ4_PLAYER_1, 0, CJ4_MELD_MINKAN, tile(6, 0));
    set_test_kan(&state, CJ4_PLAYER_2, 0, CJ4_MELD_KAKAN, tile(7, 0));

    round_end = cj4_do_ankan(
        state,
        ankan_tiles[0],
        ankan_tiles[1],
        ankan_tiles[2],
        ankan_tiles[3]);

    assert(cj4_state_phase(&round_end) == CJ4_PHASE_ANKAN_RESOLVE);

    round_end = cj4_do_rinshan_draw(round_end, NULL);

    assert(cj4_state_phase(&round_end) == CJ4_PHASE_ROUND_END);
    assert(cj4_state_round_end_type(&round_end) == CJ4_ROUND_END_ABORTIVE_DRAW);
    assert(round_end.draw_tile == CJ4_TILE_ID_INVALID);
    assert(round_end.dead_wall_draw_count == 0);
}

static void
test_triple_ron_abortive_draw_rule(
    void)
{
    cj4_rules rules = cj4_rules_default();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong round_end;
    cj4_mahjong settled;
    cj4_player claimers[] = {CJ4_PLAYER_1, CJ4_PLAYER_2, CJ4_PLAYER_3};

    rules.triple_ron_abortive_draw = 1;
    rules.max_ron_players = 3;

    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.dealer = CJ4_PLAYER_0;
    state.honba = 2;
    state.riichi_sticks = 1;
    add_discard(&state, CJ4_PLAYER_0, tile(0, 0));

    round_end = cj4_do_ron_multi(state, claimers, 3, &rules);

    assert(cj4_state_phase(&round_end) == CJ4_PHASE_ROUND_END);
    assert(cj4_state_round_end_type(&round_end) == CJ4_ROUND_END_ABORTIVE_DRAW);
    assert(cj4_state_abortive_reason(&round_end) == CJ4_ABORTIVE_DRAW_TRIPLE_RON);
    assert(cj4_state_winner_count(&round_end) == 0);

    settled = cj4_do_settle(round_end, &rules);
    assert(settled.honba == 3);
    assert(settled.riichi_sticks == 1);
    assert(settled.scores[CJ4_PLAYER_0] == 25000);
}

static void
test_kyuushu_kyuuhai_aborts_on_first_draw(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong next;
    cj4_tile_id draw = tile(30, 0);
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(8, 0),
        tile(9, 0),
        tile(17, 0),
        tile(18, 0),
        tile(26, 0),
        tile(27, 0),
        tile(28, 0),
        tile(29, 0),
        tile(2, 0),
        tile(3, 0),
        tile(4, 0),
        tile(5, 0),
        draw};

    rules.abortive_kyuushu_kyuuhai = 1;

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.draw_tile = draw;
    cj4_state_set_first_turn(&state, 1);
    cj4_state_set_draw_turn(&state, CJ4_PLAYER_0, 1);

    assert(cj4_can_kyuushu_kyuuhai(&state, &rules));

    next = cj4_do_kyuushu_kyuuhai(state);

    assert(cj4_state_phase(&next) == CJ4_PHASE_ROUND_END);
    assert(cj4_state_round_end_type(&next) == CJ4_ROUND_END_ABORTIVE_DRAW);
    assert(cj4_state_abortive_reason(&next) == CJ4_ABORTIVE_DRAW_KYUUSHU_KYUUHAI);
}

static void
test_suufon_renda_aborts_after_reactions_pass(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong next;

    rules.abortive_suufon_renda = 1;

    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_3);
    cj4_state_set_first_turn(&state, 1);
    add_discard(&state, CJ4_PLAYER_0, tile(27, 0));
    add_discard(&state, CJ4_PLAYER_1, tile(27, 1));
    add_discard(&state, CJ4_PLAYER_2, tile(27, 2));
    add_discard(&state, CJ4_PLAYER_3, tile(27, 3));

    next = cj4_do_pass(state, &rules);

    assert(cj4_state_phase(&next) == CJ4_PHASE_ROUND_END);
    assert(cj4_state_round_end_type(&next) == CJ4_ROUND_END_ABORTIVE_DRAW);
    assert(cj4_state_abortive_reason(&next) == CJ4_ABORTIVE_DRAW_SUUFON_RENDA);
}

static void
test_four_riichi_aborts_after_reactions_pass(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong next;

    rules.abortive_four_riichi = 1;

    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_3);
    add_discard(&state, CJ4_PLAYER_3, tile(4, 0));
    for (uint8_t player = 0; player < CJ4_PLAYER_COUNT; ++player)
        cj4_state_set_riichi(&state, player, 1);

    next = cj4_do_pass(state, &rules);

    assert(cj4_state_phase(&next) == CJ4_PHASE_ROUND_END);
    assert(cj4_state_round_end_type(&next) == CJ4_ROUND_END_ABORTIVE_DRAW);
    assert(cj4_state_abortive_reason(&next) == CJ4_ABORTIVE_DRAW_FOUR_RIICHI);
}

void
cj4_test_abortive_draw(
    void)
{
    test_kan_flow_aborts_on_fourth_kakan_without_noten_penalty();
    test_kan_flow_aborts_on_fourth_ankan();
    test_triple_ron_abortive_draw_rule();
    test_kyuushu_kyuuhai_aborts_on_first_draw();
    test_suufon_renda_aborts_after_reactions_pass();
    test_four_riichi_aborts_after_reactions_pass();
}
