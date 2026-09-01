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

static cj4_win_result
collect_single_ron_result(
    const cj4_tile_id *hand,
    uint8_t hand_count,
    cj4_tile_id winning_tile)
{
    cj4_rules rules = cj4_rules_tenhou();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong won;
    cj4_win_result results[CJ4_PLAYER_COUNT];
    uint8_t result_count = 0;
    cj4_player winners[] = {CJ4_PLAYER_2};

    set_hand(&state, CJ4_PLAYER_2, hand, hand_count);
    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    add_discard(&state, CJ4_PLAYER_0, winning_tile);

    won = cj4_do_ron_multi(state, winners, 1, &rules);
    assert(cj4_collect_winning_results(
        &won,
        &rules,
        results,
        CJ4_PLAYER_COUNT,
        &result_count));
    assert(result_count == 1);
    assert(results[0].player == CJ4_PLAYER_2);
    return results[0];
}

static void
test_best_score_prefers_ryanpeikou_over_chiitoi(
    void)
{
    const cj4_tile_id hand[] = {
        tile(0, 0), tile(0, 1),
        tile(1, 0), tile(1, 1),
        tile(2, 0), tile(2, 1),
        tile(9, 0), tile(9, 1),
        tile(10, 0), tile(10, 1),
        tile(11, 0), tile(11, 1),
        tile(22, 2)};
    cj4_win_result result = collect_single_ron_result(
        hand,
        (uint8_t)(sizeof(hand) / sizeof(hand[0])),
        tile(22, 3));

    assert(result.han == 3);
    assert(result.fu == 40);
    assert(result.ron_points == 5200);
    assert(result.yaku_count == 1);
    assert(contains_win_yaku(&result, CJ4_WIN_YAKU_RYANPEIKOU));
    assert(!contains_win_yaku(&result, CJ4_WIN_YAKU_CHIITOI));
}

static void
test_best_tsumo_score_prefers_ryanpeikou_over_chiitoi(
    void)
{
    cj4_rules rules = cj4_rules_tenhou();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong won;
    cj4_win_result results[CJ4_PLAYER_COUNT];
    uint8_t result_count = 0;
    cj4_tile_id draw = tile(22, 3);
    const cj4_tile_id hand[] = {
        tile(0, 0), tile(0, 1),
        tile(1, 0), tile(1, 1),
        tile(2, 0), tile(2, 1),
        tile(9, 0), tile(9, 1),
        tile(10, 0), tile(10, 1),
        tile(11, 0), tile(11, 1),
        tile(22, 2), draw};

    assert(rules.kiriage_mangan == 0);
    set_hand(
        &state,
        CJ4_PLAYER_2,
        hand,
        (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&state, CJ4_PLAYER_2);
    state.draw_tile = draw;

    won = cj4_do_tsumo(state);
    assert(cj4_collect_winning_results(
        &won,
        &rules,
        results,
        CJ4_PLAYER_COUNT,
        &result_count));
    assert(result_count == 1);
    assert(results[0].han == 4);
    assert(results[0].fu == 30);
    assert(results[0].ron_points == 0);
    assert(results[0].tsumo_dealer_payment == 3900);
    assert(results[0].tsumo_non_dealer_payment == 2000);
    assert(results[0].aka_dora_count == 0);
    assert(results[0].yaku_count == 2);
    assert(contains_win_yaku(&results[0], CJ4_WIN_YAKU_MENZEN_TSUMO));
    assert(contains_win_yaku(&results[0], CJ4_WIN_YAKU_RYANPEIKOU));
    assert(!contains_win_yaku(&results[0], CJ4_WIN_YAKU_CHIITOI));

    rules = cj4_rules_default();
    assert(rules.kiriage_mangan == 1);
    result_count = 0;
    assert(cj4_collect_winning_results(
        &won,
        &rules,
        results,
        CJ4_PLAYER_COUNT,
        &result_count));
    assert(result_count == 1);
    assert(results[0].han == 4);
    assert(results[0].fu == 30);
    assert(results[0].tsumo_dealer_payment == 4000);
    assert(results[0].tsumo_non_dealer_payment == 2000);
    assert(results[0].aka_dora_count == 0);
    assert(contains_win_yaku(&results[0], CJ4_WIN_YAKU_RYANPEIKOU));
    assert(!contains_win_yaku(&results[0], CJ4_WIN_YAKU_CHIITOI));
}

static void
test_pure_chiitoi_remains_chiitoi(
    void)
{
    const cj4_tile_id hand[] = {
        tile(0, 0), tile(0, 1),
        tile(4, 1), tile(4, 2),
        tile(8, 0), tile(8, 1),
        tile(9, 0), tile(9, 1),
        tile(13, 1), tile(13, 2),
        tile(17, 0), tile(17, 1),
        tile(27, 0)};
    cj4_win_result result = collect_single_ron_result(
        hand,
        (uint8_t)(sizeof(hand) / sizeof(hand[0])),
        tile(27, 1));

    assert(result.han == 2);
    assert(result.fu == 25);
    assert(result.ron_points == 1600);
    assert(result.yaku_count == 1);
    assert(contains_win_yaku(&result, CJ4_WIN_YAKU_CHIITOI));
    assert(!contains_win_yaku(&result, CJ4_WIN_YAKU_RYANPEIKOU));
}

static void
test_non_chiitoi_ryanpeikou_remains_ryanpeikou(
    void)
{
    const cj4_tile_id hand[] = {
        tile(0, 0), tile(0, 1),
        tile(1, 0), tile(1, 1), tile(1, 2), tile(1, 3),
        tile(2, 0), tile(2, 1), tile(2, 2), tile(2, 3),
        tile(3, 0), tile(3, 1),
        tile(13, 2)};
    cj4_win_result result = collect_single_ron_result(
        hand,
        (uint8_t)(sizeof(hand) / sizeof(hand[0])),
        tile(13, 3));

    assert(result.han == 3);
    assert(result.fu == 40);
    assert(result.ron_points == 5200);
    assert(result.yaku_count == 1);
    assert(contains_win_yaku(&result, CJ4_WIN_YAKU_RYANPEIKOU));
    assert(!contains_win_yaku(&result, CJ4_WIN_YAKU_CHIITOI));
}

static void
test_junchan_excludes_chanta(
    void)
{
    const cj4_tile_id hand[] = {
        tile(0, 0), tile(1, 0), tile(2, 0),
        tile(6, 0), tile(7, 0), tile(8, 0),
        tile(9, 0), tile(10, 0), tile(11, 0),
        tile(15, 0), tile(16, 0), tile(17, 0),
        tile(18, 0)};
    cj4_win_result result = collect_single_ron_result(
        hand,
        (uint8_t)(sizeof(hand) / sizeof(hand[0])),
        tile(18, 1));

    assert(result.han == 3);
    assert(result.fu == 40);
    assert(result.ron_points == 5200);
    assert(result.yaku_count == 1);
    assert(contains_win_yaku(&result, CJ4_WIN_YAKU_JUNCHAN));
    assert(!contains_win_yaku(&result, CJ4_WIN_YAKU_CHANTA));
}

static void
test_chanta_remains_chanta(
    void)
{
    const cj4_tile_id hand[] = {
        tile(0, 0), tile(1, 0), tile(2, 0),
        tile(6, 0), tile(7, 0), tile(8, 0),
        tile(9, 0), tile(10, 0), tile(11, 0),
        tile(15, 0), tile(16, 0), tile(17, 0),
        tile(28, 0)};
    cj4_win_result result = collect_single_ron_result(
        hand,
        (uint8_t)(sizeof(hand) / sizeof(hand[0])),
        tile(28, 1));

    assert(result.han == 2);
    assert(result.fu == 40);
    assert(result.ron_points == 2600);
    assert(result.yaku_count == 1);
    assert(contains_win_yaku(&result, CJ4_WIN_YAKU_CHANTA));
    assert(!contains_win_yaku(&result, CJ4_WIN_YAKU_JUNCHAN));
}

static void
test_collect_winning_results_returns_tsumo_details(
    void)
{
    cj4_rules rules = {0};
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
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.dealer = CJ4_PLAYER_1;
    state.draw_tile = draw;
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_riichi(&state, CJ4_PLAYER_0, 1);
    cj4_state_set_double_riichi(&state, CJ4_PLAYER_0, 1);

    won = cj4_do_tsumo(state);

    assert(cj4_collect_winning_results(
        &won,
        &rules,
        results,
        CJ4_PLAYER_COUNT,
        &result_count));
    assert(result_count == 1);
    assert(results[0].player == CJ4_PLAYER_0);
    assert(results[0].han == 6);
    assert(results[0].fu == 20);
    assert(results[0].yakuman_count == 0);
    assert(results[0].ron_points == 0);
    assert(results[0].tsumo_dealer_payment == 6000);
    assert(results[0].tsumo_non_dealer_payment == 3000);
    assert(results[0].dora_count == 0);
    assert(results[0].ura_dora_count == 0);
    assert(results[0].aka_dora_count == 0);
    assert(contains_win_yaku(&results[0], CJ4_WIN_YAKU_DOUBLE_RIICHI));
    assert(contains_win_yaku(&results[0], CJ4_WIN_YAKU_MENZEN_TSUMO));
    assert(contains_win_yaku(&results[0], CJ4_WIN_YAKU_PINFU));
    assert(contains_win_yaku(&results[0], CJ4_WIN_YAKU_SANSHOKU_DOUJUN));
    assert(!contains_win_yaku(&results[0], CJ4_WIN_YAKU_RIICHI));
}

static void
test_collect_winning_results_exposes_dora_indicators(
    void)
{
    cj4_rules rules = {0};
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
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.dealer = CJ4_PLAYER_1;
    state.draw_tile = draw;
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_riichi(&state, CJ4_PLAYER_0, 1);

    won = cj4_do_tsumo(state);
    won.dora_count = 2;
    set_test_wall(&won, (uint8_t)(130), tile(27, 0));
    set_test_wall(&won, (uint8_t)(128), tile(31, 0));
    set_test_wall(&won, (uint8_t)(131), tile(28, 0));
    set_test_wall(&won, (uint8_t)(129), tile(32, 0));

    assert(cj4_collect_winning_results(
        &won,
        &rules,
        results,
        CJ4_PLAYER_COUNT,
        &result_count));
    assert(result_count == 1);
    assert(results[0].dora_indicators_count == 2);
    assert(results[0].dora_indicators[0] == tile(27, 0));
    assert(results[0].dora_indicators[1] == tile(31, 0));
    assert(results[0].ura_dora_indicators_count == 2);
    assert(results[0].ura_dora_indicators[0] == tile(28, 0));
    assert(results[0].ura_dora_indicators[1] == tile(32, 0));

    won.locations[tile(27, 0)].wall = CJ4_LOCATION_NONE;
    result_count = 0;
    assert(cj4_collect_winning_results(
        &won,
        &rules,
        results,
        CJ4_PLAYER_COUNT,
        &result_count));
    assert(result_count == 1);
    assert(results[0].dora_indicators_count == 1);
    assert(results[0].dora_indicators[0] == tile(31, 0));
}

static void
test_collect_winning_results_returns_ron_details(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong won;
    cj4_win_result results[CJ4_PLAYER_COUNT];
    uint8_t result_count = 0;
    cj4_player winners[] = {CJ4_PLAYER_2};
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

    rules.kuitan = 1;

    set_hand(&state, CJ4_PLAYER_2, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    add_discard(&state, CJ4_PLAYER_0, tile(6, 0));

    won = cj4_do_ron_multi(state, winners, 1, &rules);

    assert(cj4_collect_winning_results(
        &won,
        &rules,
        results,
        CJ4_PLAYER_COUNT,
        &result_count));
    assert(result_count == 1);
    assert(results[0].player == CJ4_PLAYER_2);
    assert(results[0].han == 4);
    assert(results[0].fu == 30);
    assert(results[0].yakuman_count == 0);
    assert(results[0].ron_points == 7700);
    assert(results[0].tsumo_dealer_payment == 0);
    assert(results[0].tsumo_non_dealer_payment == 0);
    assert(results[0].dora_count == 0);
    assert(results[0].ura_dora_count == 0);
    assert(results[0].aka_dora_count == 0);
    assert(contains_win_yaku(&results[0], CJ4_WIN_YAKU_TANYAO));
    assert(contains_win_yaku(&results[0], CJ4_WIN_YAKU_PINFU));
    assert(contains_win_yaku(&results[0], CJ4_WIN_YAKU_SANSHOKU_DOUJUN));
}

static void
test_collect_winning_results_hides_ura_dora_without_riichi(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong won;
    cj4_win_result results[CJ4_PLAYER_COUNT];
    uint8_t result_count = 0;
    cj4_player winners[] = {CJ4_PLAYER_2};
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

    rules.kuitan = 1;

    set_hand(&state, CJ4_PLAYER_2, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    add_discard(&state, CJ4_PLAYER_0, tile(6, 0));

    won = cj4_do_ron_multi(state, winners, 1, &rules);
    won.dora_count = 2;
    set_test_wall(&won, (uint8_t)(130), tile(27, 0));
    set_test_wall(&won, (uint8_t)(128), tile(31, 0));
    set_test_wall(&won, (uint8_t)(131), tile(28, 0));
    set_test_wall(&won, (uint8_t)(129), tile(32, 0));

    assert(cj4_collect_winning_results(
        &won,
        &rules,
        results,
        CJ4_PLAYER_COUNT,
        &result_count));
    assert(result_count == 1);
    assert(results[0].dora_indicators_count == 2);
    assert(results[0].dora_indicators[0] == tile(27, 0));
    assert(results[0].dora_indicators[1] == tile(31, 0));
    assert(results[0].ura_dora_indicators_count == 0);
}

static void
test_collect_winning_results_returns_multi_ron_details(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong won;
    cj4_mahjong settled;
    cj4_win_result results[CJ4_PLAYER_COUNT];
    uint8_t result_count = 0;
    cj4_player winners[] = {CJ4_PLAYER_3, CJ4_PLAYER_2};
    const cj4_tile_id ron_hand[] = {
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
    const cj4_tile_id ron_hand_alt[] = {
        tile(1, 1),
        tile(2, 1),
        tile(3, 1),
        tile(10, 1),
        tile(11, 1),
        tile(12, 1),
        tile(19, 1),
        tile(20, 1),
        tile(21, 1),
        tile(4, 2),
        tile(5, 2),
        tile(14, 2),
        tile(14, 3)};

    rules.kuitan = 1;

    set_hand(&state, CJ4_PLAYER_2, ron_hand, (uint8_t)(sizeof(ron_hand) / sizeof(ron_hand[0])));
    set_hand(&state, CJ4_PLAYER_3, ron_hand_alt, (uint8_t)(sizeof(ron_hand_alt) / sizeof(ron_hand_alt[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    add_discard(&state, CJ4_PLAYER_0, tile(6, 0));

    won = cj4_do_ron_multi(state, winners, 2, &rules);

    assert(cj4_collect_winning_results(
        &won,
        &rules,
        results,
        CJ4_PLAYER_COUNT,
        &result_count));
    assert(result_count == 2);
    assert(results[0].player == CJ4_PLAYER_2);
    assert(results[1].player == CJ4_PLAYER_3);
    assert(results[0].han == 4);
    assert(results[1].han == 4);
    assert(results[0].fu == 30);
    assert(results[1].fu == 30);
    assert(contains_win_yaku(&results[0], CJ4_WIN_YAKU_TANYAO));
    assert(contains_win_yaku(&results[1], CJ4_WIN_YAKU_TANYAO));

    rules = cj4_rules_tenhou();
    state.honba = 2;
    won = cj4_do_ron_multi(state, winners, 2, &rules);
    settled = cj4_do_settle(won, &rules);

    assert(settled.scores[CJ4_PLAYER_0] == 9000);
    assert(settled.scores[CJ4_PLAYER_1] == 25000);
    assert(settled.scores[CJ4_PLAYER_2] == 33300);
    assert(settled.scores[CJ4_PLAYER_3] == 32700);
}

static void
test_release_settle_skips_uncalculable_win_scores(
    void)
{
#ifdef NDEBUG
    cj4_rules rules = {0};
    cj4_mahjong ron_state = make_empty_state();
    cj4_mahjong tsumo_state = make_empty_state();
    cj4_mahjong settled;

    rules.target_score = 30000;

    cj4_state_set_phase(&ron_state, CJ4_PHASE_ROUND_END);
    cj4_state_set_round_result(&ron_state, CJ4_ROUND_END_RON, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&ron_state, CJ4_PLAYER_0);
    ron_state.dealer = CJ4_PLAYER_0;
    ron_state.winner_mask = (uint8_t)(1u << CJ4_PLAYER_2);
    ron_state.winning_tile = CJ4_TILE_ID_INVALID;

    settled = cj4_do_settle(ron_state, &rules);

    assert(cj4_state_phase(&settled) == CJ4_PHASE_SETTLE);
    assert(settled.scores[CJ4_PLAYER_0] == 25000);
    assert(settled.scores[CJ4_PLAYER_1] == 25000);
    assert(settled.scores[CJ4_PLAYER_2] == 25000);
    assert(settled.scores[CJ4_PLAYER_3] == 25000);

    cj4_state_set_phase(&tsumo_state, CJ4_PHASE_ROUND_END);
    cj4_state_set_round_result(&tsumo_state, CJ4_ROUND_END_TSUMO, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&tsumo_state, CJ4_PLAYER_1);
    tsumo_state.dealer = CJ4_PLAYER_0;
    tsumo_state.winner_mask = (uint8_t)(1u << CJ4_PLAYER_1);
    tsumo_state.draw_tile = CJ4_TILE_ID_INVALID;
    tsumo_state.winning_tile = CJ4_TILE_ID_INVALID;

    settled = cj4_do_settle(tsumo_state, &rules);

    assert(cj4_state_phase(&settled) == CJ4_PHASE_SETTLE);
    assert(settled.scores[CJ4_PLAYER_0] == 25000);
    assert(settled.scores[CJ4_PLAYER_1] == 25000);
    assert(settled.scores[CJ4_PLAYER_2] == 25000);
    assert(settled.scores[CJ4_PLAYER_3] == 25000);
#endif
}

static void
test_max_ron_players_uses_head_bump_order(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong next;
    cj4_player claimers[] = {CJ4_PLAYER_3, CJ4_PLAYER_1, CJ4_PLAYER_2};

    rules.max_ron_players = 2;

    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.riichi_sticks = 2;
    add_discard(&state, CJ4_PLAYER_0, tile(0, 0));

    next = cj4_do_ron_multi(state, claimers, 3, &rules);

    assert(cj4_state_winner_count(&next) == 2);
    assert(cj4_get_winner(&next, 0) == CJ4_PLAYER_1);
    assert(cj4_get_winner(&next, 1) == CJ4_PLAYER_2);
}

static void
test_nagashi_mangan_settles_as_mangan_tsumo(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;
    cj4_win_result results[CJ4_PLAYER_COUNT];
    uint8_t result_count = 0;

    rules.target_score = 30000;
    rules.nagashi_mangan = 1;

    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    cj4_state_set_round_result(&state, CJ4_ROUND_END_EXHAUSTIVE_DRAW, CJ4_ABORTIVE_DRAW_NONE);
    state.dealer = CJ4_PLAYER_0;
    add_discard(&state, CJ4_PLAYER_1, tile(8, 0));

    assert(cj4_collect_winning_results(
        &state,
        &rules,
        results,
        CJ4_PLAYER_COUNT,
        &result_count));
    assert(result_count == 1);
    assert(results[0].player == CJ4_PLAYER_1);
    assert(contains_win_yaku(&results[0], CJ4_WIN_YAKU_NAGASHI_MANGAN));

    settled = cj4_do_settle(state, &rules);

    assert(settled.scores[CJ4_PLAYER_0] == 21000);
    assert(settled.scores[CJ4_PLAYER_1] == 33000);
    assert(settled.scores[CJ4_PLAYER_2] == 23000);
    assert(settled.scores[CJ4_PLAYER_3] == 23000);
    assert(settled.next_dealer == CJ4_PLAYER_1);
}

static void
test_tenhou_nagashi_mangan_uses_dealer_tenpai_for_renchan(
    void)
{
    cj4_rules rules = cj4_rules_tenhou();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;
    const cj4_tile_id dealer_hand[] = {
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

    set_hand(
        &state,
        CJ4_PLAYER_0,
        dealer_hand,
        (uint8_t)(sizeof(dealer_hand) / sizeof(dealer_hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    cj4_state_set_round_result(&state, CJ4_ROUND_END_EXHAUSTIVE_DRAW, CJ4_ABORTIVE_DRAW_NONE);
    state.dealer = CJ4_PLAYER_0;
    add_discard(&state, CJ4_PLAYER_1, tile(8, 0));

    settled = cj4_do_settle(state, &rules);

    assert(settled.next_dealer == CJ4_PLAYER_0);
    assert(settled.honba == 1);
}

static void
test_pao_splits_ron_payment_with_responsible_player(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;
    const cj4_tile_id haku[] = {tile(31, 0), tile(31, 1), tile(31, 2)};
    const cj4_tile_id hatsu[] = {tile(32, 0), tile(32, 1), tile(32, 2)};
    const cj4_tile_id chun[] = {tile(33, 0), tile(33, 1), tile(33, 2)};
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(13, 0),
        tile(13, 1)};

    rules.pao = 1;
    rules.target_score = 30000;

    set_hand(&state, CJ4_PLAYER_2, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    set_test_meld(&state, CJ4_PLAYER_2, 0, &(cj4_meld){.tiles = {haku[0], haku[1], haku[2]}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_0, .called_index = 0});
    set_test_meld(&state, CJ4_PLAYER_2, 1, &(cj4_meld){.tiles = {hatsu[0], hatsu[1], hatsu[2]}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_1, .called_index = 0});
    set_test_meld(&state, CJ4_PLAYER_2, 2, &(cj4_meld){.tiles = {chun[0], chun[1], chun[2]}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_1, .called_index = 0});

    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    cj4_state_set_round_result(&state, CJ4_ROUND_END_RON, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.dealer = CJ4_PLAYER_0;
    state.winner_mask = (uint8_t)(1u << CJ4_PLAYER_2);
    state.winning_tile = tile(2, 0);
    cj4_state_set_pao(
        &state,
        CJ4_PLAYER_2,
        CJ4_PLAYER_1,
        CJ4_PAO_DAISANGEN);
    add_discard(&state, CJ4_PLAYER_0, state.winning_tile);
    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);

    settled = cj4_do_settle(state, &rules);

    assert(settled.scores[CJ4_PLAYER_0] == 9000);
    assert(settled.scores[CJ4_PLAYER_1] == 9000);
    assert(settled.scores[CJ4_PLAYER_2] == 57000);
}

static void
test_pao_liability_only_splits_compound_yakuman_tsumo(
    void)
{
    cj4_rules rules = cj4_rules_default();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong won;
    cj4_mahjong settled;
    const cj4_tile_id haku[] = {tile(31, 0), tile(31, 1), tile(31, 2)};
    const cj4_tile_id hatsu[] = {tile(32, 0), tile(32, 1), tile(32, 2)};
    const cj4_tile_id chun[] = {tile(33, 0), tile(33, 1), tile(33, 2)};
    cj4_tile_id draw = tile(27, 2);
    const cj4_tile_id hand[] = {
        tile(27, 0),
        tile(27, 1),
        tile(28, 0),
        tile(28, 1),
        draw};

    rules.pao = 1;
    rules.pao_liability_only = 1;

    set_hand(&state, CJ4_PLAYER_2, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    set_test_meld(&state, CJ4_PLAYER_2, 0, &(cj4_meld){.tiles = {haku[0], haku[1], haku[2]}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_0, .called_index = 0});
    set_test_meld(&state, CJ4_PLAYER_2, 1, &(cj4_meld){.tiles = {hatsu[0], hatsu[1], hatsu[2]}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_1, .called_index = 0});
    set_test_meld(&state, CJ4_PLAYER_2, 2, &(cj4_meld){.tiles = {chun[0], chun[1], chun[2]}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_1, .called_index = 0});
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&state, CJ4_PLAYER_2);
    state.dealer = CJ4_PLAYER_0;
    state.draw_tile = draw;
    cj4_state_set_pao(
        &state,
        CJ4_PLAYER_2,
        CJ4_PLAYER_1,
        CJ4_PAO_DAISANGEN);

    won = cj4_do_tsumo(state);
    settled = cj4_do_settle(won, &rules);

    assert(settled.scores[CJ4_PLAYER_0] == 9000);
    assert(settled.scores[CJ4_PLAYER_1] == -15000);
    assert(settled.scores[CJ4_PLAYER_2] == 89000);
    assert(settled.scores[CJ4_PLAYER_3] == 17000);
}

static void
test_daisuushii_double_pao_liability_uses_two_yakuman(
    void)
{
    cj4_rules rules = cj4_rules_default();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong won;
    cj4_mahjong settled;
    cj4_player winner = CJ4_PLAYER_2;
    cj4_tile_id draw = tile(4, 1);
    const cj4_tile_id pair[] = {tile(4, 0), draw};

    rules.pao = 1;
    rules.pao_liability_only = 1;
    rules.daisuushii_double = 1;

    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&state, winner);
    state.dealer = CJ4_PLAYER_0;
    state.draw_tile = draw;
    set_hand(&state, winner, pair, 2);
    set_test_meld(&state, winner, 0, &(cj4_meld){.tiles = {tile(27, 0), tile(27, 1), tile(27, 2)}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_0, .called_index = 0});
    set_test_meld(&state, winner, 1, &(cj4_meld){.tiles = {tile(28, 0), tile(28, 1), tile(28, 2)}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_1, .called_index = 0});
    set_test_meld(&state, winner, 2, &(cj4_meld){.tiles = {tile(29, 0), tile(29, 1), tile(29, 2)}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_1, .called_index = 0});
    set_test_meld(&state, winner, 3, &(cj4_meld){.tiles = {tile(30, 0), tile(30, 1), tile(30, 2)}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_1, .called_index = 0});
    cj4_state_set_pao(
        &state,
        winner,
        CJ4_PLAYER_1,
        CJ4_PAO_DAISUUSHII);

    won = cj4_do_tsumo(state);
    settled = cj4_do_settle(won, &rules);

    assert(settled.scores[CJ4_PLAYER_0] == 25000);
    assert(settled.scores[CJ4_PLAYER_1] == -39000);
    assert(settled.scores[CJ4_PLAYER_2] == 89000);
    assert(settled.scores[CJ4_PLAYER_3] == 25000);

    rules.daisuushii_double = 0;
    settled = cj4_do_settle(won, &rules);
    assert(settled.scores[CJ4_PLAYER_1] == -7000);
    assert(settled.scores[CJ4_PLAYER_2] == 57000);
}

static void
test_pao_type_flags_disable_suukantsu_liability(
    void)
{
    cj4_rules rules = cj4_rules_tenhou();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;
    cj4_player winner = CJ4_PLAYER_2;

    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    cj4_state_set_round_result(&state, CJ4_ROUND_END_RON, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.dealer = CJ4_PLAYER_0;
    state.winner_mask = (uint8_t)(1u << winner);
    state.winning_tile = tile(4, 0);
    cj4_state_set_pao(
        &state,
        winner,
        CJ4_PLAYER_1,
        CJ4_PAO_SUUKANTSU);
    add_discard(&state, CJ4_PLAYER_0, state.winning_tile);
    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    set_hand(&state, winner, (const cj4_tile_id[]){tile(4, 1)}, 1);
    for (uint8_t i = 0; i < 4; ++i)
    {
        set_test_meld(&state, winner, i, &(cj4_meld){.tiles = {tile((cj4_tile_type)i, 0), tile((cj4_tile_type)i, 1), tile((cj4_tile_type)i, 2), tile((cj4_tile_type)i, 3)}, .size = 4, .type = CJ4_MELD_MINKAN, .from_player = CJ4_PLAYER_1, .called_index = 0});
    }

    settled = cj4_do_settle(state, &rules);
    assert(settled.scores[CJ4_PLAYER_1] == 25000);
}

void
cj4_test_scoring_settlement(
    void)
{
    test_best_score_prefers_ryanpeikou_over_chiitoi();
    test_best_tsumo_score_prefers_ryanpeikou_over_chiitoi();
    test_pure_chiitoi_remains_chiitoi();
    test_non_chiitoi_ryanpeikou_remains_ryanpeikou();
    test_junchan_excludes_chanta();
    test_chanta_remains_chanta();
    test_collect_winning_results_returns_tsumo_details();
    test_collect_winning_results_exposes_dora_indicators();
    test_collect_winning_results_returns_ron_details();
    test_collect_winning_results_hides_ura_dora_without_riichi();
    test_collect_winning_results_returns_multi_ron_details();
    test_release_settle_skips_uncalculable_win_scores();
    test_max_ron_players_uses_head_bump_order();
    test_nagashi_mangan_settles_as_mangan_tsumo();
    test_tenhou_nagashi_mangan_uses_dealer_tenpai_for_renchan();
    test_pao_splits_ron_payment_with_responsible_player();
    test_pao_liability_only_splits_compound_yakuman_tsumo();
    test_daisuushii_double_pao_liability_uses_two_yakuman();
    test_pao_type_flags_disable_suukantsu_liability();
}
