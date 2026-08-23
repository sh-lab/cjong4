#include <assert.h>
#include <string.h>

#include "../../src/core/state_ops.h"
#include "../test_support.h"
#include "cjong4/core/action.h"
#include "cjong4/core/rules.h"
#include "cjong4/core/state.h"
#include "cjong4/core/state_kan.h"
#include "cjong4/core/state_query.h"
#include "cjong4/core/state_tsumo.h"
#include "cjong4/core/tile.h"
#include "cjong4/core/wind.h"
#include "cjong4/manager/manager.h"

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

static uint8_t
contains_action(
    const cj4_action *actions,
    uint8_t count,
    cj4_action_type type,
    cj4_tile_id tile)
{
    for (uint8_t i = 0; i < count; ++i)
    {
        if (actions[i].type == type && actions[i].tile == tile)
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

typedef struct
{
    uint8_t choose_tsumo;
    uint8_t choose_minkan;
    cj4_tile_id discard;
    uint8_t call_count;
    uint8_t dora_count[3];
    cj4_tile_id dora_indicators[3][CJ4_MAX_DORA_INDICATORS];
    uint8_t saw_pass[3];
    uint8_t saw_tsumo[3];
    uint8_t saw_discard[3];
    uint8_t saw_minkan[3];
} pending_kan_dora_ctx;

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
make_delegate(
    chooser_ctx *ctx,
    cj4_action_type preferred)
{
    ctx->preferred = preferred;
    ctx->call_count = 0;
    ctx->last_action_count = 0;
    ctx->last_view_player = CJ4_PLAYER_COUNT;

    return (cj4m_player_delegate){
        .ctx = ctx,
        .decide = choose_preferred_action};
}

static cj4_action
choose_invalid_action(
    void *opaque,
    const cj4_player_view *view,
    const cj4_action *actions,
    uint8_t action_count)
{
    cj4_action action;

    (void)opaque;
    (void)view;
    (void)actions;
    (void)action_count;
    memset(&action, 0, sizeof(action));
    action.type = CJ4_ACTION_DISCARD;
    action.tile = CJ4_TILE_ID_INVALID;
    return action;
}

static cj4_action
choose_pending_kan_dora_action(
    void *opaque,
    const cj4_player_view *view,
    const cj4_action *actions,
    uint8_t action_count)
{
    pending_kan_dora_ctx *ctx = (pending_kan_dora_ctx *)opaque;
    uint8_t call = ctx->call_count++;
    cj4_dora_indicator_list indicators =
        cj4_location_collect_dora_indicators(view->locations);

    assert(call < 3);
    ctx->dora_count[call] = indicators.count;
    memcpy(
        ctx->dora_indicators[call],
        indicators.items,
        sizeof(ctx->dora_indicators[call]));

    for (uint8_t i = 0; i < action_count; ++i)
    {
        if (actions[i].type == CJ4_ACTION_PASS)
            ctx->saw_pass[call] = 1;
        if (actions[i].type == CJ4_ACTION_TSUMO)
            ctx->saw_tsumo[call] = 1;
        if (actions[i].type == CJ4_ACTION_DISCARD)
            ctx->saw_discard[call] = 1;
        if (actions[i].type == CJ4_ACTION_MINKAN)
            ctx->saw_minkan[call] = 1;
    }

    if (ctx->choose_minkan)
    {
        for (uint8_t i = 0; i < action_count; ++i)
        {
            if (actions[i].type == CJ4_ACTION_MINKAN)
                return actions[i];
        }
    }

    if (ctx->choose_tsumo)
    {
        for (uint8_t i = 0; i < action_count; ++i)
        {
            if (actions[i].type == CJ4_ACTION_TSUMO)
                return actions[i];
        }
    }

    for (uint8_t i = 0; i < action_count; ++i)
    {
        if (actions[i].type == CJ4_ACTION_PASS)
            return actions[i];
    }

    for (uint8_t i = 0; i < action_count; ++i)
    {
        if (actions[i].type == CJ4_ACTION_DISCARD &&
            actions[i].tile == ctx->discard)
        {
            return actions[i];
        }
    }

    return actions[0];
}

static cj4_mahjong
make_pending_kan_dora_win_state(
    const cj4_rules *rules)
{
    cj4_mahjong state = make_empty_state();
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

    set_hand(
        &state,
        CJ4_PLAYER_0,
        hand,
        (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_KAKAN_RESOLVE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.dealer = CJ4_PLAYER_1;
    state.pending_kakan_tile = tile(6, 0);
    state.dora_count = 1;
    set_test_wall(&state, (uint8_t)(130), tile(30, 0));
    set_test_wall(&state, (uint8_t)(128), tile(4, 1));
    set_test_wall(&state, (uint8_t)(134), tile(5, 0));

    return cj4_do_rinshan_draw(state, rules);
}

static void
test_step_replaces_invalid_delegate_action(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_mahjong next;
    chooser_ctx contexts[CJ4_PLAYER_COUNT];
    cj4m_player_delegate delegates[CJ4_PLAYER_COUNT];
    const cj4_tile_id hand[] = {tile(0, 0), tile(1, 0)};

    for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
        delegates[i] = make_delegate(&contexts[i], CJ4_ACTION_PASS);
    delegates[CJ4_PLAYER_0].ctx = NULL;
    delegates[CJ4_PLAYER_0].decide = choose_invalid_action;

    set_hand(&state, CJ4_PLAYER_0, hand, 2);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    state.draw_tile = hand[1];

    next = cj4m_step(&state, NULL, delegates);

    assert(cj4_state_phase(&next) == CJ4_PHASE_DISCARD);
    assert(next.discard_count == 1);
    assert(cj4_tile_id_is_valid(next.last_discard_tile));
}

static void
test_step_reveals_pending_kan_dora_after_single_delegate_call(
    void)
{
    cj4_rules rules = cj4_rules_tenhou();
    cj4_mahjong state = make_pending_kan_dora_win_state(&rules);
    cj4_mahjong next;
    chooser_ctx other_contexts[CJ4_PLAYER_COUNT];
    pending_kan_dora_ctx context;
    cj4m_player_delegate delegates[CJ4_PLAYER_COUNT];

    memset(&context, 0, sizeof(context));
    context.discard = state.draw_tile;

    for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
        delegates[i] = make_delegate(&other_contexts[i], CJ4_ACTION_PASS);
    delegates[CJ4_PLAYER_0] = (cj4m_player_delegate){
        .ctx = &context,
        .decide = choose_pending_kan_dora_action};

    assert(cj4_state_phase(&state) == CJ4_PHASE_DRAW);
    assert(state.dora_count == 1);
    assert(state.pending_kan_dora_count == 1);
    assert(cj4_can_tsumo(&state, &rules));

    next = cj4m_step(&state, &rules, delegates);

    assert(context.call_count == 1);
    assert(!context.saw_pass[0]);
    assert(context.saw_tsumo[0]);
    assert(context.saw_discard[0]);
    assert(context.dora_count[0] == 1);
    assert(context.dora_indicators[0][0] == tile(30, 0));
    assert(cj4_state_phase(&next) == CJ4_PHASE_DISCARD);
    assert(next.dora_count == 2);
    assert(next.pending_kan_dora_count == 0);
}

static void
test_step_does_not_reveal_current_kan_dora_on_rinshan_tsumo(
    void)
{
    cj4_rules rules = cj4_rules_tenhou();
    cj4_mahjong state = make_pending_kan_dora_win_state(&rules);
    cj4_mahjong next;
    chooser_ctx other_contexts[CJ4_PLAYER_COUNT];
    pending_kan_dora_ctx context;
    cj4m_player_delegate delegates[CJ4_PLAYER_COUNT];

    memset(&context, 0, sizeof(context));
    context.choose_tsumo = 1;

    for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
        delegates[i] = make_delegate(&other_contexts[i], CJ4_ACTION_PASS);
    delegates[CJ4_PLAYER_0] = (cj4m_player_delegate){
        .ctx = &context,
        .decide = choose_pending_kan_dora_action};

    next = cj4m_step(&state, &rules, delegates);

    assert(context.call_count == 1);
    assert(!context.saw_pass[0]);
    assert(context.saw_tsumo[0]);
    assert(context.saw_discard[0]);
    assert(context.dora_count[0] == 1);
    assert(cj4_state_phase(&next) == CJ4_PHASE_ROUND_END);
    assert(cj4_state_round_end_type(&next) == CJ4_ROUND_END_TSUMO);
    assert(next.dora_count == 1);
    assert(next.pending_kan_dora_count == 0);
}

static void
test_step_minkan_flow_reveals_dora_after_discard_choice(
    void)
{
    cj4_rules rules = cj4_rules_tenhou();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong after_minkan;
    cj4_mahjong after_discard;
    chooser_ctx other_contexts[CJ4_PLAYER_COUNT];
    pending_kan_dora_ctx context;
    cj4m_player_delegate delegates[CJ4_PLAYER_COUNT];
    const cj4_tile_id hand[] = {
        tile(27, 0),
        tile(27, 1),
        tile(27, 2),
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(15, 0),
        tile(15, 1)};

    memset(&context, 0, sizeof(context));
    context.choose_minkan = 1;
    context.discard = tile(20, 0);

    for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
        delegates[i] = make_delegate(&other_contexts[i], CJ4_ACTION_PASS);
    delegates[CJ4_PLAYER_1] = (cj4m_player_delegate){
        .ctx = &context,
        .decide = choose_pending_kan_dora_action};

    set_hand(
        &state,
        CJ4_PLAYER_1,
        hand,
        (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.dora_count = 1;
    set_test_wall(&state, (uint8_t)(130), tile(30, 0));
    set_test_wall(&state, (uint8_t)(128), tile(4, 1));
    set_test_wall(&state, (uint8_t)(134), tile(20, 0));
    add_discard(&state, CJ4_PLAYER_0, tile(27, 3));

    after_minkan = cj4m_step(&state, &rules, delegates);

    assert(context.call_count == 1);
    assert(context.saw_minkan[0]);
    assert(context.dora_count[0] == 1);
    assert(cj4_state_phase(&after_minkan) == CJ4_PHASE_DRAW);
    assert(cj4_state_current_player(&after_minkan) == CJ4_PLAYER_1);
    cj4_meld_list minkan_melds =
        cj4_location_collect_melds(after_minkan.locations, CJ4_PLAYER_1);
    assert(minkan_melds.count == 1);
    assert(minkan_melds.items[0].type == CJ4_MELD_MINKAN);

    assert(context.call_count == 1);
    assert(after_minkan.draw_tile == tile(20, 0));
    assert(after_minkan.dora_count == 1);
    assert(after_minkan.pending_kan_dora_count == 1);
    assert(cj4_can_tsumo(&after_minkan, &rules));

    after_discard = cj4m_step(&after_minkan, &rules, delegates);

    assert(context.call_count == 2);
    assert(!context.saw_pass[1]);
    assert(context.saw_tsumo[1]);
    assert(context.saw_discard[1]);
    assert(context.dora_count[1] == 1);
    assert(cj4_state_phase(&after_discard) == CJ4_PHASE_DISCARD);
    assert(cj4_state_current_player(&after_discard) == CJ4_PLAYER_1);
    assert(after_discard.dora_count == 2);
    assert(after_discard.pending_kan_dora_count == 0);
}

static void
test_step_uses_delegate_for_draw_phase(
    void)
{
    cj4_mahjong state = make_empty_state();
    chooser_ctx contexts[CJ4_PLAYER_COUNT];
    cj4m_player_delegate delegates[CJ4_PLAYER_COUNT];
    cj4_mahjong next;
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

    for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
        delegates[i] = make_delegate(&contexts[i], CJ4_ACTION_PASS);

    delegates[CJ4_PLAYER_0] = make_delegate(&contexts[CJ4_PLAYER_0], CJ4_ACTION_RIICHI);

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    state.draw_tile = draw;

    next = cj4m_step(&state, NULL, delegates);

    assert(cj4_state_phase(&next) == CJ4_PHASE_DISCARD);
    assert(cj4_state_has_pending_riichi(&next));
    assert(cj4_state_pending_riichi_player(&next) == CJ4_PLAYER_0);
    assert(cj4_state_is_riichi(&next, CJ4_PLAYER_0) == 0);
    assert(next.scores[CJ4_PLAYER_0] == 25000);
    assert(contexts[CJ4_PLAYER_0].call_count == 1);
    assert(contexts[CJ4_PLAYER_0].last_view_player == CJ4_PLAYER_0);

    next = cj4m_step(&next, NULL, delegates);

    assert(cj4_state_is_riichi(&next, CJ4_PLAYER_0) == 1);
    assert(!cj4_state_has_pending_riichi(&next));
    assert(next.scores[CJ4_PLAYER_0] == 24000);
    assert(next.riichi_sticks == 1);
}

static void
test_step_can_choose_kyuushu_kyuuhai(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    chooser_ctx contexts[CJ4_PLAYER_COUNT];
    cj4m_player_delegate delegates[CJ4_PLAYER_COUNT];
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

    for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
        delegates[i] = make_delegate(&contexts[i], CJ4_ACTION_PASS);

    delegates[CJ4_PLAYER_0] = make_delegate(&contexts[CJ4_PLAYER_0], CJ4_ACTION_ABORTIVE_DRAW);

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    state.draw_tile = draw;
    cj4_state_set_first_turn(&state, 1);
    cj4_state_set_draw_turn(&state, CJ4_PLAYER_0, 1);

    next = cj4m_step(&state, &rules, delegates);

    assert(cj4_state_phase(&next) == CJ4_PHASE_ROUND_END);
    assert(cj4_state_round_end_type(&next) == CJ4_ROUND_END_ABORTIVE_DRAW);
    assert(cj4_state_abortive_reason(&next) == CJ4_ABORTIVE_DRAW_KYUUSHU_KYUUHAI);
}

static void
test_step_prioritizes_pon_over_chi(
    void)
{
    cj4_mahjong state = make_empty_state();
    chooser_ctx contexts[CJ4_PLAYER_COUNT];
    cj4m_player_delegate delegates[CJ4_PLAYER_COUNT];
    cj4_mahjong next;
    const cj4_tile_id chi_hand[] = {
        tile(1, 0),
        tile(2, 0)};
    const cj4_tile_id pon_hand[] = {
        tile(3, 1),
        tile(3, 2)};

    for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
        delegates[i] = make_delegate(&contexts[i], CJ4_ACTION_PASS);

    delegates[CJ4_PLAYER_1] = make_delegate(&contexts[CJ4_PLAYER_1], CJ4_ACTION_CHI);
    delegates[CJ4_PLAYER_2] = make_delegate(&contexts[CJ4_PLAYER_2], CJ4_ACTION_PON);

    set_hand(&state, CJ4_PLAYER_1, chi_hand, 2);
    set_hand(&state, CJ4_PLAYER_2, pon_hand, 2);
    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    add_discard(&state, CJ4_PLAYER_0, tile(3, 0));

    next = cj4m_step(&state, NULL, delegates);

    assert(cj4_state_phase(&next) == CJ4_PHASE_AFTER_CALL);
    assert(cj4_state_current_player(&next) == CJ4_PLAYER_2);
    cj4_meld_list pon_melds =
        cj4_location_collect_melds(next.locations, CJ4_PLAYER_2);
    assert(pon_melds.count == 1);
    assert(pon_melds.items[0].type == CJ4_MELD_PON);
    assert(contexts[CJ4_PLAYER_1].call_count == 1);
    assert(contexts[CJ4_PLAYER_2].call_count == 1);
}

static void
test_step_prioritizes_ron_and_respects_limit(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    chooser_ctx contexts[CJ4_PLAYER_COUNT];
    cj4m_player_delegate delegates[CJ4_PLAYER_COUNT];
    cj4_mahjong next;
    const cj4_tile_id chi_hand[] = {
        tile(4, 0),
        tile(5, 0)};
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
    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    add_discard(&state, CJ4_PLAYER_0, tile(6, 0));

    next = cj4m_step(&state, &rules, delegates);

    assert(cj4_state_phase(&next) == CJ4_PHASE_ROUND_END);
    assert(cj4_state_round_end_type(&next) == CJ4_ROUND_END_RON);
    assert(cj4_state_winner_count(&next) == 1);
    assert(cj4_get_winner(&next, 0) == CJ4_PLAYER_2);
    assert(contexts[CJ4_PLAYER_1].call_count == 1);
    assert(contexts[CJ4_PLAYER_2].call_count == 1);
    assert(contexts[CJ4_PLAYER_3].call_count == 1);
}

static void
test_step_advances_after_all_pass(
    void)
{
    cj4_mahjong state = make_empty_state();
    chooser_ctx contexts[CJ4_PLAYER_COUNT];
    cj4m_player_delegate delegates[CJ4_PLAYER_COUNT];
    cj4_mahjong next;
    cj4_tile_id next_draw = tile(30, 0);

    for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
        delegates[i] = make_delegate(&contexts[i], CJ4_ACTION_PASS);

    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    set_test_wall(&state, (uint8_t)(0), next_draw);
    state.wall_pos = 0;
    add_discard(&state, CJ4_PLAYER_0, tile(8, 0));

    next = cj4m_step(&state, NULL, delegates);

    assert(cj4_state_phase(&next) == CJ4_PHASE_DRAW);
    assert(cj4_state_current_player(&next) == CJ4_PLAYER_1);
    assert(next.draw_tile == next_draw);
    assert(contexts[CJ4_PLAYER_1].call_count == 1);
    assert(contexts[CJ4_PLAYER_2].call_count == 1);
    assert(contexts[CJ4_PLAYER_3].call_count == 1);
}

static void
test_step_allows_kokushi_ron_on_ankan(
    void)
{
    cj4_rules rules = cj4_rules_default();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong ankan;
    cj4_mahjong next;
    chooser_ctx contexts[CJ4_PLAYER_COUNT];
    cj4m_player_delegate delegates[CJ4_PLAYER_COUNT];
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

    for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
        delegates[i] = make_delegate(&contexts[i], CJ4_ACTION_PASS);
    delegates[CJ4_PLAYER_1] = make_delegate(&contexts[CJ4_PLAYER_1], CJ4_ACTION_RON);

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
    next = cj4m_step(&ankan, &rules, delegates);

    assert(cj4_state_phase(&next) == CJ4_PHASE_ROUND_END);
    assert(cj4_state_round_end_type(&next) == CJ4_ROUND_END_RON);
    assert(cj4_state_winner_count(&next) == 1);
    assert(cj4_get_winner(&next, 0) == CJ4_PLAYER_1);
}

void
cj4_test_manager_step(
    void)
{
    test_step_replaces_invalid_delegate_action();
    test_step_reveals_pending_kan_dora_after_single_delegate_call();
    test_step_does_not_reveal_current_kan_dora_on_rinshan_tsumo();
    test_step_minkan_flow_reveals_dora_after_discard_choice();
    test_step_uses_delegate_for_draw_phase();
    test_step_can_choose_kyuushu_kyuuhai();
    test_step_prioritizes_pon_over_chi();
    test_step_prioritizes_ron_and_respects_limit();
    test_step_advances_after_all_pass();
    test_step_allows_kokushi_ron_on_ankan();
}
