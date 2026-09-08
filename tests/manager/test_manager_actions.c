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
test_collect_actions_includes_pass_and_claims(
    void)
{
    cj4_mahjong state = make_empty_state();
    const cj4_tile_id chi_hand[] = {
        tile(1, 0),
        tile(2, 0),
        tile(5, 0)};
    const cj4_tile_id pon_hand[] = {
        tile(3, 1),
        tile(3, 2),
        tile(6, 0)};
    cj4_action actions[CJ4M_MAX_ACTIONS];
    uint8_t action_count;

    set_hand(
        &state,
        CJ4_PLAYER_1,
        chi_hand,
        (uint8_t)(sizeof(chi_hand) / sizeof(chi_hand[0])));
    set_hand(
        &state,
        CJ4_PLAYER_2,
        pon_hand,
        (uint8_t)(sizeof(pon_hand) / sizeof(pon_hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
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
test_collect_actions_respects_riichi_restrictions(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_action actions[CJ4M_MAX_ACTIONS];
    uint8_t action_count;
    cj4_tile_id draw = tile(4, 3);
    const cj4_tile_id hand[] = {
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

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    state.draw_tile = draw;
    cj4_state_set_riichi(&state, CJ4_PLAYER_0, 1);
    set_test_meld(&state, CJ4_PLAYER_0, 0, &(cj4_meld){.tiles = {tile(4, 0), tile(4, 1), tile(4, 2)}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_1, .called_index = 0});

    action_count = cj4m_collect_actions(
        &state,
        NULL,
        CJ4_PLAYER_0,
        actions,
        CJ4M_MAX_ACTIONS);

    assert(contains_action(actions, action_count, CJ4_ACTION_DISCARD, draw));
    assert(!contains_action(actions, action_count, CJ4_ACTION_DISCARD, tile(9, 0)));
    assert(!contains_action_type(actions, action_count, CJ4_ACTION_KAKAN));
}

static void
test_collect_actions_filters_kuikae_discards(
    void)
{
    cj4_rules rules = cj4_rules_default();
    cj4_mahjong state = make_empty_state();
    cj4_action actions[CJ4M_MAX_ACTIONS];
    uint8_t action_count;

    cj4_state_set_phase(&state, CJ4_PHASE_AFTER_CALL);
    cj4_state_set_current_player(&state, CJ4_PLAYER_1);
    set_test_meld(&state, CJ4_PLAYER_1, 0, &(cj4_meld){.tiles = {tile(2, 0), tile(3, 0), tile(4, 0)}, .size = 3, .type = CJ4_MELD_CHI, .from_player = CJ4_PLAYER_0, .called_index = 0});
    set_hand(&state, CJ4_PLAYER_1, (const cj4_tile_id[]){tile(2, 1), tile(5, 0), tile(6, 0)}, 3);

    action_count = cj4m_collect_actions(
        &state,
        &rules,
        CJ4_PLAYER_1,
        actions,
        CJ4M_MAX_ACTIONS);

    assert(!contains_action(actions, action_count, CJ4_ACTION_DISCARD, tile(2, 1)));
    assert(!contains_action(actions, action_count, CJ4_ACTION_DISCARD, tile(5, 0)));
    assert(contains_action(actions, action_count, CJ4_ACTION_DISCARD, tile(6, 0)));
}

static void
test_collect_actions_filters_calls_without_legal_discard(
    void)
{
    cj4_rules rules = cj4_rules_default();
    cj4_mahjong state = make_empty_state();
    cj4_action actions[CJ4M_MAX_ACTIONS];
    const cj4_tile_id hand[] = {
        tile(3, 0),
        tile(4, 0),
        tile(2, 1),
        tile(5, 0)};
    uint8_t action_count;

    set_hand(
        &state,
        CJ4_PLAYER_1,
        hand,
        (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    add_discard(&state, CJ4_PLAYER_0, tile(2, 0));

    action_count = cj4m_collect_actions(
        &state,
        NULL,
        CJ4_PLAYER_1,
        actions,
        CJ4M_MAX_ACTIONS);
    assert(contains_action_type(actions, action_count, CJ4_ACTION_CHI));

    action_count = cj4m_collect_actions(
        &state,
        &rules,
        CJ4_PLAYER_1,
        actions,
        CJ4M_MAX_ACTIONS);
    assert(contains_action_type(actions, action_count, CJ4_ACTION_PASS));
    assert(!contains_action_type(actions, action_count, CJ4_ACTION_CHI));
}

static void
test_collect_actions_hides_ankan_ron_when_rule_disabled(
    void)
{
    cj4_rules rules = cj4_rules_tenhou();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong ankan;
    cj4_action actions[CJ4M_MAX_ACTIONS];
    uint8_t action_count;
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
    action_count = cj4m_collect_actions(
        &ankan,
        &rules,
        CJ4_PLAYER_1,
        actions,
        CJ4M_MAX_ACTIONS);

    assert(contains_action_type(actions, action_count, CJ4_ACTION_PASS));
    assert(!contains_action_type(actions, action_count, CJ4_ACTION_RON));
}

void
cj4_test_manager_actions(
    void)
{
    test_collect_actions_includes_pass_and_claims();
    test_collect_actions_respects_riichi_restrictions();
    test_collect_actions_filters_kuikae_discards();
    test_collect_actions_filters_calls_without_legal_discard();
    test_collect_actions_hides_ankan_ron_when_rule_disabled();
}
