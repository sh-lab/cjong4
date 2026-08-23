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
test_chankan_pass_records_furiten(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong next;
    const cj4_tile_id meld_tiles[] = {
        tile(1, 0),
        tile(2, 0),
        tile(3, 0)};
    const cj4_tile_id hand[] = {
        tile(11, 0),
        tile(12, 0),
        tile(13, 0),
        tile(21, 0),
        tile(22, 0),
        tile(23, 0),
        tile(5, 0),
        tile(6, 0),
        tile(16, 0),
        tile(16, 1)};

    rules.kuitan = 1;

    set_hand(&state, CJ4_PLAYER_2, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    for (uint8_t i = 0; i < 3; ++i)
    {
        state.locations[meld_tiles[i]].placement =
            cj4_location_make_meld(CJ4_PLAYER_2, 0, CJ4_MELD_CHI);
    }

    cj4_state_set_phase(&state, CJ4_PHASE_KAKAN_RESOLVE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.pending_kakan_tile = tile(4, 0);
    set_test_meld(&state, CJ4_PLAYER_2, 0, &(cj4_meld){.tiles = {meld_tiles[0], meld_tiles[1], meld_tiles[2]}, .size = 3, .type = CJ4_MELD_CHI, .from_player = CJ4_PLAYER_1, .called_index = 0});

    assert(cj4_can_ron(&state, CJ4_PLAYER_2, &rules));

    next = cj4_do_rinshan_draw(state, &rules);

    assert(cj4_state_temporary_furiten(&next, CJ4_PLAYER_2) == 1);
    assert(cj4_state_riichi_furiten(&next, CJ4_PLAYER_2) == 0);
}

static void
test_claim_tile_arguments_must_be_distinct(
    void)
{
    cj4_mahjong pon_state = make_empty_state();
    cj4_mahjong minkan_state = make_empty_state();
    const cj4_tile_id pon_hand[] = {
        tile(3, 1),
        tile(3, 2)};
    const cj4_tile_id minkan_hand[] = {
        tile(3, 1),
        tile(3, 2),
        tile(3, 3)};

    set_hand(&pon_state, CJ4_PLAYER_2, pon_hand, (uint8_t)(sizeof(pon_hand) / sizeof(pon_hand[0])));
    cj4_state_set_phase(&pon_state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&pon_state, CJ4_PLAYER_0);
    add_discard(&pon_state, CJ4_PLAYER_0, tile(3, 0));

    assert(cj4_can_pon(&pon_state, CJ4_PLAYER_2));
    assert(!cj4_can_pon_with_tile(&pon_state, CJ4_PLAYER_2, pon_hand[0], pon_hand[0]));

    set_hand(&minkan_state, CJ4_PLAYER_2, minkan_hand, (uint8_t)(sizeof(minkan_hand) / sizeof(minkan_hand[0])));
    cj4_state_set_phase(&minkan_state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&minkan_state, CJ4_PLAYER_0);
    add_discard(&minkan_state, CJ4_PLAYER_0, tile(3, 0));

    assert(cj4_can_minkan(&minkan_state, CJ4_PLAYER_2));
    assert(!cj4_can_minkan_with_tile(&minkan_state, CJ4_PLAYER_2, minkan_hand[0], minkan_hand[0], minkan_hand[1]));
}

static void
test_kuikae_forbidden_after_pon_and_chi(
    void)
{
    cj4_rules rules = cj4_rules_default();
    cj4_mahjong pon_state = make_empty_state();
    cj4_mahjong chi_state = make_empty_state();

    cj4_state_set_phase(&pon_state, CJ4_PHASE_AFTER_CALL);
    cj4_state_set_current_player(&pon_state, CJ4_PLAYER_1);
    set_test_meld(&pon_state, CJ4_PLAYER_1, 0, &(cj4_meld){.tiles = {tile(3, 0), tile(3, 1), tile(3, 2)}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_0, .called_index = 0});
    set_hand(&pon_state, CJ4_PLAYER_1, (const cj4_tile_id[]){tile(3, 3), tile(4, 0)}, 2);

    assert(!cj4_can_discard_with_rules(pon_state, &rules, tile(3, 3)));
    assert(cj4_can_discard_with_rules(pon_state, &rules, tile(4, 0)));
    rules.kuikae_forbidden = 0;
    assert(cj4_can_discard_with_rules(pon_state, &rules, tile(3, 3)));

    rules.kuikae_forbidden = 1;
    cj4_state_set_phase(&chi_state, CJ4_PHASE_AFTER_CALL);
    cj4_state_set_current_player(&chi_state, CJ4_PLAYER_1);
    set_test_meld(&chi_state, CJ4_PLAYER_1, 0, &(cj4_meld){.tiles = {tile(2, 0), tile(3, 0), tile(4, 0)}, .size = 3, .type = CJ4_MELD_CHI, .from_player = CJ4_PLAYER_0, .called_index = 0});
    set_hand(&chi_state, CJ4_PLAYER_1, (const cj4_tile_id[]){tile(2, 1), tile(5, 0), tile(6, 0)}, 3);

    assert(!cj4_can_discard_with_rules(chi_state, &rules, tile(2, 1)));
    assert(!cj4_can_discard_with_rules(chi_state, &rules, tile(5, 0)));
    assert(cj4_can_discard_with_rules(chi_state, &rules, tile(6, 0)));
}

static void
test_minkan_uses_rule_timing_and_draws_rinshan_immediately(
    void)
{
    cj4_rules early_rules = cj4_rules_default();
    cj4_rules late_rules = cj4_rules_tenhou();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong early;
    cj4_mahjong late;
    const cj4_tile_id hand[] = {
        tile(3, 1),
        tile(3, 2),
        tile(3, 3)};
    cj4_tile_id rinshan = tile(4, 0);

    set_hand(&state, CJ4_PLAYER_1, hand, 3);
    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    add_discard(&state, CJ4_PLAYER_0, tile(3, 0));
    state.dora_count = 1;
    set_test_wall(&state, 134, rinshan);

    early = cj4_do_minkan(
        state,
        &early_rules,
        CJ4_PLAYER_1,
        hand[0],
        hand[1],
        hand[2]);
    assert(cj4_state_phase(&early) == CJ4_PHASE_DRAW);
    assert(early.draw_tile == rinshan);
    assert(early.dora_count == 2);
    assert(early.pending_kan_dora_count == 0);

    late = cj4_do_minkan(
        state,
        &late_rules,
        CJ4_PLAYER_1,
        hand[0],
        hand[1],
        hand[2]);
    assert(cj4_state_phase(&late) == CJ4_PHASE_DRAW);
    assert(late.draw_tile == rinshan);
    assert(late.dora_count == 1);
    assert(late.pending_kan_dora_count == 1);
}

static void
test_kan_dora_timing_for_ankan_and_kakan(
    void)
{
    cj4_rules late_rules = cj4_rules_tenhou();
    cj4_mahjong ankan_state = make_empty_state();
    cj4_mahjong after_ankan;
    cj4_mahjong after_ankan_draw;
    cj4_mahjong kakan_state = make_empty_state();
    cj4_mahjong after_early_kakan_draw;
    cj4_mahjong after_kakan_draw;
    cj4_mahjong after_discard;
    const cj4_tile_id ankan_tiles[] = {
        tile(0, 0),
        tile(0, 1),
        tile(0, 2),
        tile(0, 3)};

    set_hand(&ankan_state, CJ4_PLAYER_0, ankan_tiles, 4);
    cj4_state_set_phase(&ankan_state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&ankan_state, CJ4_PLAYER_0);
    ankan_state.draw_tile = ankan_tiles[3];
    ankan_state.dora_count = 1;
    set_test_wall(&ankan_state, (uint8_t)(130), tile(9, 0));

    after_ankan = cj4_do_ankan(
        ankan_state,
        ankan_tiles[0],
        ankan_tiles[1],
        ankan_tiles[2],
        ankan_tiles[3]);

    assert(cj4_state_phase(&after_ankan) == CJ4_PHASE_ANKAN_RESOLVE);
    assert(after_ankan.dora_count == 1);
    assert(cj4_location_collect_melds(
               after_ankan.locations,
               CJ4_PLAYER_0)
               .count == 0);

    after_ankan_draw = cj4_do_rinshan_draw(after_ankan, NULL);
    assert(cj4_state_phase(&after_ankan_draw) == CJ4_PHASE_DRAW);
    assert(after_ankan_draw.dora_count == 2);
    assert(cj4_location_collect_melds(
               after_ankan_draw.locations,
               CJ4_PLAYER_0)
               .count == 1);

    cj4_state_set_phase(&kakan_state, CJ4_PHASE_KAKAN_RESOLVE);
    cj4_state_set_current_player(&kakan_state, CJ4_PLAYER_0);
    kakan_state.pending_kakan_tile = tile(3, 0);
    kakan_state.dora_count = 1;
    set_test_wall(&kakan_state, (uint8_t)(130), tile(9, 1));

    after_early_kakan_draw = cj4_do_rinshan_draw(kakan_state, NULL);
    assert(after_early_kakan_draw.dora_count == 2);
    assert(after_early_kakan_draw.pending_kan_dora_count == 0);

    after_kakan_draw = cj4_do_rinshan_draw(kakan_state, &late_rules);
    assert(cj4_state_phase(&after_kakan_draw) == CJ4_PHASE_DRAW);
    assert(after_kakan_draw.dora_count == 1);
    assert(after_kakan_draw.pending_kan_dora_count == 1);

    after_discard = cj4_do_discard(after_kakan_draw, after_kakan_draw.draw_tile);
    assert(after_discard.dora_count == 2);
    assert(after_discard.pending_kan_dora_count == 0);
}

static void
test_kan_requires_a_live_wall_tile(
    void)
{
    cj4_mahjong state = make_empty_state();
    const cj4_tile_id kan_tiles[] = {
        tile(0, 0),
        tile(0, 1),
        tile(0, 2),
        tile(0, 3)};

    set_hand(&state, CJ4_PLAYER_0, kan_tiles, 4);
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.draw_tile = kan_tiles[3];

    state.wall_pos = 121;
    assert(cj4_can_ankan(&state));

    state.wall_pos = 122;
    assert(!cj4_can_ankan(&state));
}

static void
test_consecutive_kakan_reveals_previous_dora_before_rinshan(
    void)
{
    cj4_rules rules = cj4_rules_tenhou();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong declared;
    cj4_mahjong after_rinshan;
    cj4_mahjong after_discard;
    cj4_tile_id added = tile(3, 3);
    const cj4_meld pon = {
        .tiles = {tile(3, 0), tile(3, 1), tile(3, 2)},
        .size = 3,
        .type = CJ4_MELD_PON,
        .from_player = CJ4_PLAYER_1,
        .called_index = 0};

    set_test_meld(&state, CJ4_PLAYER_0, 0, &pon);
    set_hand(&state, CJ4_PLAYER_0, &added, 1);
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.draw_tile = added;
    state.pending_kan_dora_count = 1;
    state.dora_count = 1;
    set_test_wall(&state, (uint8_t)(134), tile(4, 0));

    declared = cj4_do_kakan(state, added);
    assert(cj4_state_phase(&declared) == CJ4_PHASE_KAKAN_RESOLVE);
    assert(declared.pending_kan_dora_count == 0);
    assert(declared.dora_count == 2);

    after_rinshan = cj4_do_rinshan_draw(declared, &rules);
    assert(after_rinshan.pending_kan_dora_count == 1);
    assert(after_rinshan.dora_count == 2);

    after_discard = cj4_do_discard(after_rinshan, after_rinshan.draw_tile);
    assert(after_discard.pending_kan_dora_count == 0);
    assert(after_discard.dora_count == 3);
}

static void
test_minkan_then_ankan_preserves_pending_dora(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_mahjong after_ankan_declared;
    cj4_mahjong after_ankan_draw;
    cj4_mahjong after_discard;
    const cj4_tile_id ankan_tiles[] = {
        tile(0, 0),
        tile(0, 1),
        tile(0, 2),
        tile(0, 3)};

    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.pending_kan_dora_count = 1;
    state.dora_count = 1;
    set_test_wall(&state, (uint8_t)(135), tile(9, 0));

    set_hand(&state, CJ4_PLAYER_0, ankan_tiles, 4);
    state.draw_tile = ankan_tiles[3];
    after_ankan_declared = cj4_do_ankan(
        state,
        ankan_tiles[0],
        ankan_tiles[1],
        ankan_tiles[2],
        ankan_tiles[3]);
    assert(after_ankan_declared.pending_kan_dora_count == 0);
    assert(after_ankan_declared.dora_count == 2);

    after_ankan_draw = cj4_do_rinshan_draw(after_ankan_declared, NULL);
    assert(after_ankan_draw.pending_kan_dora_count == 0);
    assert(after_ankan_draw.dora_count == 3);

    after_discard = cj4_do_discard(after_ankan_draw, after_ankan_draw.draw_tile);
    assert(after_discard.pending_kan_dora_count == 0);
    assert(after_discard.dora_count == 3);
}

static void
test_previous_kan_dora_counts_on_following_rinshan_tsumo(
    void)
{
    cj4_rules rules = cj4_rules_tenhou();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong after_rinshan;
    cj4_mahjong without_previous_dora;
    cj4_mahjong won;
    cj4_hand_score score;
    cj4_hand_score score_without_previous_dora;
    cj4_hand_score won_score;
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
        tile(15, 1)};

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_KAKAN_RESOLVE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.dealer = CJ4_PLAYER_1;
    state.pending_kakan_tile = tile(6, 0);
    state.pending_kan_dora_count = 0;
    state.dora_count = 2;
    set_test_wall(&state, (uint8_t)(130), tile(30, 0));
    set_test_wall(&state, (uint8_t)(128), tile(4, 1));
    set_test_wall(&state, (uint8_t)(134), draw);

    after_rinshan = cj4_do_rinshan_draw(state, &rules);

    assert(cj4_state_phase(&after_rinshan) == CJ4_PHASE_DRAW);
    assert(after_rinshan.draw_tile == draw);
    assert(after_rinshan.dora_count == 2);
    assert(after_rinshan.pending_kan_dora_count == 1);
    assert(cj4_calculate_hand_score(
        &after_rinshan,
        CJ4_PLAYER_0,
        &rules,
        &score));

    without_previous_dora = after_rinshan;
    without_previous_dora.dora_count = 1;
    assert(cj4_calculate_hand_score(
        &without_previous_dora,
        CJ4_PLAYER_0,
        &rules,
        &score_without_previous_dora));
    assert(score.han == score_without_previous_dora.han + 1);

    won = cj4_do_tsumo(after_rinshan);
    assert(won.dora_count == 2);
    assert(won.pending_kan_dora_count == 0);
    assert(cj4_calculate_hand_score(
        &won,
        CJ4_PLAYER_0,
        &rules,
        &won_score));
    assert(won_score.han == score.han);
}

static void
test_pending_kan_dora_is_discarded_on_win_and_capped(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_mahjong won;
    cj4_mahjong discarded;
    cj4_player winner = CJ4_PLAYER_1;

    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.pending_kan_dora_count = 2;
    state.dora_count = 1;
    add_discard(&state, CJ4_PLAYER_0, tile(1, 0));

    won = cj4_do_ron_multi(state, &winner, 1, NULL);
    assert(won.pending_kan_dora_count == 0);
    assert(won.dora_count == 1);

    state = make_empty_state();
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.draw_tile = tile(2, 0);
    state.locations[state.draw_tile].placement =
        cj4_location_make_hand(CJ4_PLAYER_0);
    state.pending_kan_dora_count = 4;
    state.dora_count = 4;

    discarded = cj4_do_discard(state, state.draw_tile);
    assert(discarded.pending_kan_dora_count == 0);
    assert(discarded.dora_count == 5);
}

static void
test_kokushi_ron_on_ankan_rule(
    void)
{
    cj4_rules rules = cj4_rules_default();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong ankan;
    const cj4_tile_id ankan_tiles[] = {
        tile(0, 0),
        tile(0, 1),
        tile(0, 2),
        tile(0, 3)};
    const cj4_tile_id kokushi[] = {
        tile(8, 0),
        tile(8, 1),
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

    set_hand(&state, CJ4_PLAYER_0, ankan_tiles, 4);
    set_hand(&state, CJ4_PLAYER_1, kokushi, (uint8_t)(sizeof(kokushi) / sizeof(kokushi[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.draw_tile = ankan_tiles[3];

    ankan = cj4_do_ankan(
        state,
        ankan_tiles[0],
        ankan_tiles[1],
        ankan_tiles[2],
        ankan_tiles[3]);

    assert(cj4_can_ron(&ankan, CJ4_PLAYER_1, &rules));
    assert(ankan.dora_count == 0);
    assert(cj4_location_collect_melds(ankan.locations, CJ4_PLAYER_0).count == 0);
    rules.kokushi_ron_on_ankan = 0;
    assert(!cj4_can_ron(&ankan, CJ4_PLAYER_1, &rules));
}

static void
test_ankan_kokushi_ron_leaves_no_committed_kan(
    void)
{
    cj4_rules rules = cj4_rules_default();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong ankan;
    cj4_mahjong ron;
    cj4_win_result results[CJ4_PLAYER_COUNT];
    uint8_t result_count = 0;
    cj4_player winner = CJ4_PLAYER_1;
    const cj4_tile_id ankan_tiles[] = {
        tile(0, 0),
        tile(0, 1),
        tile(0, 2),
        tile(0, 3)};
    const cj4_tile_id kokushi[] = {
        tile(8, 0),
        tile(8, 1),
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

    set_hand(&state, CJ4_PLAYER_0, ankan_tiles, 4);
    set_hand(&state, CJ4_PLAYER_1, kokushi, (uint8_t)(sizeof(kokushi) / sizeof(kokushi[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.draw_tile = ankan_tiles[3];
    state.dora_count = 1;

    ankan = cj4_do_ankan(
        state,
        ankan_tiles[0],
        ankan_tiles[1],
        ankan_tiles[2],
        ankan_tiles[3]);
    ron = cj4_do_ron_multi(ankan, &winner, 1, &rules);

    assert(cj4_state_phase(&ron) == CJ4_PHASE_ROUND_END);
    assert(ron.dora_count == 1);
    assert(cj4_location_collect_melds(ron.locations, CJ4_PLAYER_0).count == 0);
    assert(cj4_location_is_hand(ron.locations[ankan_tiles[1]].placement));
    assert(cj4_location_placement_player(
               ron.locations[ankan_tiles[1]].placement) == CJ4_PLAYER_0);
    assert(cj4_collect_winning_results(
        &ron,
        &rules,
        results,
        CJ4_PLAYER_COUNT,
        &result_count));
    assert(result_count == 1);
    assert(results[0].player == CJ4_PLAYER_1);
    assert(contains_win_yaku(&results[0], CJ4_WIN_YAKU_KOKUSHI) ||
           contains_win_yaku(&results[0], CJ4_WIN_YAKU_KOKUSHI_13_WAIT));
}

static void
test_last_discard_cannot_be_called(
    void)
{
    cj4_mahjong state = make_empty_state();
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 1),
        tile(2, 2),
        tile(2, 3)};

    set_hand(&state, CJ4_PLAYER_1, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.wall_pos = 121;
    add_discard(&state, CJ4_PLAYER_0, tile(2, 0));

    assert(cj4_can_chi(&state));
    assert(cj4_can_pon(&state, CJ4_PLAYER_1));
    assert(cj4_can_minkan(&state, CJ4_PLAYER_1));

    state.wall_pos = 122;
    assert(!cj4_can_chi(&state));
    assert(!cj4_can_pon(&state, CJ4_PLAYER_1));
    assert(!cj4_can_minkan(&state, CJ4_PLAYER_1));
}

void
cj4_test_calls_kan(
    void)
{
    test_chankan_pass_records_furiten();
    test_claim_tile_arguments_must_be_distinct();
    test_kuikae_forbidden_after_pon_and_chi();
    test_minkan_uses_rule_timing_and_draws_rinshan_immediately();
    test_kan_dora_timing_for_ankan_and_kakan();
    test_kan_requires_a_live_wall_tile();
    test_consecutive_kakan_reveals_previous_dora_before_rinshan();
    test_minkan_then_ankan_preserves_pending_dora();
    test_previous_kan_dora_counts_on_following_rinshan_tsumo();
    test_pending_kan_dora_is_discarded_on_win_and_capped();
    test_kokushi_ron_on_ankan_rule();
    test_ankan_kokushi_ron_leaves_no_committed_kan();
    test_last_discard_cannot_be_called();
}
