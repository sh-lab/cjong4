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
    cj4_rules rules = cj4_rules_default();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong round_end;
    cj4_mahjong settled;

    rules.four_kans_abort_timing = CJ4_FOUR_KANS_ABORT_IMMEDIATE;

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
test_fourth_kan_can_abort_after_rinshan_discard_and_ron(
    void)
{
    cj4_rules rules = cj4_rules_default();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong after_ankan;
    cj4_mahjong after_rinshan;
    cj4_mahjong after_discard;
    cj4_mahjong won;
    cj4_mahjong round_end;
    cj4_player winner = CJ4_PLAYER_3;
    const cj4_tile_id ankan_tiles[] = {
        tile(4, 0),
        tile(4, 1),
        tile(4, 2),
        tile(4, 3)};
    const cj4_tile_id winning_hand[] = {
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(21, 0),
        tile(22, 0),
        tile(23, 0),
        tile(24, 1),
        tile(25, 1),
        tile(27, 0),
        tile(27, 1)};
    const cj4_tile_id chi_hand[] = {tile(24, 2), tile(25, 2)};
    const cj4_tile_id pon_hand[] = {tile(26, 1), tile(26, 2)};
    cj4_tile_id rinshan = tile(26, 0);

    set_test_kan(&state, CJ4_PLAYER_0, 0, CJ4_MELD_ANKAN, tile(0, 0));
    set_test_kan(&state, CJ4_PLAYER_1, 0, CJ4_MELD_MINKAN, tile(1, 0));
    set_test_kan(&state, CJ4_PLAYER_2, 0, CJ4_MELD_KAKAN, tile(2, 0));
    set_hand(&state, CJ4_PLAYER_0, ankan_tiles, 4);
    set_hand(
        &state,
        CJ4_PLAYER_3,
        winning_hand,
        (uint8_t)(sizeof(winning_hand) / sizeof(winning_hand[0])));
    set_hand(&state, CJ4_PLAYER_1, chi_hand, 2);
    set_hand(&state, CJ4_PLAYER_2, pon_hand, 2);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_riichi(&state, CJ4_PLAYER_3, 1);
    state.draw_tile = ankan_tiles[3];
    state.dead_wall_draw_count = 3;
    set_test_wall(&state, 133, rinshan);

    after_ankan = cj4_do_ankan(
        state,
        ankan_tiles[0],
        ankan_tiles[1],
        ankan_tiles[2],
        ankan_tiles[3]);
    after_rinshan = cj4_do_rinshan_draw(after_ankan, &rules);

    assert(cj4_state_phase(&after_rinshan) == CJ4_PHASE_DRAW);
    assert(after_rinshan.draw_tile == rinshan);
    assert(after_rinshan.dead_wall_draw_count == 4);

    after_discard = cj4_do_discard_with_rules(after_rinshan, &rules, rinshan);

    assert(cj4_state_phase(&after_discard) == CJ4_PHASE_DISCARD);
    assert(cj4_can_ron(&after_discard, winner, &rules));
    assert(!cj4_can_chi(&after_discard, NULL));
    assert(!cj4_can_pon(&after_discard, NULL, CJ4_PLAYER_2));
    assert(!cj4_can_minkan(&after_discard, CJ4_PLAYER_2));

    won = cj4_do_ron_multi(after_discard, &winner, 1, &rules);
    assert(cj4_state_round_end_type(&won) == CJ4_ROUND_END_RON);

    round_end = cj4_do_pass(after_discard, &rules);
    assert(cj4_state_phase(&round_end) == CJ4_PHASE_ROUND_END);
    assert(cj4_state_round_end_type(&round_end) == CJ4_ROUND_END_ABORTIVE_DRAW);
    assert(cj4_state_abortive_reason(&round_end) == CJ4_ABORTIVE_DRAW_FOUR_KANS);
}

static void
test_fourth_minkan_and_kakan_can_draw_rinshan_when_delayed(
    void)
{
    cj4_rules rules = cj4_rules_default();
    cj4_mahjong minkan_state = make_empty_state();
    cj4_mahjong kakan_state = make_empty_state();
    cj4_mahjong next;
    const cj4_tile_id minkan_tiles[] = {
        tile(3, 1),
        tile(3, 2),
        tile(3, 3)};
    const cj4_meld pon = {
        .tiles = {tile(3, 0), tile(3, 1), tile(3, 2)},
        .size = 3,
        .type = CJ4_MELD_PON,
        .from_player = CJ4_PLAYER_0,
        .called_index = 0};
    cj4_tile_id added = tile(3, 3);
    cj4_tile_id rinshan = tile(4, 0);

    for (uint8_t group = 0; group < 3; ++group)
    {
        set_test_kan(
            &minkan_state,
            (cj4_player)group,
            0,
            CJ4_MELD_ANKAN,
            tile((cj4_tile_type)group, 0));
        set_test_kan(
            &kakan_state,
            (cj4_player)group,
            0,
            CJ4_MELD_ANKAN,
            tile((cj4_tile_type)group, 0));
    }

    set_hand(&minkan_state, CJ4_PLAYER_3, minkan_tiles, 3);
    cj4_state_set_current_player(&minkan_state, CJ4_PLAYER_2);
    cj4_state_set_phase(&minkan_state, CJ4_PHASE_DISCARD);
    minkan_state.dead_wall_draw_count = 3;
    add_discard(&minkan_state, CJ4_PLAYER_2, tile(3, 0));
    set_test_wall(&minkan_state, 133, rinshan);

    next = cj4_do_minkan(
        minkan_state,
        &rules,
        CJ4_PLAYER_3,
        minkan_tiles[0],
        minkan_tiles[1],
        minkan_tiles[2]);
    assert(cj4_state_phase(&next) == CJ4_PHASE_DRAW);
    assert(next.draw_tile == rinshan);
    assert(cj4_state_count_total_kans(&next) == 4);

    set_test_meld(&kakan_state, CJ4_PLAYER_3, 0, &pon);
    set_hand(&kakan_state, CJ4_PLAYER_3, &added, 1);
    cj4_state_set_current_player(&kakan_state, CJ4_PLAYER_3);
    cj4_state_set_phase(&kakan_state, CJ4_PHASE_DRAW);
    kakan_state.draw_tile = added;
    kakan_state.dead_wall_draw_count = 3;
    set_test_wall(&kakan_state, 133, rinshan);

    next = cj4_do_kakan(kakan_state, added);
    assert(cj4_state_phase(&next) == CJ4_PHASE_KAKAN_RESOLVE);
    next = cj4_do_rinshan_draw(next, &rules);
    assert(cj4_state_phase(&next) == CJ4_PHASE_DRAW);
    assert(next.draw_tile == rinshan);
    assert(cj4_state_count_total_kans(&next) == 4);
}

static void
test_fifth_kan_declarations_abort_immediately(
    void)
{
    cj4_rules rules = cj4_rules_default();
    cj4_mahjong ankan_state = make_empty_state();
    cj4_mahjong kakan_state = make_empty_state();
    cj4_mahjong minkan_state = make_empty_state();
    cj4_mahjong next;
    const cj4_tile_id fifth_ankan[] = {
        tile(4, 0),
        tile(4, 1),
        tile(4, 2),
        tile(4, 3)};
    const cj4_tile_id fifth_minkan[] = {
        tile(4, 1),
        tile(4, 2),
        tile(4, 3)};
    const cj4_meld pon = {
        .tiles = {tile(4, 0), tile(4, 1), tile(4, 2)},
        .size = 3,
        .type = CJ4_MELD_PON,
        .from_player = CJ4_PLAYER_2,
        .called_index = 0};
    cj4_tile_id added = tile(4, 3);

    for (uint8_t group = 0; group < 4; ++group)
    {
        set_test_kan(
            &ankan_state,
            CJ4_PLAYER_0,
            group,
            CJ4_MELD_ANKAN,
            tile((cj4_tile_type)group, 0));
        set_test_kan(
            &kakan_state,
            CJ4_PLAYER_0,
            group,
            CJ4_MELD_ANKAN,
            tile((cj4_tile_type)group, 0));
        set_test_kan(
            &minkan_state,
            CJ4_PLAYER_0,
            group,
            CJ4_MELD_ANKAN,
            tile((cj4_tile_type)group, 0));
    }

    set_hand(&ankan_state, CJ4_PLAYER_1, fifth_ankan, 4);
    cj4_state_set_current_player(&ankan_state, CJ4_PLAYER_1);
    cj4_state_set_phase(&ankan_state, CJ4_PHASE_DRAW);
    ankan_state.draw_tile = fifth_ankan[3];
    ankan_state.dead_wall_draw_count = 4;

    assert(cj4_can_ankan(&ankan_state));
    next = cj4_do_ankan(
        ankan_state,
        fifth_ankan[0],
        fifth_ankan[1],
        fifth_ankan[2],
        fifth_ankan[3]);
    assert(cj4_state_phase(&next) == CJ4_PHASE_ROUND_END);
    assert(cj4_state_abortive_reason(&next) == CJ4_ABORTIVE_DRAW_FOUR_KANS);
    assert(cj4_state_count_total_kans(&next) == 5);

    set_test_meld(&kakan_state, CJ4_PLAYER_1, 0, &pon);
    set_hand(&kakan_state, CJ4_PLAYER_1, &added, 1);
    cj4_state_set_current_player(&kakan_state, CJ4_PLAYER_1);
    cj4_state_set_phase(&kakan_state, CJ4_PHASE_DRAW);
    kakan_state.draw_tile = added;
    kakan_state.dead_wall_draw_count = 4;

    assert(cj4_can_kakan_with_tile(&kakan_state, added));
    next = cj4_do_kakan(kakan_state, added);
    assert(cj4_state_phase(&next) == CJ4_PHASE_ROUND_END);
    assert(cj4_state_abortive_reason(&next) == CJ4_ABORTIVE_DRAW_FOUR_KANS);
    assert(cj4_state_count_total_kans(&next) == 5);

    set_hand(&minkan_state, CJ4_PLAYER_2, fifth_minkan, 3);
    cj4_state_set_current_player(&minkan_state, CJ4_PLAYER_1);
    cj4_state_set_phase(&minkan_state, CJ4_PHASE_DISCARD);
    minkan_state.dead_wall_draw_count = 4;
    add_discard(&minkan_state, CJ4_PLAYER_1, tile(4, 0));

    assert(cj4_can_minkan(&minkan_state, CJ4_PLAYER_2));
    next = cj4_do_minkan(
        minkan_state,
        &rules,
        CJ4_PLAYER_2,
        fifth_minkan[0],
        fifth_minkan[1],
        fifth_minkan[2]);
    assert(cj4_state_phase(&next) == CJ4_PHASE_ROUND_END);
    assert(cj4_state_abortive_reason(&next) == CJ4_ABORTIVE_DRAW_FOUR_KANS);
    assert(cj4_state_count_total_kans(&next) == 5);
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
    test_fourth_kan_can_abort_after_rinshan_discard_and_ron();
    test_fourth_minkan_and_kakan_can_draw_rinshan_when_delayed();
    test_fifth_kan_declarations_abort_immediately();
    test_triple_ron_abortive_draw_rule();
    test_kyuushu_kyuuhai_aborts_on_first_draw();
    test_suufon_renda_aborts_after_reactions_pass();
    test_four_riichi_aborts_after_reactions_pass();
}
