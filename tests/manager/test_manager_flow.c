#include <assert.h>
#include <string.h>

#include "cjong4/core/action.h"
#include "cjong4/core/state.h"
#include "cjong4/core/state_query.h"
#include "cjong4/core/tile.h"
#include "cjong4/core/wind.h"
#include "cjong4/manager/manager.h"

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
contains_tile(
    const cj4_tile_id *tiles,
    uint8_t count,
    cj4_tile_id tile_id)
{
    for (uint8_t i = 0; i < count; ++i)
    {
        if (tiles[i] == tile_id)
            return 1;
    }

    return 0;
}

static uint8_t
contains_action_type(
    const cj4_action *actions,
    uint8_t count,
    cj4_action_type type)
{
    for (uint8_t i = 0; i < count; ++i)
    {
        if (actions[i].type == type)
            return 1;
    }

    return 0;
}

typedef struct
{
    cj4_action_type preferred;
    uint8_t call_count;
    uint8_t last_action_count;
    cj4_player last_view_player;
} chooser_ctx;

static cj4_action
choose_preferred_action(
    void *opaque,
    const cj4_player_view *view,
    const cj4_action *actions,
    uint8_t action_count)
{
    chooser_ctx *ctx = (chooser_ctx *)opaque;

    ctx->call_count++;
    ctx->last_action_count = action_count;
    ctx->last_view_player = view->player;

    for (uint8_t i = 0; i < action_count; ++i)
    {
        if (actions[i].type == ctx->preferred)
            return actions[i];
    }

    return actions[0];
}

static cj4m_player_delegate
make_delegate(chooser_ctx *ctx, cj4_action_type preferred)
{
    ctx->preferred = preferred;
    ctx->call_count = 0;
    ctx->last_action_count = 0;
    ctx->last_view_player = CJ4_PLAYER_COUNT;

    return (cj4m_player_delegate){
        .ctx = ctx,
        .decide = choose_preferred_action};
}

static void
test_player_view_hides_hidden_information(void)
{
    cj4_mahjong state = make_empty_state();
    const cj4_tile_id hand0[] = {
        tile(0, 0), tile(1, 0), tile(2, 0), tile(3, 0)};
    const cj4_tile_id hand1[] = {
        tile(10, 0), tile(10, 1), tile(10, 2)};
    cj4_player_view view;

    set_hand(&state, CJ4_PLAYER_0, hand0, 4);
    set_hand(&state, CJ4_PLAYER_1, hand1, 3);
    state.current_player = CJ4_PLAYER_0;
    state.phase = CJ4_PHASE_DRAW;
    state.draw_tile = hand0[3];
    state.is_riichi[CJ4_PLAYER_1] = 1;
    state.temporary_furiten[CJ4_PLAYER_0] = 1;
    add_discard(&state, CJ4_PLAYER_2, tile(20, 0));

    view = cj4m_make_player_view(&state, CJ4_PLAYER_0);

    assert(view.player == CJ4_PLAYER_0);
    assert(view.hand_count == 4);
    assert(contains_tile(view.hand, view.hand_count, hand0[0]));
    assert(contains_tile(view.hand, view.hand_count, hand0[3]));
    assert(!contains_tile(view.hand, view.hand_count, hand1[0]));
    assert(view.draw_tile == hand0[3]);
    assert(view.last_discard == tile(20, 0));
    assert(view.is_riichi[CJ4_PLAYER_1] == 1);
    assert(view.temporary_furiten == 1);
}

static void
test_collect_actions_includes_pass_and_claims(void)
{
    cj4_mahjong state = make_empty_state();
    const cj4_tile_id chi_hand[] = {
        tile(1, 0), tile(2, 0)};
    const cj4_tile_id pon_hand[] = {
        tile(3, 1), tile(3, 2)};
    cj4_action actions[CJ4M_MAX_ACTIONS];
    uint8_t action_count;

    set_hand(&state, CJ4_PLAYER_1, chi_hand, 2);
    set_hand(&state, CJ4_PLAYER_2, pon_hand, 2);
    state.phase = CJ4_PHASE_DISCARD;
    state.current_player = CJ4_PLAYER_0;
    add_discard(&state, CJ4_PLAYER_0, tile(3, 0));

    action_count = cj4m_collect_actions(
        &state,
        NULL,
        CJ4_PLAYER_1,
        actions,
        CJ4M_MAX_ACTIONS);
    assert(contains_action_type(actions, action_count, CJ4_ACTION_PASS));
    assert(contains_action_type(actions, action_count, CJ4_ACTION_CHI));

    action_count = cj4m_collect_actions(
        &state,
        NULL,
        CJ4_PLAYER_2,
        actions,
        CJ4M_MAX_ACTIONS);
    assert(contains_action_type(actions, action_count, CJ4_ACTION_PASS));
    assert(contains_action_type(actions, action_count, CJ4_ACTION_PON));
}

static void
test_step_uses_delegate_for_draw_phase(void)
{
    cj4_mahjong state = make_empty_state();
    chooser_ctx contexts[CJ4_PLAYER_COUNT];
    cj4m_player_delegate delegates[CJ4_PLAYER_COUNT];
    cj4_mahjong next;
    cj4_tile_id draw = tile(26, 0);
    const cj4_tile_id hand[] = {
        tile(0, 0), tile(1, 0), tile(2, 0),
        tile(9, 0), tile(10, 0), tile(11, 0),
        tile(18, 0), tile(19, 0), tile(20, 0),
        tile(3, 0), tile(4, 0),
        tile(15, 0), tile(15, 1),
        draw};

    for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
        delegates[i] = make_delegate(&contexts[i], CJ4_ACTION_PASS);

    delegates[CJ4_PLAYER_0] = make_delegate(&contexts[CJ4_PLAYER_0], CJ4_ACTION_RIICHI);

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    state.current_player = CJ4_PLAYER_0;
    state.phase = CJ4_PHASE_DRAW;
    state.draw_tile = draw;

    next = cj4m_step(&state, NULL, delegates);

    assert(next.phase == CJ4_PHASE_DISCARD);
    assert(next.is_riichi[CJ4_PLAYER_0] == 1);
    assert(contexts[CJ4_PLAYER_0].call_count == 1);
    assert(contexts[CJ4_PLAYER_0].last_view_player == CJ4_PLAYER_0);
}

static void
test_step_prioritizes_pon_over_chi(void)
{
    cj4_mahjong state = make_empty_state();
    chooser_ctx contexts[CJ4_PLAYER_COUNT];
    cj4m_player_delegate delegates[CJ4_PLAYER_COUNT];
    cj4_mahjong next;
    const cj4_tile_id chi_hand[] = {
        tile(1, 0), tile(2, 0)};
    const cj4_tile_id pon_hand[] = {
        tile(3, 1), tile(3, 2)};

    for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
        delegates[i] = make_delegate(&contexts[i], CJ4_ACTION_PASS);

    delegates[CJ4_PLAYER_1] = make_delegate(&contexts[CJ4_PLAYER_1], CJ4_ACTION_CHI);
    delegates[CJ4_PLAYER_2] = make_delegate(&contexts[CJ4_PLAYER_2], CJ4_ACTION_PON);

    set_hand(&state, CJ4_PLAYER_1, chi_hand, 2);
    set_hand(&state, CJ4_PLAYER_2, pon_hand, 2);
    state.phase = CJ4_PHASE_DISCARD;
    state.current_player = CJ4_PLAYER_0;
    add_discard(&state, CJ4_PLAYER_0, tile(3, 0));

    next = cj4m_step(&state, NULL, delegates);

    assert(next.phase == CJ4_PHASE_AFTER_CALL);
    assert(next.current_player == CJ4_PLAYER_2);
    assert(next.meld_count[CJ4_PLAYER_2] == 1);
    assert(next.melds[CJ4_PLAYER_2][0].type == CJ4_MELD_PON);
    assert(contexts[CJ4_PLAYER_1].call_count == 1);
    assert(contexts[CJ4_PLAYER_2].call_count == 1);
}

static void
test_step_prioritizes_ron_and_respects_limit(void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    chooser_ctx contexts[CJ4_PLAYER_COUNT];
    cj4m_player_delegate delegates[CJ4_PLAYER_COUNT];
    cj4_mahjong next;
    const cj4_tile_id chi_hand[] = {
        tile(4, 0), tile(5, 0)};
    const cj4_tile_id ron_hand[] = {
        tile(1, 0), tile(2, 0), tile(3, 0),
        tile(10, 0), tile(11, 0), tile(12, 0),
        tile(19, 0), tile(20, 0), tile(21, 0),
        tile(4, 1), tile(5, 1),
        tile(14, 0), tile(14, 1)};
    const cj4_tile_id ron_hand_alt[] = {
        tile(1, 1), tile(2, 1), tile(3, 1),
        tile(10, 1), tile(11, 1), tile(12, 1),
        tile(19, 1), tile(20, 1), tile(21, 1),
        tile(4, 2), tile(5, 2),
        tile(14, 2), tile(14, 3)};

    for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
        delegates[i] = make_delegate(&contexts[i], CJ4_ACTION_PASS);

    delegates[CJ4_PLAYER_1] = make_delegate(&contexts[CJ4_PLAYER_1], CJ4_ACTION_CHI);
    delegates[CJ4_PLAYER_2] = make_delegate(&contexts[CJ4_PLAYER_2], CJ4_ACTION_RON);
    delegates[CJ4_PLAYER_3] = make_delegate(&contexts[CJ4_PLAYER_3], CJ4_ACTION_RON);

    rules.kuitan = 1;
    rules.max_ron_players = 1;

    set_hand(&state, CJ4_PLAYER_1, chi_hand, 2);
    set_hand(&state, CJ4_PLAYER_2, ron_hand, (uint8_t)(sizeof(ron_hand) / sizeof(ron_hand[0])));
    set_hand(&state, CJ4_PLAYER_3, ron_hand_alt, (uint8_t)(sizeof(ron_hand_alt) / sizeof(ron_hand_alt[0])));
    state.phase = CJ4_PHASE_DISCARD;
    state.current_player = CJ4_PLAYER_0;
    add_discard(&state, CJ4_PLAYER_0, tile(6, 0));

    next = cj4m_step(&state, &rules, delegates);

    assert(next.phase == CJ4_PHASE_ROUND_END);
    assert(next.round_end_type == CJ4_ROUND_END_RON);
    assert(next.winner_count == 1);
    assert(next.winners[0] == CJ4_PLAYER_2);
    assert(contexts[CJ4_PLAYER_1].call_count == 1);
    assert(contexts[CJ4_PLAYER_2].call_count == 1);
    assert(contexts[CJ4_PLAYER_3].call_count == 1);
}

static void
test_step_advances_after_all_pass(void)
{
    cj4_mahjong state = make_empty_state();
    chooser_ctx contexts[CJ4_PLAYER_COUNT];
    cj4m_player_delegate delegates[CJ4_PLAYER_COUNT];
    cj4_mahjong next;
    cj4_tile_id next_draw = tile(30, 0);

    for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
        delegates[i] = make_delegate(&contexts[i], CJ4_ACTION_PASS);

    state.phase = CJ4_PHASE_DISCARD;
    state.current_player = CJ4_PLAYER_0;
    state.wall[0] = next_draw;
    state.wall_pos = 0;
    add_discard(&state, CJ4_PLAYER_0, tile(8, 0));

    next = cj4m_step(&state, NULL, delegates);

    assert(next.phase == CJ4_PHASE_DRAW);
    assert(next.current_player == CJ4_PLAYER_1);
    assert(next.draw_tile == next_draw);
    assert(contexts[CJ4_PLAYER_1].call_count == 1);
    assert(contexts[CJ4_PLAYER_2].call_count == 1);
    assert(contexts[CJ4_PLAYER_3].call_count == 1);
}

int manager_tests_main(void)
{
    test_player_view_hides_hidden_information();
    test_collect_actions_includes_pass_and_claims();
    test_step_uses_delegate_for_draw_phase();
    test_step_prioritizes_pon_over_chi();
    test_step_prioritizes_ron_and_respects_limit();
    test_step_advances_after_all_pass();
    return 0;
}
