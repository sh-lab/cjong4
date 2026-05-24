#include <assert.h>
#include <string.h>

#include "../src/state_score.h"
#include "state.h"
#include "state_init.h"
#include "state_kan.h"
#include "state_pass.h"
#include "state_riichi.h"
#include "state_ron.h"
#include "state_settle.h"

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

static void
test_riichi_uses_shape_tenpai(void)
{
    cj4_mahjong state = make_empty_state();
    cj4_tile_id draw = tile(26, 0);
    const cj4_tile_id hand[] = {
        tile(0, 0), tile(1, 0), tile(2, 0),
        tile(9, 0), tile(10, 0), tile(11, 0),
        tile(18, 0), tile(19, 0), tile(20, 0),
        tile(3, 0), tile(4, 0),
        tile(15, 0), tile(15, 1),
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
        tile(1, 0), tile(2, 0), tile(3, 0),
        tile(10, 0), tile(11, 0), tile(12, 0),
        tile(19, 0), tile(20, 0), tile(21, 0),
        tile(4, 0), tile(5, 0),
        tile(14, 0), tile(14, 1)};

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
        tile(1, 0), tile(2, 0), tile(3, 0),
        tile(10, 0), tile(11, 0), tile(12, 0),
        tile(19, 0), tile(20, 0), tile(21, 0),
        tile(4, 0), tile(5, 0),
        tile(14, 0), tile(14, 1)};

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
        tile(1, 0), tile(2, 0), tile(3, 0),
        tile(10, 0), tile(11, 0), tile(12, 0),
        tile(19, 0), tile(20, 0), tile(21, 0),
        tile(4, 0), tile(5, 0),
        tile(14, 0), tile(14, 1)};

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

    round_end = cj4_do_rinshan_draw(state);
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
test_kan_flow_aborts_on_fourth_ankan(void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong round_end;
    const cj4_tile_id ankan_tiles[] = {
        tile(0, 0), tile(0, 1), tile(0, 2), tile(0, 3)};
    const cj4_tile_id hand[] = {
        tile(0, 0), tile(0, 1), tile(0, 2), tile(0, 3),
        tile(10, 0), tile(11, 0), tile(12, 0),
        tile(19, 0), tile(20, 0), tile(21, 0),
        tile(4, 0), tile(5, 0),
        tile(14, 0), tile(14, 1)};

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
        ankan_tiles[3],
        &rules);

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
        tile(0, 0), tile(1, 0), tile(2, 0),
        tile(9, 0), tile(10, 0), tile(11, 0),
        tile(18, 0), tile(19, 0), tile(20, 0),
        tile(3, 0), tile(4, 0),
        tile(15, 0), tile(15, 1)};

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
test_double_riichi_is_always_enabled(void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_hand_score score;
    cj4_hand_score base_score;
    cj4_tile_id draw = tile(5, 0);
    const cj4_tile_id hand[] = {
        tile(0, 0), tile(1, 0), tile(2, 0),
        tile(9, 0), tile(10, 0), tile(11, 0),
        tile(18, 0), tile(19, 0), tile(20, 0),
        tile(3, 0), tile(4, 0),
        tile(15, 0), tile(15, 1),
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
        tile(0, 0), tile(0, 1), tile(0, 2), tile(0, 3)};
    const cj4_tile_id hand[] = {
        tile(0, 0), tile(0, 1), tile(0, 2), tile(0, 3),
        tile(10, 0), tile(11, 0), tile(12, 0),
        tile(19, 0), tile(20, 0), tile(21, 0),
        tile(4, 0), tile(5, 0),
        tile(15, 0), tile(15, 1)};

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

int main(void)
{
    test_riichi_uses_shape_tenpai();
    test_temporary_furiten_blocks_ron();
    test_riichi_furiten_is_recorded();
    test_permanent_furiten_blocks_other_wait();
    test_kan_flow_aborts_on_fourth_kakan_without_noten_penalty();
    test_kan_flow_aborts_on_fourth_ankan();
    test_exhaustive_draw_uses_shape_tenpai();
    test_double_riichi_is_always_enabled();
    test_max_ron_players_uses_head_bump_order();
    test_riichi_ankan_keeps_waits();
    return 0;
}
