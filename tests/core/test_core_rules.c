#include <assert.h>
#include <string.h>

#include "../../src/core/hand_check.h"
#include "../../src/core/state_score.h"
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
#include "cjong4/core/state_settle.h"
#include "cjong4/core/state_tsumo.h"

int
manager_tests_main(void);

static cj4_tile_id
tile(
    cj4_tile_type type,
    uint8_t index)
{
    return cj4_tile_make(type, index);
}

static cj4_mahjong
make_empty_state(
    void)
{
    cj4_mahjong state;

    memset(&state, 0, sizeof(state));
    state.phase = CJ4_PHASE_DRAW;
    state.dealer = CJ4_PLAYER_0;
    state.current_player = CJ4_PLAYER_0;
    state.round_wind = CJ4_WIND_EAST;
    state.draw_tile = CJ4_TILE_ID_INVALID;
    state.pending_kakan_tile = CJ4_TILE_ID_INVALID;
    state.pending_ankan_tile = CJ4_TILE_ID_INVALID;
    for (uint8_t i = 0; i < 4; ++i)
        state.pending_ankan_tiles[i] = CJ4_TILE_ID_INVALID;
    state.pending_riichi_player = CJ4_PLAYER_COUNT;
    state.winning_tile = CJ4_TILE_ID_INVALID;
    state.round_end_type = CJ4_ROUND_END_NONE;

    for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
        state.scores[i] = 25000;

    return state;
}

static void
set_hand(
    cj4_mahjong *state,
    cj4_player player,
    const cj4_tile_id *tiles,
    uint8_t count)
{
    for (uint8_t i = 0; i < count; ++i)
    {
        state->locations[tiles[i]].zone = CJ4_ZONE_HAND;
        state->locations[tiles[i]].owner = player;
    }
}

static void
add_discard(
    cj4_mahjong *state,
    cj4_player player,
    cj4_tile_id discarded)
{
    cj4_discard *entry = &state->discards[state->discard_count++];
    entry->tile = discarded;
    entry->player = player;
    entry->is_active = 1;
    entry->is_tsumogiri = 0;
    state->locations[discarded].zone = CJ4_ZONE_RIVER;
    state->locations[discarded].owner = player;
}

static uint8_t
contains_win_yaku(
    const cj4_win_result *result,
    cj4_win_yaku yaku)
{
    for (uint8_t i = 0; i < result->yaku_count; ++i)
    {
        if (result->yaku[i] == yaku)
            return 1;
    }

    return 0;
}

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
    state.current_player = CJ4_PLAYER_0;
    state.phase = CJ4_PHASE_DRAW;
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
    state.current_player = CJ4_PLAYER_0;
    state.phase = CJ4_PHASE_DRAW;
    state.draw_tile = draw;

    state.wall_pos = 118;
    assert(cj4_can_riichi(&state, draw));

    state.wall_pos = 119;
    assert(!cj4_can_riichi(&state, draw));
}

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

    rules.max_ron_players = 0;
    assert(!cj4_rules_validate(&rules));
}

static void
test_tenhou_preset_fields(
    void)
{
    cj4_rules rules = cj4_rules_tenhou();

    assert(cj4_rules_validate(&rules));
    assert(rules.version == CJ4_RULES_VERSION);
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
    state.current_player = CJ4_PLAYER_0;
    state.phase = CJ4_PHASE_DRAW;
    state.draw_tile = draw;
    state.first_turn_uninterrupted = 1;
    state.draw_turn_count[CJ4_PLAYER_0] = 1;
    state.wall[0] = tile(30, 0);

    declared = cj4_do_riichi(state, draw);

    assert(declared.pending_riichi == 1);
    assert(declared.is_riichi[CJ4_PLAYER_0] == 0);
    assert(declared.scores[CJ4_PLAYER_0] == 25000);
    assert(declared.riichi_sticks == 0);

    established = cj4_do_pass(declared, NULL);

    assert(established.pending_riichi == 0);
    assert(established.is_riichi[CJ4_PLAYER_0] == 1);
    assert(established.is_ippatsu[CJ4_PLAYER_0] == 1);
    assert(established.riichi_declared_on_first_turn[CJ4_PLAYER_0] == 1);
    assert(established.scores[CJ4_PLAYER_0] == 24000);
    assert(established.riichi_sticks == 1);
    assert(!cj4_can_discard(established, tile(0, 0)));
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
    state.current_player = CJ4_PLAYER_0;
    state.phase = CJ4_PHASE_DRAW;
    state.draw_tile = draw;

    declared = cj4_do_riichi(state, draw);
    ron = cj4_do_ron_multi(declared, &winner, 1, NULL);

    assert(ron.phase == CJ4_PHASE_ROUND_END);
    assert(ron.pending_riichi == 0);
    assert(ron.is_riichi[CJ4_PLAYER_0] == 0);
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
    const cj4_tile_id pon_tiles[] = {tile(26, 1), tile(26, 2)};

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    set_hand(&state, CJ4_PLAYER_2, pon_tiles, 2);
    state.current_player = CJ4_PLAYER_0;
    state.phase = CJ4_PHASE_DRAW;
    state.draw_tile = draw;

    declared = cj4_do_riichi(state, draw);
    called = cj4_do_pon(declared, CJ4_PLAYER_2, pon_tiles[0], pon_tiles[1]);

    assert(called.is_riichi[CJ4_PLAYER_0] == 1);
    assert(called.pending_riichi == 0);
    assert(called.scores[CJ4_PLAYER_0] == 24000);
    assert(called.riichi_sticks == 1);
    assert(called.is_ippatsu[CJ4_PLAYER_0] == 0);
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
    state.phase = CJ4_PHASE_DISCARD;
    state.current_player = CJ4_PLAYER_0;
    add_discard(&state, CJ4_PLAYER_0, tile(6, 0));

    assert(cj4_can_ron(&state, CJ4_PLAYER_2, &rules));

    after_pass = cj4_do_pass(state, &rules);
    assert(after_pass.temporary_furiten[CJ4_PLAYER_2] == 1);
    assert(after_pass.riichi_furiten[CJ4_PLAYER_2] == 0);

    later = after_pass;
    later.phase = CJ4_PHASE_DISCARD;
    later.current_player = CJ4_PLAYER_1;
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
    state.phase = CJ4_PHASE_DISCARD;
    state.current_player = CJ4_PLAYER_0;
    state.is_riichi[CJ4_PLAYER_2] = 1;
    add_discard(&state, CJ4_PLAYER_0, tile(6, 0));

    assert(cj4_can_ron(&state, CJ4_PLAYER_2, &rules));

    after_pass = cj4_do_pass(state, &rules);
    assert(after_pass.temporary_furiten[CJ4_PLAYER_2] == 0);
    assert(after_pass.riichi_furiten[CJ4_PLAYER_2] == 1);

    later = after_pass;
    later.phase = CJ4_PHASE_DISCARD;
    later.current_player = CJ4_PLAYER_1;
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
    state.phase = CJ4_PHASE_DISCARD;
    state.current_player = CJ4_PLAYER_0;
    add_discard(&state, CJ4_PLAYER_2, tile(6, 0));
    add_discard(&state, CJ4_PLAYER_0, tile(3, 1));

    assert(!cj4_can_ron(&state, CJ4_PLAYER_2, &rules));
}

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

    state.phase = CJ4_PHASE_ANKAN_RESOLVE;
    state.current_player = CJ4_PLAYER_0;
    state.dead_wall_draw_count = 3;
    state.honba = 0;

    state.meld_count[CJ4_PLAYER_0] = 2;
    state.melds[CJ4_PLAYER_0][0].type = CJ4_MELD_ANKAN;
    state.melds[CJ4_PLAYER_0][1].type = CJ4_MELD_KAKAN;
    state.meld_count[CJ4_PLAYER_1] = 2;
    state.melds[CJ4_PLAYER_1][0].type = CJ4_MELD_MINKAN;
    state.melds[CJ4_PLAYER_1][1].type = CJ4_MELD_ANKAN;

    round_end = cj4_do_rinshan_draw(state, &rules);
    assert(round_end.phase == CJ4_PHASE_ROUND_END);
    assert(round_end.round_end_type == CJ4_ROUND_END_ABORTIVE_DRAW);
    assert(round_end.dead_wall_draw_count == 3);

    settled = cj4_do_settle(round_end, &rules);
    assert(settled.phase == CJ4_PHASE_SETTLE);
    assert(settled.scores[0] == 25000);
    assert(settled.scores[1] == 25000);
    assert(settled.scores[2] == 25000);
    assert(settled.scores[3] == 25000);
    assert(settled.next_dealer == CJ4_PLAYER_0);
    assert(settled.honba == 1);
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
    discard_state.phase = CJ4_PHASE_DRAW;
    discard_state.current_player = CJ4_PLAYER_0;
    discard_state.draw_tile = draw;
    discard_state.is_riichi[CJ4_PLAYER_0] = 1;

    assert(cj4_can_discard(discard_state, draw));
    assert(!cj4_can_discard(discard_state, tile(0, 0)));

    set_hand(&chi_state, CJ4_PLAYER_1, chi_hand, (uint8_t)(sizeof(chi_hand) / sizeof(chi_hand[0])));
    chi_state.phase = CJ4_PHASE_DISCARD;
    chi_state.current_player = CJ4_PLAYER_0;
    chi_state.is_riichi[CJ4_PLAYER_1] = 1;
    add_discard(&chi_state, CJ4_PLAYER_0, tile(3, 0));

    assert(!cj4_can_chi(&chi_state));
    assert(!cj4_can_chi_with_tile(&chi_state, chi_hand[0], chi_hand[1]));

    set_hand(&pon_state, CJ4_PLAYER_2, pon_hand, (uint8_t)(sizeof(pon_hand) / sizeof(pon_hand[0])));
    pon_state.phase = CJ4_PHASE_DISCARD;
    pon_state.current_player = CJ4_PLAYER_0;
    pon_state.is_riichi[CJ4_PLAYER_2] = 1;
    add_discard(&pon_state, CJ4_PLAYER_0, tile(3, 0));

    assert(!cj4_can_pon(&pon_state, CJ4_PLAYER_2));
    assert(!cj4_can_pon_with_tile(&pon_state, CJ4_PLAYER_2, pon_hand[0], pon_hand[1]));

    set_hand(&minkan_state, CJ4_PLAYER_2, minkan_hand, (uint8_t)(sizeof(minkan_hand) / sizeof(minkan_hand[0])));
    minkan_state.phase = CJ4_PHASE_DISCARD;
    minkan_state.current_player = CJ4_PLAYER_0;
    minkan_state.is_riichi[CJ4_PLAYER_2] = 1;
    add_discard(&minkan_state, CJ4_PLAYER_0, tile(3, 0));

    assert(!cj4_can_minkan(&minkan_state, CJ4_PLAYER_2));
    assert(!cj4_can_minkan_with_tile(&minkan_state, CJ4_PLAYER_2, minkan_hand[0], minkan_hand[1], minkan_hand[2]));

    set_hand(&kakan_state, CJ4_PLAYER_0, kakan_hand, (uint8_t)(sizeof(kakan_hand) / sizeof(kakan_hand[0])));
    kakan_state.phase = CJ4_PHASE_DRAW;
    kakan_state.current_player = CJ4_PLAYER_0;
    kakan_state.is_riichi[CJ4_PLAYER_0] = 1;
    kakan_state.draw_tile = kakan_hand[0];
    kakan_state.meld_count[CJ4_PLAYER_0] = 1;
    kakan_state.melds[CJ4_PLAYER_0][0] = (cj4_meld){
        .tiles = {tile(4, 0), tile(4, 1), tile(4, 2)},
        .size = 3,
        .type = CJ4_MELD_PON,
        .from_player = CJ4_PLAYER_1,
        .called_index = 0};

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
        state.locations[meld_tiles[i]].zone = CJ4_ZONE_MELD;
        state.locations[meld_tiles[i]].owner = CJ4_PLAYER_2;
    }

    state.phase = CJ4_PHASE_KAKAN_RESOLVE;
    state.current_player = CJ4_PLAYER_0;
    state.pending_kakan_tile = tile(4, 0);
    state.meld_count[CJ4_PLAYER_2] = 1;
    state.melds[CJ4_PLAYER_2][0] = (cj4_meld){
        .tiles = {meld_tiles[0], meld_tiles[1], meld_tiles[2]},
        .size = 3,
        .type = CJ4_MELD_CHI,
        .from_player = CJ4_PLAYER_1,
        .called_index = 0};

    assert(cj4_can_ron(&state, CJ4_PLAYER_2, &rules));

    next = cj4_do_rinshan_draw(state, &rules);

    assert(next.temporary_furiten[CJ4_PLAYER_2] == 1);
    assert(next.riichi_furiten[CJ4_PLAYER_2] == 0);
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
    pon_state.phase = CJ4_PHASE_DISCARD;
    pon_state.current_player = CJ4_PLAYER_0;
    add_discard(&pon_state, CJ4_PLAYER_0, tile(3, 0));

    assert(cj4_can_pon(&pon_state, CJ4_PLAYER_2));
    assert(!cj4_can_pon_with_tile(&pon_state, CJ4_PLAYER_2, pon_hand[0], pon_hand[0]));

    set_hand(&minkan_state, CJ4_PLAYER_2, minkan_hand, (uint8_t)(sizeof(minkan_hand) / sizeof(minkan_hand[0])));
    minkan_state.phase = CJ4_PHASE_DISCARD;
    minkan_state.current_player = CJ4_PLAYER_0;
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

    pon_state.phase = CJ4_PHASE_AFTER_CALL;
    pon_state.current_player = CJ4_PLAYER_1;
    pon_state.meld_count[CJ4_PLAYER_1] = 1;
    pon_state.melds[CJ4_PLAYER_1][0] = (cj4_meld){
        .tiles = {tile(3, 0), tile(3, 1), tile(3, 2)},
        .size = 3,
        .type = CJ4_MELD_PON,
        .from_player = CJ4_PLAYER_0,
        .called_index = 0};
    set_hand(&pon_state, CJ4_PLAYER_1, (const cj4_tile_id[]){tile(3, 3), tile(4, 0)}, 2);

    assert(!cj4_can_discard_with_rules(pon_state, &rules, tile(3, 3)));
    assert(cj4_can_discard_with_rules(pon_state, &rules, tile(4, 0)));
    rules.kuikae_forbidden = 0;
    assert(cj4_can_discard_with_rules(pon_state, &rules, tile(3, 3)));

    rules.kuikae_forbidden = 1;
    chi_state.phase = CJ4_PHASE_AFTER_CALL;
    chi_state.current_player = CJ4_PLAYER_1;
    chi_state.meld_count[CJ4_PLAYER_1] = 1;
    chi_state.melds[CJ4_PLAYER_1][0] = (cj4_meld){
        .tiles = {tile(2, 0), tile(3, 0), tile(4, 0)},
        .size = 3,
        .type = CJ4_MELD_CHI,
        .from_player = CJ4_PLAYER_0,
        .called_index = 0};
    set_hand(&chi_state, CJ4_PLAYER_1, (const cj4_tile_id[]){tile(2, 1), tile(5, 0), tile(6, 0)}, 3);

    assert(!cj4_can_discard_with_rules(chi_state, &rules, tile(2, 1)));
    assert(!cj4_can_discard_with_rules(chi_state, &rules, tile(5, 0)));
    assert(cj4_can_discard_with_rules(chi_state, &rules, tile(6, 0)));
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
    state.phase = CJ4_PHASE_DRAW;
    state.current_player = CJ4_PLAYER_0;
    state.draw_tile = tile(14, 1);
    state.meld_count[CJ4_PLAYER_0] = 1;
    state.melds[CJ4_PLAYER_0][0].type = CJ4_MELD_ANKAN;
    state.meld_count[CJ4_PLAYER_1] = 1;
    state.melds[CJ4_PLAYER_1][0].type = CJ4_MELD_MINKAN;
    state.meld_count[CJ4_PLAYER_2] = 1;
    state.melds[CJ4_PLAYER_2][0].type = CJ4_MELD_KAKAN;

    round_end = cj4_do_ankan(
        state,
        ankan_tiles[0],
        ankan_tiles[1],
        ankan_tiles[2],
        ankan_tiles[3]);

    assert(round_end.phase == CJ4_PHASE_ANKAN_RESOLVE);

    round_end = cj4_do_rinshan_draw(round_end, NULL);

    assert(round_end.phase == CJ4_PHASE_ROUND_END);
    assert(round_end.round_end_type == CJ4_ROUND_END_ABORTIVE_DRAW);
    assert(round_end.draw_tile == CJ4_TILE_ID_INVALID);
    assert(round_end.dead_wall_draw_count == 0);
}

static void
test_kan_dora_timing_for_ankan_and_kakan(
    void)
{
    cj4_mahjong ankan_state = make_empty_state();
    cj4_mahjong after_ankan;
    cj4_mahjong after_ankan_draw;
    cj4_mahjong kakan_state = make_empty_state();
    cj4_mahjong after_kakan_draw;
    cj4_mahjong after_discard;
    const cj4_tile_id ankan_tiles[] = {
        tile(0, 0),
        tile(0, 1),
        tile(0, 2),
        tile(0, 3)};

    set_hand(&ankan_state, CJ4_PLAYER_0, ankan_tiles, 4);
    ankan_state.phase = CJ4_PHASE_DRAW;
    ankan_state.current_player = CJ4_PLAYER_0;
    ankan_state.draw_tile = ankan_tiles[3];
    ankan_state.dora_indicators_count = 1;
    ankan_state.wall[130] = tile(9, 0);

    after_ankan = cj4_do_ankan(
        ankan_state,
        ankan_tiles[0],
        ankan_tiles[1],
        ankan_tiles[2],
        ankan_tiles[3]);

    assert(after_ankan.phase == CJ4_PHASE_ANKAN_RESOLVE);
    assert(after_ankan.dora_indicators_count == 1);
    assert(after_ankan.meld_count[CJ4_PLAYER_0] == 0);

    after_ankan_draw = cj4_do_rinshan_draw(after_ankan, NULL);
    assert(after_ankan_draw.phase == CJ4_PHASE_DRAW);
    assert(after_ankan_draw.dora_indicators_count == 2);
    assert(after_ankan_draw.meld_count[CJ4_PLAYER_0] == 1);

    kakan_state.phase = CJ4_PHASE_KAKAN_RESOLVE;
    kakan_state.current_player = CJ4_PLAYER_0;
    kakan_state.pending_kakan_tile = tile(3, 0);
    kakan_state.dora_indicators_count = 1;
    kakan_state.wall[130] = tile(9, 1);

    after_kakan_draw = cj4_do_rinshan_draw(kakan_state, NULL);
    assert(after_kakan_draw.phase == CJ4_PHASE_DRAW);
    assert(after_kakan_draw.dora_indicators_count == 1);
    assert(after_kakan_draw.pending_kan_dora == 1);

    after_discard = cj4_do_discard(after_kakan_draw, after_kakan_draw.draw_tile);
    assert(after_discard.dora_indicators_count == 2);
    assert(after_discard.pending_kan_dora == 0);
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
    state.phase = CJ4_PHASE_DRAW;
    state.current_player = CJ4_PLAYER_0;
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
    cj4_mahjong state = make_empty_state();
    cj4_mahjong after_first;
    cj4_mahjong after_second;
    cj4_mahjong after_discard;

    state.phase = CJ4_PHASE_KAKAN_RESOLVE;
    state.current_player = CJ4_PLAYER_0;
    state.pending_kakan_tile = tile(3, 0);
    state.dora_indicators_count = 1;
    state.wall[134] = tile(4, 0);
    state.wall[135] = tile(5, 0);

    after_first = cj4_do_rinshan_draw(state, NULL);
    assert(after_first.pending_kan_dora == 1);
    assert(after_first.dora_indicators_count == 1);

    after_first.phase = CJ4_PHASE_KAKAN_RESOLVE;
    after_first.pending_kakan_tile = tile(6, 0);
    after_second = cj4_do_rinshan_draw(after_first, NULL);
    assert(after_second.pending_kan_dora == 1);
    assert(after_second.dora_indicators_count == 2);

    after_discard = cj4_do_discard(after_second, after_second.draw_tile);
    assert(after_discard.pending_kan_dora == 0);
    assert(after_discard.dora_indicators_count == 3);
}

static void
test_minkan_then_ankan_preserves_pending_dora(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_mahjong after_minkan;
    cj4_mahjong after_ankan_declared;
    cj4_mahjong after_ankan_draw;
    cj4_mahjong after_discard;
    const cj4_tile_id ankan_tiles[] = {
        tile(0, 0),
        tile(0, 1),
        tile(0, 2),
        tile(0, 3)};

    state.phase = CJ4_PHASE_ANKAN_RESOLVE;
    state.current_player = CJ4_PLAYER_0;
    state.pending_ankan_tile = CJ4_TILE_ID_INVALID;
    state.dora_indicators_count = 1;
    state.wall[134] = tile(0, 3);
    state.wall[135] = tile(9, 0);

    after_minkan = cj4_do_rinshan_draw(state, NULL);
    assert(after_minkan.pending_kan_dora == 1);

    set_hand(&after_minkan, CJ4_PLAYER_0, ankan_tiles, 4);
    after_minkan.draw_tile = ankan_tiles[3];
    after_ankan_declared = cj4_do_ankan(
        after_minkan,
        ankan_tiles[0],
        ankan_tiles[1],
        ankan_tiles[2],
        ankan_tiles[3]);
    assert(after_ankan_declared.pending_kan_dora == 1);
    assert(after_ankan_declared.dora_indicators_count == 1);

    after_ankan_draw = cj4_do_rinshan_draw(after_ankan_declared, NULL);
    assert(after_ankan_draw.pending_kan_dora == 0);
    assert(after_ankan_draw.dora_indicators_count == 3);

    after_discard = cj4_do_discard(after_ankan_draw, after_ankan_draw.draw_tile);
    assert(after_discard.pending_kan_dora == 0);
    assert(after_discard.dora_indicators_count == 3);
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
    state.phase = CJ4_PHASE_KAKAN_RESOLVE;
    state.current_player = CJ4_PLAYER_0;
    state.dealer = CJ4_PLAYER_1;
    state.pending_kakan_tile = tile(6, 0);
    state.pending_kan_dora = 1;
    state.dora_indicators_count = 1;
    state.wall[130] = tile(30, 0);
    state.wall[128] = tile(4, 1);
    state.wall[134] = draw;

    after_rinshan = cj4_do_rinshan_draw(state, &rules);

    assert(after_rinshan.phase == CJ4_PHASE_DRAW);
    assert(after_rinshan.draw_tile == draw);
    assert(after_rinshan.dora_indicators_count == 2);
    assert(after_rinshan.pending_kan_dora == 1);
    assert(cj4_calculate_hand_score(
        &after_rinshan,
        CJ4_PLAYER_0,
        &rules,
        &score));

    without_previous_dora = after_rinshan;
    without_previous_dora.dora_indicators_count = 1;
    assert(cj4_calculate_hand_score(
        &without_previous_dora,
        CJ4_PLAYER_0,
        &rules,
        &score_without_previous_dora));
    assert(score.han == score_without_previous_dora.han + 1);

    won = cj4_do_tsumo(after_rinshan);
    assert(won.dora_indicators_count == 2);
    assert(won.pending_kan_dora == 0);
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

    state.phase = CJ4_PHASE_DISCARD;
    state.current_player = CJ4_PLAYER_0;
    state.pending_kan_dora = 2;
    state.dora_indicators_count = 1;
    add_discard(&state, CJ4_PLAYER_0, tile(1, 0));

    won = cj4_do_ron_multi(state, &winner, 1, NULL);
    assert(won.pending_kan_dora == 0);
    assert(won.dora_indicators_count == 1);

    state = make_empty_state();
    state.phase = CJ4_PHASE_DRAW;
    state.current_player = CJ4_PLAYER_0;
    state.draw_tile = tile(2, 0);
    state.locations[state.draw_tile].zone = CJ4_ZONE_HAND;
    state.locations[state.draw_tile].owner = CJ4_PLAYER_0;
    state.pending_kan_dora = 4;
    state.dora_indicators_count = 4;

    discarded = cj4_do_discard(state, state.draw_tile);
    assert(discarded.pending_kan_dora == 0);
    assert(discarded.dora_indicators_count == 5);
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
    state.phase = CJ4_PHASE_DRAW;
    state.current_player = CJ4_PLAYER_0;
    state.draw_tile = ankan_tiles[3];

    ankan = cj4_do_ankan(
        state,
        ankan_tiles[0],
        ankan_tiles[1],
        ankan_tiles[2],
        ankan_tiles[3]);

    assert(cj4_can_ron(&ankan, CJ4_PLAYER_1, &rules));
    assert(ankan.dora_indicators_count == 0);
    assert(ankan.meld_count[CJ4_PLAYER_0] == 0);
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
    state.phase = CJ4_PHASE_DRAW;
    state.current_player = CJ4_PLAYER_0;
    state.draw_tile = ankan_tiles[3];
    state.dora_indicators_count = 1;

    ankan = cj4_do_ankan(
        state,
        ankan_tiles[0],
        ankan_tiles[1],
        ankan_tiles[2],
        ankan_tiles[3]);
    ron = cj4_do_ron_multi(ankan, &winner, 1, &rules);

    assert(ron.phase == CJ4_PHASE_ROUND_END);
    assert(ron.dora_indicators_count == 1);
    assert(ron.meld_count[CJ4_PLAYER_0] == 0);
    assert(ron.locations[ankan_tiles[1]].zone == CJ4_ZONE_HAND);
    assert(ron.locations[ankan_tiles[1]].owner == CJ4_PLAYER_0);
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

    state.phase = CJ4_PHASE_DISCARD;
    state.current_player = CJ4_PLAYER_0;
    state.dealer = CJ4_PLAYER_0;
    state.honba = 2;
    state.riichi_sticks = 1;
    add_discard(&state, CJ4_PLAYER_0, tile(0, 0));

    round_end = cj4_do_ron_multi(state, claimers, 3, &rules);

    assert(round_end.phase == CJ4_PHASE_ROUND_END);
    assert(round_end.round_end_type == CJ4_ROUND_END_ABORTIVE_DRAW);
    assert(round_end.abortive_draw_reason == CJ4_ABORTIVE_DRAW_TRIPLE_RON);
    assert(round_end.winner_count == 0);

    settled = cj4_do_settle(round_end, &rules);
    assert(settled.honba == 3);
    assert(settled.riichi_sticks == 1);
    assert(settled.scores[CJ4_PLAYER_0] == 25000);
}

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
    state.phase = CJ4_PHASE_ROUND_END;
    state.round_end_type = CJ4_ROUND_END_EXHAUSTIVE_DRAW;
    state.current_player = CJ4_PLAYER_0;
    state.dealer = CJ4_PLAYER_0;
    state.winner_count = 0;

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

    state.phase = CJ4_PHASE_DISCARD;
    state.current_player = CJ4_PLAYER_0;
    state.first_turn_uninterrupted = 0;
    state.discard_count = CJ4_MAX_DRAWS;
    state.wall_pos = 120;
    state.dead_wall_draw_count = 1;
    state.wall[120] = last_draw;

    drawn = cj4_do_pass(state, NULL);

    assert(drawn.phase == CJ4_PHASE_DRAW);
    assert(drawn.current_player == CJ4_PLAYER_1);
    assert(drawn.draw_tile == last_draw);
    assert(drawn.wall_pos == 121);

    discarded = cj4_do_discard(drawn, last_draw);
    round_end = cj4_do_pass(discarded, NULL);

    assert(round_end.phase == CJ4_PHASE_ROUND_END);
    assert(round_end.round_end_type == CJ4_ROUND_END_EXHAUSTIVE_DRAW);
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
    state.phase = CJ4_PHASE_DISCARD;
    state.current_player = CJ4_PLAYER_0;
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
    state.phase = CJ4_PHASE_DRAW;
    state.current_player = CJ4_PLAYER_0;
    state.dealer = CJ4_PLAYER_1;
    state.draw_tile = draw;
    state.wall_pos = 121;
    state.dead_wall_draw_count = 1;
    state.wall[120] = draw;

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
    state.phase = CJ4_PHASE_ROUND_END;
    state.round_end_type = CJ4_ROUND_END_EXHAUSTIVE_DRAW;
    state.current_player = CJ4_PLAYER_0;
    state.dealer = CJ4_PLAYER_0;
    state.honba = 2;
    state.winner_count = 0;

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
    state.phase = CJ4_PHASE_ROUND_END;
    state.round_wind = CJ4_WIND_EAST;
    state.round_end_type = CJ4_ROUND_END_EXHAUSTIVE_DRAW;
    state.current_player = CJ4_PLAYER_3;
    state.dealer = CJ4_PLAYER_3;
    state.winner_count = 0;
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
    state.phase = CJ4_PHASE_ROUND_END;
    state.round_wind = CJ4_WIND_SOUTH;
    state.round_end_type = CJ4_ROUND_END_EXHAUSTIVE_DRAW;
    state.current_player = CJ4_PLAYER_3;
    state.dealer = CJ4_PLAYER_3;
    state.winner_count = 0;
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
    state.phase = CJ4_PHASE_ROUND_END;
    state.round_wind = CJ4_WIND_EAST;
    state.round_end_type = CJ4_ROUND_END_EXHAUSTIVE_DRAW;
    state.current_player = CJ4_PLAYER_3;
    state.dealer = CJ4_PLAYER_3;
    state.winner_count = 0;
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

    state.phase = CJ4_PHASE_ROUND_END;
    state.round_wind = CJ4_WIND_EAST;
    state.round_end_type = CJ4_ROUND_END_EXHAUSTIVE_DRAW;
    state.current_player = CJ4_PLAYER_2;
    state.dealer = CJ4_PLAYER_2;
    state.winner_count = 0;
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

    state.phase = CJ4_PHASE_ROUND_END;
    state.round_wind = CJ4_WIND_EAST;
    state.round_end_type = CJ4_ROUND_END_ABORTIVE_DRAW;
    state.current_player = CJ4_PLAYER_2;
    state.dealer = CJ4_PLAYER_2;
    state.winner_count = 0;
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
    state.phase = CJ4_PHASE_ROUND_END;
    state.round_wind = CJ4_WIND_EAST;
    state.round_end_type = CJ4_ROUND_END_EXHAUSTIVE_DRAW;
    state.current_player = CJ4_PLAYER_3;
    state.dealer = CJ4_PLAYER_3;
    state.winner_count = 0;
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
    state.phase = CJ4_PHASE_ROUND_END;
    state.round_wind = CJ4_WIND_EAST;
    state.round_end_type = CJ4_ROUND_END_EXHAUSTIVE_DRAW;
    state.current_player = CJ4_PLAYER_3;
    state.dealer = CJ4_PLAYER_3;
    state.honba = 2;
    state.winner_count = 0;
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
    state.phase = CJ4_PHASE_ROUND_END;
    state.round_wind = CJ4_WIND_EAST;
    state.round_end_type = CJ4_ROUND_END_EXHAUSTIVE_DRAW;
    state.current_player = CJ4_PLAYER_3;
    state.dealer = CJ4_PLAYER_3;
    state.winner_count = 0;
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
    state.phase = CJ4_PHASE_ROUND_END;
    state.round_wind = CJ4_WIND_EAST;
    state.round_end_type = CJ4_ROUND_END_EXHAUSTIVE_DRAW;
    state.current_player = CJ4_PLAYER_3;
    state.dealer = CJ4_PLAYER_3;
    state.honba = 2;
    state.winner_count = 0;
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
    state.current_player = CJ4_PLAYER_0;
    state.dealer = CJ4_PLAYER_1;
    state.draw_tile = draw;
    state.phase = CJ4_PHASE_DRAW;
    state.first_turn_uninterrupted = 0;
    state.draw_turn_count[CJ4_PLAYER_0] = 2;
    state.is_riichi[CJ4_PLAYER_0] = 1;

    assert(cj4_calculate_hand_score(&state, CJ4_PLAYER_0, &rules, &base_score));

    state.riichi_declared_on_first_turn[CJ4_PLAYER_0] = 1;
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
    kazoe_state.current_player = CJ4_PLAYER_0;
    kazoe_state.dealer = CJ4_PLAYER_1;
    kazoe_state.draw_tile = draw;
    kazoe_state.phase = CJ4_PHASE_DRAW;
    kazoe_state.is_riichi[CJ4_PLAYER_0] = 1;
    kazoe_state.riichi_declared_on_first_turn[CJ4_PLAYER_0] = 1;
    kazoe_state.dora_indicators_count = 5;
    kazoe_state.wall[130] = tile(4, 0);
    kazoe_state.wall[128] = tile(4, 1);
    kazoe_state.wall[126] = tile(4, 2);
    kazoe_state.wall[124] = tile(4, 3);
    kazoe_state.wall[122] = tile(4, 0);
    kazoe_state.wall[131] = tile(4, 0);
    kazoe_state.wall[129] = tile(4, 1);
    kazoe_state.wall[127] = tile(4, 2);
    kazoe_state.wall[125] = tile(4, 3);
    kazoe_state.wall[123] = tile(4, 0);

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
    ron_state.phase = CJ4_PHASE_DISCARD;
    ron_state.current_player = CJ4_PLAYER_1;
    ron_state.dealer = CJ4_PLAYER_1;
    ron_state.is_riichi[CJ4_PLAYER_0] = 1;
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
    kokushi_state.phase = CJ4_PHASE_DRAW;
    kokushi_state.current_player = CJ4_PLAYER_0;
    kokushi_state.draw_tile = draw;

    assert(cj4_calculate_hand_score(&kokushi_state, CJ4_PLAYER_0, &zero_rules, &score));
    assert(score.yakuman_count == 2);

    current_rules.kokushi_13_wait_double = 0;
    assert(cj4_calculate_hand_score(&kokushi_state, CJ4_PLAYER_0, &current_rules, &score));
    assert(score.yakuman_count == 1);

    current_rules = cj4_rules_default();
    set_hand(&kazoe_state, CJ4_PLAYER_0, pinfu_hand, (uint8_t)(sizeof(pinfu_hand) / sizeof(pinfu_hand[0])));
    kazoe_state.current_player = CJ4_PLAYER_0;
    kazoe_state.dealer = CJ4_PLAYER_1;
    kazoe_state.draw_tile = pinfu_draw;
    kazoe_state.phase = CJ4_PHASE_DRAW;
    kazoe_state.is_riichi[CJ4_PLAYER_0] = 1;
    kazoe_state.riichi_declared_on_first_turn[CJ4_PLAYER_0] = 1;
    kazoe_state.dora_indicators_count = 5;
    kazoe_state.wall[130] = tile(4, 0);
    kazoe_state.wall[128] = tile(4, 1);
    kazoe_state.wall[126] = tile(4, 2);
    kazoe_state.wall[124] = tile(4, 3);
    kazoe_state.wall[122] = tile(4, 0);
    kazoe_state.wall[131] = tile(4, 0);
    kazoe_state.wall[129] = tile(4, 1);
    kazoe_state.wall[127] = tile(4, 2);
    kazoe_state.wall[125] = tile(4, 3);
    kazoe_state.wall[123] = tile(4, 0);

    assert(cj4_calculate_hand_score(&kazoe_state, CJ4_PLAYER_0, &zero_rules, &score));
    assert(score.yakuman_count == 1);

    current_rules.kazoe_yakuman = 0;
    assert(cj4_calculate_hand_score(&kazoe_state, CJ4_PLAYER_0, &current_rules, &score));
    assert(score.yakuman_count == 0);
    assert(score.han >= 13);
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
    state.current_player = CJ4_PLAYER_0;
    state.dealer = CJ4_PLAYER_1;
    state.draw_tile = draw;
    state.phase = CJ4_PHASE_DRAW;
    state.is_riichi[CJ4_PLAYER_0] = 1;
    state.riichi_declared_on_first_turn[CJ4_PLAYER_0] = 1;

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
    state.current_player = CJ4_PLAYER_0;
    state.dealer = CJ4_PLAYER_1;
    state.draw_tile = draw;
    state.phase = CJ4_PHASE_DRAW;
    state.is_riichi[CJ4_PLAYER_0] = 1;

    won = cj4_do_tsumo(state);
    won.dora_indicators_count = 2;
    won.wall[130] = tile(27, 0);
    won.wall[128] = tile(31, 0);
    won.wall[131] = tile(28, 0);
    won.wall[129] = tile(32, 0);

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
    state.phase = CJ4_PHASE_DISCARD;
    state.current_player = CJ4_PLAYER_0;
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
    state.phase = CJ4_PHASE_DISCARD;
    state.current_player = CJ4_PLAYER_0;
    add_discard(&state, CJ4_PLAYER_0, tile(6, 0));

    won = cj4_do_ron_multi(state, winners, 1, &rules);
    won.dora_indicators_count = 2;
    won.wall[130] = tile(27, 0);
    won.wall[128] = tile(31, 0);
    won.wall[131] = tile(28, 0);
    won.wall[129] = tile(32, 0);

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
    state.phase = CJ4_PHASE_DISCARD;
    state.current_player = CJ4_PLAYER_0;
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

    ron_state.phase = CJ4_PHASE_ROUND_END;
    ron_state.round_end_type = CJ4_ROUND_END_RON;
    ron_state.current_player = CJ4_PLAYER_0;
    ron_state.dealer = CJ4_PLAYER_0;
    ron_state.winner = CJ4_PLAYER_2;
    ron_state.winners[0] = CJ4_PLAYER_2;
    ron_state.winner_count = 1;
    ron_state.loser = CJ4_PLAYER_0;
    ron_state.winning_tile = CJ4_TILE_ID_INVALID;

    settled = cj4_do_settle(ron_state, &rules);

    assert(settled.phase == CJ4_PHASE_SETTLE);
    assert(settled.scores[CJ4_PLAYER_0] == 25000);
    assert(settled.scores[CJ4_PLAYER_1] == 25000);
    assert(settled.scores[CJ4_PLAYER_2] == 25000);
    assert(settled.scores[CJ4_PLAYER_3] == 25000);

    tsumo_state.phase = CJ4_PHASE_ROUND_END;
    tsumo_state.round_end_type = CJ4_ROUND_END_TSUMO;
    tsumo_state.current_player = CJ4_PLAYER_1;
    tsumo_state.dealer = CJ4_PLAYER_0;
    tsumo_state.winner = CJ4_PLAYER_1;
    tsumo_state.winners[0] = CJ4_PLAYER_1;
    tsumo_state.winner_count = 1;
    tsumo_state.draw_tile = CJ4_TILE_ID_INVALID;
    tsumo_state.winning_tile = CJ4_TILE_ID_INVALID;

    settled = cj4_do_settle(tsumo_state, &rules);

    assert(settled.phase == CJ4_PHASE_SETTLE);
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

    state.phase = CJ4_PHASE_DISCARD;
    state.current_player = CJ4_PLAYER_0;
    state.riichi_sticks = 2;
    add_discard(&state, CJ4_PLAYER_0, tile(0, 0));

    next = cj4_do_ron_multi(state, claimers, 3, &rules);

    assert(next.winner_count == 2);
    assert(next.winners[0] == CJ4_PLAYER_1);
    assert(next.winners[1] == CJ4_PLAYER_2);
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
    state.phase = CJ4_PHASE_DRAW;
    state.current_player = CJ4_PLAYER_0;
    state.is_riichi[CJ4_PLAYER_0] = 1;
    state.draw_tile = tile(0, 3);

    assert(cj4_can_ankan_with_tile(
        &state,
        kan_tiles[0],
        kan_tiles[1],
        kan_tiles[2],
        kan_tiles[3]));
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
    state.phase = CJ4_PHASE_DRAW;
    state.current_player = CJ4_PLAYER_0;
    state.draw_tile = draw;
    state.first_turn_uninterrupted = 1;
    state.draw_turn_count[CJ4_PLAYER_0] = 1;

    assert(cj4_can_kyuushu_kyuuhai(&state, &rules));

    next = cj4_do_kyuushu_kyuuhai(state);

    assert(next.phase == CJ4_PHASE_ROUND_END);
    assert(next.round_end_type == CJ4_ROUND_END_ABORTIVE_DRAW);
    assert(next.abortive_draw_reason == CJ4_ABORTIVE_DRAW_KYUUSHU_KYUUHAI);
}

static void
test_suufon_renda_aborts_after_reactions_pass(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong next;

    rules.abortive_suufon_renda = 1;

    state.phase = CJ4_PHASE_DISCARD;
    state.current_player = CJ4_PLAYER_3;
    state.first_turn_uninterrupted = 1;
    add_discard(&state, CJ4_PLAYER_0, tile(27, 0));
    add_discard(&state, CJ4_PLAYER_1, tile(27, 1));
    add_discard(&state, CJ4_PLAYER_2, tile(27, 2));
    add_discard(&state, CJ4_PLAYER_3, tile(27, 3));

    next = cj4_do_pass(state, &rules);

    assert(next.phase == CJ4_PHASE_ROUND_END);
    assert(next.round_end_type == CJ4_ROUND_END_ABORTIVE_DRAW);
    assert(next.abortive_draw_reason == CJ4_ABORTIVE_DRAW_SUUFON_RENDA);
}

static void
test_four_riichi_aborts_after_reactions_pass(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong next;

    rules.abortive_four_riichi = 1;

    state.phase = CJ4_PHASE_DISCARD;
    state.current_player = CJ4_PLAYER_3;
    add_discard(&state, CJ4_PLAYER_3, tile(4, 0));
    for (uint8_t player = 0; player < CJ4_PLAYER_COUNT; ++player)
        state.is_riichi[player] = 1;

    next = cj4_do_pass(state, &rules);

    assert(next.phase == CJ4_PHASE_ROUND_END);
    assert(next.round_end_type == CJ4_ROUND_END_ABORTIVE_DRAW);
    assert(next.abortive_draw_reason == CJ4_ABORTIVE_DRAW_FOUR_RIICHI);
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

    state.phase = CJ4_PHASE_ROUND_END;
    state.round_end_type = CJ4_ROUND_END_EXHAUSTIVE_DRAW;
    state.dealer = CJ4_PLAYER_0;
    state.nagashi_mangan[CJ4_PLAYER_1] = 1;

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
    state.phase = CJ4_PHASE_ROUND_END;
    state.round_end_type = CJ4_ROUND_END_EXHAUSTIVE_DRAW;
    state.dealer = CJ4_PLAYER_0;
    state.nagashi_mangan[CJ4_PLAYER_1] = 1;

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
    state.meld_count[CJ4_PLAYER_2] = 3;
    state.melds[CJ4_PLAYER_2][0] = (cj4_meld){
        .tiles = {haku[0], haku[1], haku[2]},
        .size = 3,
        .type = CJ4_MELD_PON,
        .from_player = CJ4_PLAYER_0,
        .called_index = 0};
    state.melds[CJ4_PLAYER_2][1] = (cj4_meld){
        .tiles = {hatsu[0], hatsu[1], hatsu[2]},
        .size = 3,
        .type = CJ4_MELD_PON,
        .from_player = CJ4_PLAYER_1,
        .called_index = 0};
    state.melds[CJ4_PLAYER_2][2] = (cj4_meld){
        .tiles = {chun[0], chun[1], chun[2]},
        .size = 3,
        .type = CJ4_MELD_PON,
        .from_player = CJ4_PLAYER_1,
        .called_index = 0};

    state.phase = CJ4_PHASE_ROUND_END;
    state.round_end_type = CJ4_ROUND_END_RON;
    state.current_player = CJ4_PLAYER_0;
    state.dealer = CJ4_PLAYER_0;
    state.winner = CJ4_PLAYER_2;
    state.winners[0] = CJ4_PLAYER_2;
    state.winner_count = 1;
    state.loser = CJ4_PLAYER_0;
    state.winning_tile = tile(2, 0);
    state.pao_owner[CJ4_PLAYER_2] = 1;
    state.pao_player[CJ4_PLAYER_2] = CJ4_PLAYER_1;
    state.pao_type[CJ4_PLAYER_2] = CJ4_PAO_DAISANGEN;
    add_discard(&state, CJ4_PLAYER_0, state.winning_tile);
    state.phase = CJ4_PHASE_ROUND_END;

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
    state.meld_count[CJ4_PLAYER_2] = 3;
    state.melds[CJ4_PLAYER_2][0] = (cj4_meld){
        .tiles = {haku[0], haku[1], haku[2]},
        .size = 3,
        .type = CJ4_MELD_PON,
        .from_player = CJ4_PLAYER_0,
        .called_index = 0};
    state.melds[CJ4_PLAYER_2][1] = (cj4_meld){
        .tiles = {hatsu[0], hatsu[1], hatsu[2]},
        .size = 3,
        .type = CJ4_MELD_PON,
        .from_player = CJ4_PLAYER_1,
        .called_index = 0};
    state.melds[CJ4_PLAYER_2][2] = (cj4_meld){
        .tiles = {chun[0], chun[1], chun[2]},
        .size = 3,
        .type = CJ4_MELD_PON,
        .from_player = CJ4_PLAYER_1,
        .called_index = 0};
    state.phase = CJ4_PHASE_DRAW;
    state.current_player = CJ4_PLAYER_2;
    state.dealer = CJ4_PLAYER_0;
    state.draw_tile = draw;
    state.pao_owner[CJ4_PLAYER_2] = 1;
    state.pao_player[CJ4_PLAYER_2] = CJ4_PLAYER_1;
    state.pao_type[CJ4_PLAYER_2] = CJ4_PAO_DAISANGEN;

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

    state.phase = CJ4_PHASE_DRAW;
    state.current_player = winner;
    state.dealer = CJ4_PLAYER_0;
    state.draw_tile = draw;
    set_hand(&state, winner, pair, 2);
    state.meld_count[winner] = 4;
    state.melds[winner][0] = (cj4_meld){
        .tiles = {tile(27, 0), tile(27, 1), tile(27, 2)},
        .size = 3,
        .type = CJ4_MELD_PON,
        .from_player = CJ4_PLAYER_0,
        .called_index = 0};
    state.melds[winner][1] = (cj4_meld){
        .tiles = {tile(28, 0), tile(28, 1), tile(28, 2)},
        .size = 3,
        .type = CJ4_MELD_PON,
        .from_player = CJ4_PLAYER_1,
        .called_index = 0};
    state.melds[winner][2] = (cj4_meld){
        .tiles = {tile(29, 0), tile(29, 1), tile(29, 2)},
        .size = 3,
        .type = CJ4_MELD_PON,
        .from_player = CJ4_PLAYER_1,
        .called_index = 0};
    state.melds[winner][3] = (cj4_meld){
        .tiles = {tile(30, 0), tile(30, 1), tile(30, 2)},
        .size = 3,
        .type = CJ4_MELD_PON,
        .from_player = CJ4_PLAYER_1,
        .called_index = 0};
    state.pao_owner[winner] = 1;
    state.pao_player[winner] = CJ4_PLAYER_1;
    state.pao_type[winner] = CJ4_PAO_DAISUUSHII;

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

    state.phase = CJ4_PHASE_ROUND_END;
    state.round_end_type = CJ4_ROUND_END_RON;
    state.current_player = CJ4_PLAYER_0;
    state.dealer = CJ4_PLAYER_0;
    state.winner = winner;
    state.winners[0] = winner;
    state.winner_count = 1;
    state.loser = CJ4_PLAYER_0;
    state.winning_tile = tile(4, 0);
    state.pao_owner[winner] = 1;
    state.pao_player[winner] = CJ4_PLAYER_1;
    state.pao_type[winner] = CJ4_PAO_SUUKANTSU;
    add_discard(&state, CJ4_PLAYER_0, state.winning_tile);
    state.phase = CJ4_PHASE_ROUND_END;
    set_hand(&state, winner, (const cj4_tile_id[]){tile(4, 1)}, 1);
    state.meld_count[winner] = 4;
    for (uint8_t i = 0; i < 4; ++i)
    {
        state.melds[winner][i] = (cj4_meld){
            .tiles = {tile((cj4_tile_type)i, 0), tile((cj4_tile_type)i, 1), tile((cj4_tile_type)i, 2), tile((cj4_tile_type)i, 3)},
            .size = 4,
            .type = CJ4_MELD_MINKAN,
            .from_player = CJ4_PLAYER_1,
            .called_index = 0};
    }

    settled = cj4_do_settle(state, &rules);
    assert(settled.scores[CJ4_PLAYER_1] == 25000);
}

int
main(
    void)
{
    test_rules_default_and_validate();
    test_tenhou_preset_fields();
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
    test_chankan_pass_records_furiten();
    test_claim_tile_arguments_must_be_distinct();
    test_kuikae_forbidden_after_pon_and_chi();
    test_kan_flow_aborts_on_fourth_kakan_without_noten_penalty();
    test_kan_flow_aborts_on_fourth_ankan();
    test_kan_dora_timing_for_ankan_and_kakan();
    test_kan_requires_a_live_wall_tile();
    test_consecutive_kakan_reveals_previous_dora_before_rinshan();
    test_minkan_then_ankan_preserves_pending_dora();
    test_previous_kan_dora_counts_on_following_rinshan_tsumo();
    test_pending_kan_dora_is_discarded_on_win_and_capped();
    test_kokushi_ron_on_ankan_rule();
    test_ankan_kokushi_ron_leaves_no_committed_kan();
    test_exhaustive_draw_uses_shape_tenpai();
    test_exhaustive_draw_uses_live_wall_not_discard_count();
    test_last_discard_cannot_be_called();
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
    test_double_riichi_is_always_enabled();
    test_score_rules_control_kazoe_and_kiriage();
    test_zero_initialized_rules_keep_v1_score_compatibility();
    test_collect_winning_results_returns_tsumo_details();
    test_collect_winning_results_exposes_dora_indicators();
    test_collect_winning_results_returns_ron_details();
    test_collect_winning_results_hides_ura_dora_without_riichi();
    test_collect_winning_results_returns_multi_ron_details();
    test_release_settle_skips_uncalculable_win_scores();
    test_max_ron_players_uses_head_bump_order();
    test_triple_ron_abortive_draw_rule();
    test_riichi_ankan_keeps_waits();
    test_kyuushu_kyuuhai_aborts_on_first_draw();
    test_suufon_renda_aborts_after_reactions_pass();
    test_four_riichi_aborts_after_reactions_pass();
    test_nagashi_mangan_settles_as_mangan_tsumo();
    test_tenhou_nagashi_mangan_uses_dealer_tenpai_for_renchan();
    test_pao_splits_ron_payment_with_responsible_player();
    test_pao_liability_only_splits_compound_yakuman_tsumo();
    test_daisuushii_double_pao_liability_uses_two_yakuman();
    test_pao_type_flags_disable_suukantsu_liability();
    manager_tests_main();
    return 0;
}
