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
test_exhaustive_draw_uses_shape_tenpai(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;
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

    rules.noten_penalty = 1;
    rules.noten_penalty_points = 3000;

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    cj4_state_set_round_result(&state, CJ4_ROUND_END_EXHAUSTIVE_DRAW, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.dealer = CJ4_PLAYER_0;
    state.winner_mask = 0;

    settled = cj4_do_settle(state, &rules);

    assert(settled.scores[CJ4_PLAYER_0] == 28000);
    assert(settled.scores[CJ4_PLAYER_1] == 24000);
    assert(settled.scores[CJ4_PLAYER_2] == 24000);
    assert(settled.scores[CJ4_PLAYER_3] == 24000);
    assert(settled.next_dealer == CJ4_PLAYER_0);
}

static void
test_exhaustive_draw_uses_live_wall_not_discard_count(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_mahjong drawn;
    cj4_mahjong discarded;
    cj4_mahjong round_end;
    cj4_tile_id last_draw = tile(33, 3);

    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    cj4_state_set_first_turn(&state, 0);
    state.discard_count = CJ4_MAX_DRAWS;
    state.wall_pos = 120;
    state.dead_wall_draw_count = 1;
    set_test_wall(&state, (uint8_t)(120), last_draw);

    drawn = cj4_do_pass(state, NULL);

    assert(cj4_state_phase(&drawn) == CJ4_PHASE_DRAW);
    assert(cj4_state_current_player(&drawn) == CJ4_PLAYER_1);
    assert(drawn.draw_tile == last_draw);
    assert(drawn.wall_pos == 121);

    discarded = cj4_do_discard(drawn, NULL, last_draw);
    round_end = cj4_do_pass(discarded, NULL);

    assert(cj4_state_phase(&round_end) == CJ4_PHASE_ROUND_END);
    assert(cj4_state_round_end_type(&round_end) == CJ4_ROUND_END_EXHAUSTIVE_DRAW);
}

static void
test_haitei_uses_kan_adjusted_live_wall_end(
    void)
{
    cj4_rules rules = cj4_rules_default();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong won;
    cj4_win_result results[CJ4_PLAYER_COUNT];
    uint8_t result_count = 0;
    cj4_tile_id draw = tile(5, 0);
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
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.dealer = CJ4_PLAYER_1;
    state.draw_tile = draw;
    state.wall_pos = 121;
    state.dead_wall_draw_count = 1;
    set_test_wall(&state, (uint8_t)(120), draw);

    won = cj4_do_tsumo(state);

    assert(cj4_collect_winning_results(
        &won,
        &rules,
        results,
        CJ4_PLAYER_COUNT,
        &result_count));
    assert(result_count == 1);
    assert(contains_win_yaku(&results[0], CJ4_WIN_YAKU_HAITEI));
}

static void
test_exhaustive_draw_resets_honba_when_dealer_is_noten(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;
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

    rules.noten_penalty = 1;
    rules.noten_penalty_points = 3000;

    set_hand(&state, CJ4_PLAYER_1, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    cj4_state_set_round_result(&state, CJ4_ROUND_END_EXHAUSTIVE_DRAW, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.dealer = CJ4_PLAYER_0;
    state.honba = 2;
    state.winner_mask = 0;

    settled = cj4_do_settle(state, &rules);

    assert(settled.scores[CJ4_PLAYER_0] == 24000);
    assert(settled.scores[CJ4_PLAYER_1] == 28000);
    assert(settled.scores[CJ4_PLAYER_2] == 24000);
    assert(settled.scores[CJ4_PLAYER_3] == 24000);
    assert(settled.next_dealer == CJ4_PLAYER_1);
    assert(settled.honba == 0);
}

static void
test_tonpuu_enters_south_when_target_is_not_reached(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;
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

    rules.game_type = CJ4_GAME_TONPUU;
    rules.target_score = 30000;

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    state.round_wind = CJ4_WIND_EAST;
    cj4_state_set_round_result(&state, CJ4_ROUND_END_EXHAUSTIVE_DRAW, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_3);
    state.dealer = CJ4_PLAYER_3;
    state.winner_mask = 0;
    state.scores[CJ4_PLAYER_0] = 29000;
    state.scores[CJ4_PLAYER_1] = 28000;
    state.scores[CJ4_PLAYER_2] = 22000;
    state.scores[CJ4_PLAYER_3] = 21000;

    settled = cj4_do_settle(state, &rules);

    assert(settled.settlement_should_end == 0);
    assert(settled.next_dealer == CJ4_PLAYER_0);
    assert(settled.next_round_wind == CJ4_WIND_SOUTH);
}

static void
test_hanchan_enters_west_when_target_is_not_reached(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;
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

    rules.game_type = CJ4_GAME_HANCHAN;
    rules.target_score = 30000;

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    state.round_wind = CJ4_WIND_SOUTH;
    cj4_state_set_round_result(&state, CJ4_ROUND_END_EXHAUSTIVE_DRAW, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_3);
    state.dealer = CJ4_PLAYER_3;
    state.winner_mask = 0;
    state.scores[CJ4_PLAYER_0] = 29000;
    state.scores[CJ4_PLAYER_1] = 28000;
    state.scores[CJ4_PLAYER_2] = 22000;
    state.scores[CJ4_PLAYER_3] = 21000;

    settled = cj4_do_settle(state, &rules);

    assert(settled.settlement_should_end == 0);
    assert(settled.next_dealer == CJ4_PLAYER_0);
    assert(settled.next_round_wind == CJ4_WIND_WEST);
}

static void
test_tonpuu_ends_when_target_is_reached(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;
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

    rules.game_type = CJ4_GAME_TONPUU;
    rules.target_score = 30000;

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    state.round_wind = CJ4_WIND_EAST;
    cj4_state_set_round_result(&state, CJ4_ROUND_END_EXHAUSTIVE_DRAW, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_3);
    state.dealer = CJ4_PLAYER_3;
    state.winner_mask = 0;
    state.scores[CJ4_PLAYER_0] = 32000;
    state.scores[CJ4_PLAYER_1] = 26000;
    state.scores[CJ4_PLAYER_2] = 22000;
    state.scores[CJ4_PLAYER_3] = 20000;

    settled = cj4_do_settle(state, &rules);

    assert(settled.settlement_should_end == 1);
    assert(settled.next_round_wind == CJ4_WIND_SOUTH);
}

static void
test_tonpuu_does_not_end_before_east_4_when_non_dealer_reaches_target(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;

    rules.game_type = CJ4_GAME_TONPUU;
    rules.target_score = 30000;

    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    state.round_wind = CJ4_WIND_EAST;
    cj4_state_set_round_result(&state, CJ4_ROUND_END_EXHAUSTIVE_DRAW, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_2);
    state.dealer = CJ4_PLAYER_2;
    state.winner_mask = 0;
    state.scores[CJ4_PLAYER_0] = 32000;
    state.scores[CJ4_PLAYER_1] = 25000;
    state.scores[CJ4_PLAYER_2] = 23000;
    state.scores[CJ4_PLAYER_3] = 20000;

    settled = cj4_do_settle(state, &rules);

    assert(settled.settlement_should_end == 0);
    assert(settled.next_dealer == CJ4_PLAYER_3);
    assert(settled.next_round_wind == CJ4_WIND_EAST);
}

static void
test_tonpuu_does_not_end_before_east_4_when_dealer_renchan_is_top(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;

    rules.game_type = CJ4_GAME_TONPUU;
    rules.target_score = 30000;

    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    state.round_wind = CJ4_WIND_EAST;
    cj4_state_set_round_result(&state, CJ4_ROUND_END_ABORTIVE_DRAW, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_2);
    state.dealer = CJ4_PLAYER_2;
    state.winner_mask = 0;
    state.scores[CJ4_PLAYER_0] = 25000;
    state.scores[CJ4_PLAYER_1] = 24000;
    state.scores[CJ4_PLAYER_2] = 31000;
    state.scores[CJ4_PLAYER_3] = 20000;

    settled = cj4_do_settle(state, &rules);

    assert(settled.settlement_should_end == 0);
    assert(settled.next_dealer == CJ4_PLAYER_2);
    assert(settled.next_round_wind == CJ4_WIND_EAST);
}

static void
test_tonpuu_ends_when_score_matches_target(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;
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

    rules.game_type = CJ4_GAME_TONPUU;
    rules.target_score = 30000;

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    state.round_wind = CJ4_WIND_EAST;
    cj4_state_set_round_result(&state, CJ4_ROUND_END_EXHAUSTIVE_DRAW, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_3);
    state.dealer = CJ4_PLAYER_3;
    state.winner_mask = 0;
    state.scores[CJ4_PLAYER_0] = 30000;
    state.scores[CJ4_PLAYER_1] = 28000;
    state.scores[CJ4_PLAYER_2] = 22000;
    state.scores[CJ4_PLAYER_3] = 20000;

    settled = cj4_do_settle(state, &rules);

    assert(settled.settlement_should_end == 1);
    assert(settled.next_dealer == CJ4_PLAYER_0);
    assert(settled.next_round_wind == CJ4_WIND_SOUTH);
}

static void
test_tonpuu_ends_when_dealer_is_top_and_matches_target(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;
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

    rules.game_type = CJ4_GAME_TONPUU;
    rules.target_score = 30000;

    set_hand(&state, CJ4_PLAYER_3, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    state.round_wind = CJ4_WIND_EAST;
    cj4_state_set_round_result(&state, CJ4_ROUND_END_EXHAUSTIVE_DRAW, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_3);
    state.dealer = CJ4_PLAYER_3;
    state.honba = 2;
    state.winner_mask = 0;
    state.scores[CJ4_PLAYER_0] = 28000;
    state.scores[CJ4_PLAYER_1] = 26000;
    state.scores[CJ4_PLAYER_2] = 15000;
    state.scores[CJ4_PLAYER_3] = 30000;

    settled = cj4_do_settle(state, &rules);

    assert(settled.settlement_should_end == 1);
    assert(settled.next_dealer == CJ4_PLAYER_3);
    assert(settled.next_round_wind == CJ4_WIND_EAST);
    assert(settled.honba == 2);
}

static void
test_tonpuu_continues_when_dealer_renchan_is_not_top(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;
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

    rules.game_type = CJ4_GAME_TONPUU;
    rules.target_score = 30000;

    set_hand(&state, CJ4_PLAYER_3, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    state.round_wind = CJ4_WIND_EAST;
    cj4_state_set_round_result(&state, CJ4_ROUND_END_EXHAUSTIVE_DRAW, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_3);
    state.dealer = CJ4_PLAYER_3;
    state.winner_mask = 0;
    state.scores[CJ4_PLAYER_0] = 32000;
    state.scores[CJ4_PLAYER_1] = 25000;
    state.scores[CJ4_PLAYER_2] = 14000;
    state.scores[CJ4_PLAYER_3] = 29000;

    settled = cj4_do_settle(state, &rules);

    assert(settled.settlement_should_end == 0);
    assert(settled.next_dealer == CJ4_PLAYER_3);
    assert(settled.next_round_wind == CJ4_WIND_EAST);
}

static void
test_tonpuu_ending_on_dealer_tenpai_draw_does_not_increase_honba(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;
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

    rules.game_type = CJ4_GAME_TONPUU;
    rules.target_score = 30000;
    rules.noten_penalty = 1;
    rules.noten_penalty_points = 3000;

    set_hand(&state, CJ4_PLAYER_3, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    state.round_wind = CJ4_WIND_EAST;
    cj4_state_set_round_result(&state, CJ4_ROUND_END_EXHAUSTIVE_DRAW, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_3);
    state.dealer = CJ4_PLAYER_3;
    state.honba = 2;
    state.winner_mask = 0;
    state.scores[CJ4_PLAYER_0] = 24000;
    state.scores[CJ4_PLAYER_1] = 24000;
    state.scores[CJ4_PLAYER_2] = 24000;
    state.scores[CJ4_PLAYER_3] = 29000;

    settled = cj4_do_settle(state, &rules);

    assert(settled.settlement_should_end == 1);
    assert(settled.next_dealer == CJ4_PLAYER_3);
    assert(settled.next_round_wind == CJ4_WIND_EAST);
    assert(settled.honba == 2);
}

static void
test_tenhou_target_score_excludes_awarded_riichi_sticks(
    void)
{
    cj4_rules rules = cj4_rules_tenhou();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong won;
    cj4_mahjong settled;
    cj4_player winner = CJ4_PLAYER_2;
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
        tile(4, 1),
        tile(5, 1),
        tile(14, 0),
        tile(14, 1)};

    set_hand(&state, winner, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    state.round_wind = CJ4_WIND_SOUTH;
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.dealer = CJ4_PLAYER_3;
    state.riichi_sticks = 1;
    state.scores[CJ4_PLAYER_0] = 32700;
    state.scores[CJ4_PLAYER_1] = 25000;
    state.scores[CJ4_PLAYER_2] = 21300;
    state.scores[CJ4_PLAYER_3] = 20000;
    add_discard(&state, CJ4_PLAYER_0, tile(6, 0));

    won = cj4_do_ron_multi(state, &winner, 1, &rules);
    settled = cj4_do_settle(won, &rules);

    assert(settled.scores[winner] == 30000);
    assert(settled.riichi_sticks == 0);
    assert(settled.settlement_should_end == 0);
    assert(settled.next_round_wind == CJ4_WIND_WEST);

    rules.target_score_excludes_riichi_sticks = 0;
    settled = cj4_do_settle(won, &rules);
    assert(settled.settlement_should_end == 1);
}

static void
test_tenhou_dealer_top_check_excludes_awarded_riichi_sticks(
    void)
{
    cj4_rules rules = cj4_rules_tenhou();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong won;
    cj4_mahjong settled;
    cj4_player winner = CJ4_PLAYER_3;
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
        tile(4, 1),
        tile(5, 1),
        tile(14, 0),
        tile(14, 1)};

    set_hand(&state, winner, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    state.round_wind = CJ4_WIND_SOUTH;
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.dealer = winner;
    state.riichi_sticks = 1;
    state.scores[CJ4_PLAYER_0] = 36600;
    state.scores[CJ4_PLAYER_1] = 30000;
    state.scores[CJ4_PLAYER_2] = 14500;
    state.scores[CJ4_PLAYER_3] = 17900;
    add_discard(&state, CJ4_PLAYER_0, tile(6, 0));

    won = cj4_do_ron_multi(state, &winner, 1, &rules);
    settled = cj4_do_settle(won, &rules);

    assert(settled.scores[winner] == 30500);
    assert(settled.riichi_sticks == 0);
    assert(settled.settlement_should_end == 0);
    assert(settled.next_dealer == winner);

    rules.target_score_excludes_riichi_sticks = 0;
    settled = cj4_do_settle(won, &rules);
    assert(settled.settlement_should_end == 1);
}

void
cj4_test_round_progression(
    void)
{
    test_exhaustive_draw_uses_shape_tenpai();
    test_exhaustive_draw_uses_live_wall_not_discard_count();
    test_haitei_uses_kan_adjusted_live_wall_end();
    test_exhaustive_draw_resets_honba_when_dealer_is_noten();
    test_tonpuu_enters_south_when_target_is_not_reached();
    test_hanchan_enters_west_when_target_is_not_reached();
    test_tonpuu_ends_when_target_is_reached();
    test_tonpuu_does_not_end_before_east_4_when_non_dealer_reaches_target();
    test_tonpuu_does_not_end_before_east_4_when_dealer_renchan_is_top();
    test_tonpuu_ends_when_score_matches_target();
    test_tonpuu_ends_when_dealer_is_top_and_matches_target();
    test_tonpuu_continues_when_dealer_renchan_is_not_top();
    test_tonpuu_ending_on_dealer_tenpai_draw_does_not_increase_honba();
    test_tenhou_target_score_excludes_awarded_riichi_sticks();
    test_tenhou_dealer_top_check_excludes_awarded_riichi_sticks();
}
