#include <assert.h>
#include <string.h>

#include "../../src/core/hand_check.h"
#include "../../src/core/state_score.h"
#include "cjong4/core/state.h"
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
tile(cj4_tile_type type, uint8_t index)
{
    return cj4_tile_make(type, index);
}

static cj4_mahjong
make_empty_state(void)
{
    cj4_mahjong state;

    memset(&state, 0, sizeof(state));
    state.phase = CJ4_PHASE_DRAW;
    state.dealer = CJ4_PLAYER_0;
    state.current_player = CJ4_PLAYER_0;
    state.round_wind = CJ4_WIND_EAST;
    state.draw_tile = CJ4_TILE_ID_INVALID;
    state.pending_kakan_tile = CJ4_TILE_ID_INVALID;
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
test_riichi_uses_shape_tenpai(void)
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
test_temporary_furiten_blocks_ron(void)
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
test_riichi_furiten_is_recorded(void)
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
test_permanent_furiten_blocks_other_wait(void)
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
test_kan_flow_aborts_on_fourth_kakan_without_noten_penalty(void)
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
test_riichi_restricts_actions(void)
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
test_seven_pairs_rejects_quad(void)
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
test_chankan_pass_records_furiten(void)
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
test_claim_tile_arguments_must_be_distinct(void)
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
test_kan_flow_aborts_on_fourth_ankan(void)
{
    cj4_rules rules = {0};
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

    assert(round_end.phase == CJ4_PHASE_ROUND_END);
    assert(round_end.round_end_type == CJ4_ROUND_END_ABORTIVE_DRAW);
    assert(round_end.draw_tile == CJ4_TILE_ID_INVALID);
    assert(round_end.dead_wall_draw_count == 0);
}

static void
test_exhaustive_draw_uses_shape_tenpai(void)
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
test_exhaustive_draw_resets_honba_when_dealer_is_noten(void)
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
test_tonpuu_enters_south_when_target_is_not_reached(void)
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
test_hanchan_enters_west_when_target_is_not_reached(void)
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
test_tonpuu_ends_when_target_is_reached(void)
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
test_tonpuu_does_not_end_before_east_4_when_non_dealer_reaches_target(void)
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
test_tonpuu_does_not_end_before_east_4_when_dealer_renchan_is_top(void)
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
test_tonpuu_ends_when_score_matches_target(void)
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
test_tonpuu_ends_when_dealer_is_top_and_matches_target(void)
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
test_tonpuu_continues_when_dealer_renchan_is_not_top(void)
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
test_tonpuu_ending_on_dealer_tenpai_draw_does_not_increase_honba(void)
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
test_double_riichi_is_always_enabled(void)
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
test_collect_winning_results_returns_tsumo_details(void)
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
test_collect_winning_results_exposes_dora_indicators(void)
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
test_collect_winning_results_returns_ron_details(void)
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
test_collect_winning_results_hides_ura_dora_without_riichi(void)
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
test_collect_winning_results_returns_multi_ron_details(void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong won;
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
}

static void
test_max_ron_players_uses_head_bump_order(void)
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
test_riichi_ankan_keeps_waits(void)
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

int
main(void)
{
    test_riichi_uses_shape_tenpai();
    test_temporary_furiten_blocks_ron();
    test_riichi_furiten_is_recorded();
    test_permanent_furiten_blocks_other_wait();
    test_riichi_restricts_actions();
    test_seven_pairs_rejects_quad();
    test_chankan_pass_records_furiten();
    test_claim_tile_arguments_must_be_distinct();
    test_kan_flow_aborts_on_fourth_kakan_without_noten_penalty();
    test_kan_flow_aborts_on_fourth_ankan();
    test_exhaustive_draw_uses_shape_tenpai();
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
    test_collect_winning_results_returns_tsumo_details();
    test_collect_winning_results_exposes_dora_indicators();
    test_collect_winning_results_returns_ron_details();
    test_collect_winning_results_hides_ura_dora_without_riichi();
    test_collect_winning_results_returns_multi_ron_details();
    test_max_ron_players_uses_head_bump_order();
    test_riichi_ankan_keeps_waits();
    manager_tests_main();
    return 0;
}
