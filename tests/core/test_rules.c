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
test_rules_default_and_validate(
    void)
{
    cj4_rules rules = cj4_rules_default();

    assert(cj4_rules_validate(&rules));
    assert(rules.initial_score == 25000);
    assert(rules.target_score == 30000);
    assert(rules.max_ron_players == 3);
    assert(rules.kuikae_forbidden == 1);
    assert(rules.kokushi_ron_on_ankan == 1);
    assert(rules.kazoe_yakuman == 1);
    assert(rules.kiriage_mangan == 1);
    assert(rules.multi_ron_honba_first_only == 0);
    assert(rules.nagashi_dealer_tenpai_renchan == 0);
    assert(rules.target_score_excludes_riichi_sticks == 0);
    assert(rules.kan_dora_timing == CJ4_KAN_DORA_EARLY);
    assert(rules.four_kans_abort_timing == CJ4_FOUR_KANS_ABORT_AFTER_DISCARD);

    rules.target_score_excludes_riichi_sticks = 2;
    assert(!cj4_rules_validate(&rules));
    rules.target_score_excludes_riichi_sticks = 0;

    rules.max_ron_players = 0;
    assert(!cj4_rules_validate(&rules));
    rules.max_ron_players = 3;

    rules.kan_dora_timing = (cj4_kan_dora_timing)2;
    assert(!cj4_rules_validate(&rules));

    rules = cj4_rules_default();
    rules.four_kans_abort_timing = (cj4_four_kans_abort_timing)2;
    assert(!cj4_rules_validate(&rules));

    rules = cj4_rules_mjsoul();
    assert(cj4_rules_validate(&rules));
    assert(rules.kan_dora_timing == CJ4_KAN_DORA_LATE);
}

static void
test_tenhou_preset_fields(
    void)
{
    cj4_rules rules = cj4_rules_tenhou();

    assert(cj4_rules_validate(&rules));
    assert(rules.version == CJ4_RULES_VERSION);
    assert(rules.kan_dora_timing == CJ4_KAN_DORA_LATE);
    assert(rules.triple_ron_abortive_draw == 1);
    assert(rules.pao_liability_only == 0);
    assert(rules.kiriage_mangan == 0);
    assert(rules.kokushi_ron_on_ankan == 0);
    assert(rules.kokushi_13_wait_double == 0);
    assert(rules.suuankou_tanki_double == 0);
    assert(rules.junsei_chuuren_double == 0);
    assert(rules.daisuushii_double == 0);
    assert(rules.pao == 1);
    assert(rules.pao_daisangen == 1);
    assert(rules.pao_daisuushii == 1);
    assert(rules.pao_suukantsu == 0);
    assert(rules.multi_ron_honba_first_only == 1);
    assert(rules.nagashi_dealer_tenpai_renchan == 1);
    assert(rules.target_score_excludes_riichi_sticks == 1);
}

static void
test_double_riichi_is_always_enabled(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_hand_score score;
    cj4_hand_score base_score;
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
    cj4_state_set_first_turn(&state, 0);
    cj4_state_set_draw_turn(&state, CJ4_PLAYER_0, 2);
    cj4_state_set_riichi(&state, CJ4_PLAYER_0, 1);

    assert(cj4_calculate_hand_score(&state, CJ4_PLAYER_0, &rules, &base_score));

    cj4_state_set_double_riichi(&state, CJ4_PLAYER_0, 1);
    assert(cj4_calculate_hand_score(&state, CJ4_PLAYER_0, &rules, &score));
    assert(score.han > base_score.han);
}

static void
test_score_rules_control_kazoe_and_kiriage(
    void)
{
    cj4_rules rules = cj4_rules_default();
    cj4_mahjong kazoe_state = make_empty_state();
    cj4_hand_score score;
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
    cj4_mahjong ron_state = make_empty_state();
    cj4_mahjong won;
    cj4_player winner = CJ4_PLAYER_0;
    const cj4_tile_id ron_hand[] = {
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

    set_hand(&kazoe_state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_current_player(&kazoe_state, CJ4_PLAYER_0);
    kazoe_state.dealer = CJ4_PLAYER_1;
    kazoe_state.draw_tile = draw;
    cj4_state_set_phase(&kazoe_state, CJ4_PHASE_DRAW);
    cj4_state_set_riichi(&kazoe_state, CJ4_PLAYER_0, 1);
    cj4_state_set_double_riichi(&kazoe_state, CJ4_PLAYER_0, 1);
    kazoe_state.dora_count = 5;
    set_test_wall(&kazoe_state, (uint8_t)(130), tile(14, 0));
    set_test_wall(&kazoe_state, (uint8_t)(128), tile(14, 1));
    set_test_wall(&kazoe_state, (uint8_t)(126), tile(14, 2));
    set_test_wall(&kazoe_state, (uint8_t)(124), tile(14, 3));
    set_test_wall(&kazoe_state, (uint8_t)(122), tile(4, 0));
    set_test_wall(&kazoe_state, (uint8_t)(131), tile(2, 0));
    set_test_wall(&kazoe_state, (uint8_t)(129), tile(2, 1));
    set_test_wall(&kazoe_state, (uint8_t)(127), tile(2, 2));
    set_test_wall(&kazoe_state, (uint8_t)(125), tile(2, 3));
    set_test_wall(&kazoe_state, (uint8_t)(123), tile(3, 1));

    assert(cj4_calculate_hand_score(&kazoe_state, CJ4_PLAYER_0, &rules, &score));
    assert(score.yakuman_count == 1);

    rules.kazoe_yakuman = 0;
    assert(cj4_calculate_hand_score(&kazoe_state, CJ4_PLAYER_0, &rules, &score));
    assert(score.yakuman_count == 0);
    assert(score.han >= 13);

    rules = cj4_rules_default();
    rules.kiriage_mangan = 0;
    memset(rules.aka_tiles, 0, sizeof(rules.aka_tiles));
    set_hand(&ron_state, CJ4_PLAYER_0, ron_hand, (uint8_t)(sizeof(ron_hand) / sizeof(ron_hand[0])));
    cj4_state_set_phase(&ron_state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&ron_state, CJ4_PLAYER_1);
    ron_state.dealer = CJ4_PLAYER_1;
    cj4_state_set_riichi(&ron_state, CJ4_PLAYER_0, 1);
    add_discard(&ron_state, CJ4_PLAYER_1, draw);
    won = cj4_do_ron_multi(ron_state, &winner, 1, &rules);

    assert(cj4_calculate_hand_score(&won, CJ4_PLAYER_0, &rules, &score));
    assert(score.fu == 30);
    assert(score.han == 4);
    assert(score.ron_points == 7700);

    rules.kiriage_mangan = 1;
    assert(cj4_calculate_hand_score(&won, CJ4_PLAYER_0, &rules, &score));
    assert(score.ron_points == 8000);
}

static void
test_zero_initialized_rules_keep_v1_score_compatibility(
    void)
{
    cj4_rules zero_rules = {0};
    cj4_rules current_rules = cj4_rules_default();
    cj4_mahjong kokushi_state = make_empty_state();
    cj4_mahjong kazoe_state = make_empty_state();
    cj4_hand_score score;
    cj4_tile_id draw = tile(0, 1);
    const cj4_tile_id kokushi_hand[] = {
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
        tile(33, 0),
        draw};
    cj4_tile_id pinfu_draw = tile(5, 0);
    const cj4_tile_id pinfu_hand[] = {
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
        pinfu_draw};

    set_hand(&kokushi_state, CJ4_PLAYER_0, kokushi_hand, (uint8_t)(sizeof(kokushi_hand) / sizeof(kokushi_hand[0])));
    cj4_state_set_phase(&kokushi_state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&kokushi_state, CJ4_PLAYER_0);
    kokushi_state.draw_tile = draw;

    assert(cj4_calculate_hand_score(&kokushi_state, CJ4_PLAYER_0, &zero_rules, &score));
    assert(score.yakuman_count == 2);

    current_rules.kokushi_13_wait_double = 0;
    assert(cj4_calculate_hand_score(&kokushi_state, CJ4_PLAYER_0, &current_rules, &score));
    assert(score.yakuman_count == 1);

    current_rules = cj4_rules_default();
    set_hand(&kazoe_state, CJ4_PLAYER_0, pinfu_hand, (uint8_t)(sizeof(pinfu_hand) / sizeof(pinfu_hand[0])));
    cj4_state_set_current_player(&kazoe_state, CJ4_PLAYER_0);
    kazoe_state.dealer = CJ4_PLAYER_1;
    kazoe_state.draw_tile = pinfu_draw;
    cj4_state_set_phase(&kazoe_state, CJ4_PHASE_DRAW);
    cj4_state_set_riichi(&kazoe_state, CJ4_PLAYER_0, 1);
    cj4_state_set_double_riichi(&kazoe_state, CJ4_PLAYER_0, 1);
    kazoe_state.dora_count = 5;
    set_test_wall(&kazoe_state, (uint8_t)(130), tile(14, 0));
    set_test_wall(&kazoe_state, (uint8_t)(128), tile(14, 1));
    set_test_wall(&kazoe_state, (uint8_t)(126), tile(14, 2));
    set_test_wall(&kazoe_state, (uint8_t)(124), tile(14, 3));
    set_test_wall(&kazoe_state, (uint8_t)(122), tile(4, 0));
    set_test_wall(&kazoe_state, (uint8_t)(131), tile(2, 0));
    set_test_wall(&kazoe_state, (uint8_t)(129), tile(2, 1));
    set_test_wall(&kazoe_state, (uint8_t)(127), tile(2, 2));
    set_test_wall(&kazoe_state, (uint8_t)(125), tile(2, 3));
    set_test_wall(&kazoe_state, (uint8_t)(123), tile(3, 1));

    assert(cj4_calculate_hand_score(&kazoe_state, CJ4_PLAYER_0, &zero_rules, &score));
    assert(score.yakuman_count == 1);

    current_rules.kazoe_yakuman = 0;
    assert(cj4_calculate_hand_score(&kazoe_state, CJ4_PLAYER_0, &current_rules, &score));
    assert(score.yakuman_count == 0);
    assert(score.han >= 13);
}

void
cj4_test_rules(
    void)
{
    test_rules_default_and_validate();
    test_tenhou_preset_fields();
    test_double_riichi_is_always_enabled();
    test_score_rules_control_kazoe_and_kiriage();
    test_zero_initialized_rules_keep_v1_score_compatibility();
}
