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
test_player_view_hides_hidden_information(
    void)
{
    cj4_mahjong state = make_empty_state();
    const cj4_tile_id hand0[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(3, 0)};
    const cj4_tile_id hand1[] = {
        tile(10, 0),
        tile(10, 1),
        tile(10, 2)};
    cj4_player_view view;

    set_hand(&state, CJ4_PLAYER_0, hand0, 4);
    set_hand(&state, CJ4_PLAYER_1, hand1, 3);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    state.draw_tile = hand0[3];
    cj4_state_set_riichi(&state, CJ4_PLAYER_1, 1);
    cj4_state_set_temporary_furiten(&state, CJ4_PLAYER_0, 1);
    state.honba = 2;
    state.riichi_sticks = 3;
    state.dora_count = 2;
    set_test_wall(&state, (uint8_t)(130), tile(27, 0));
    set_test_wall(&state, (uint8_t)(128), tile(31, 0));
    add_discard(&state, CJ4_PLAYER_2, tile(20, 0));

    view = cj4m_make_player_view(&state, CJ4_PLAYER_0);
    cj4_hand visible_hand =
        cj4_location_collect_hand(view.locations, CJ4_PLAYER_0);
    cj4_discard_list visible_discards =
        cj4_location_collect_discards(view.locations);
    cj4_dora_indicator_list visible_dora =
        cj4_location_collect_dora_indicators(view.locations);

    assert(view.player == CJ4_PLAYER_0);
    assert(visible_hand.count == 4);
    assert(contains_tile(visible_hand.items, visible_hand.count, hand0[0]));
    assert(contains_tile(visible_hand.items, visible_hand.count, hand0[3]));
    assert(!contains_tile(visible_hand.items, visible_hand.count, hand1[0]));
    assert(cj4_location_is_unknown(&view.locations[hand1[0]]));
    assert(view.draw_tile == hand0[3]);
    assert(view.last_discard == tile(20, 0));
    assert(visible_discards.count == 1);
    assert(visible_discards.items[0].tile == tile(20, 0));
    assert(view.is_riichi[CJ4_PLAYER_1] == 1);
    assert(view.temporary_furiten == 1);
    assert(view.honba == 2);
    assert(view.riichi_sticks == 3);
    assert(visible_dora.count == 2);
    assert(visible_dora.items[0] == tile(27, 0));
    assert(visible_dora.items[1] == tile(31, 0));

    state.draw_tile = 200;
    state.locations[tile(27, 0)].wall = CJ4_LOCATION_NONE;
    view = cj4m_make_player_view(&state, CJ4_PLAYER_0);
    visible_dora = cj4_location_collect_dora_indicators(view.locations);
    assert(view.draw_tile == CJ4_TILE_ID_INVALID);
    assert(visible_dora.count == 1);
    assert(visible_dora.items[0] == tile(31, 0));
}

static void
test_player_view_exposes_only_kan_target_and_public_meld_location(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_tile_id kakan_tile = tile(6, 3);
    cj4_tile_id ankan_tiles[] = {
        tile(7, 0),
        tile(7, 1),
        tile(7, 2),
        tile(7, 3)};
    cj4_player_view view;

    state.locations[kakan_tile].placement =
        cj4_location_make_meld(CJ4_PLAYER_1, 0, CJ4_MELD_KAKAN);
    state.pending_kakan_tile = kakan_tile;
    cj4_state_set_current_player(&state, CJ4_PLAYER_1);
    cj4_state_set_phase(&state, CJ4_PHASE_KAKAN_RESOLVE);

    view = cj4m_make_player_view(&state, CJ4_PLAYER_0);
    assert(view.kan_tile == kakan_tile);
    assert(cj4_location_is_meld(view.locations[kakan_tile].placement));
    assert(view.locations[kakan_tile].wall == CJ4_LOCATION_NONE);

    view = cj4m_make_player_view(&state, CJ4_PLAYER_1);
    assert(view.locations[kakan_tile].wall != CJ4_LOCATION_NONE);

    set_hand(&state, CJ4_PLAYER_1, ankan_tiles, 4);
    state.pending_kakan_tile = CJ4_TILE_ID_INVALID;
    state.pending_ankan_tile = ankan_tiles[0];
    cj4_state_set_phase(&state, CJ4_PHASE_ANKAN_RESOLVE);
    view = cj4m_make_player_view(&state, CJ4_PLAYER_0);
    assert(view.kan_tile == ankan_tiles[0]);
    assert(cj4_location_is_unknown(&view.locations[ankan_tiles[0]]));

    set_test_meld(
        &state,
        CJ4_PLAYER_1,
        1,
        &(cj4_meld){
            .tiles = {
                ankan_tiles[0],
                ankan_tiles[1],
                ankan_tiles[2],
                ankan_tiles[3]},
            .size = 4,
            .type = CJ4_MELD_ANKAN,
            .from_player = CJ4_PLAYER_1,
            .called_index = CJ4_CALLED_INDEX_NONE});
    state.pending_ankan_tile = CJ4_TILE_ID_INVALID;
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    view = cj4m_make_player_view(&state, CJ4_PLAYER_0);
    assert(view.kan_tile == CJ4_TILE_ID_INVALID);
    assert(cj4_location_is_meld(view.locations[ankan_tiles[0]].placement));
    assert(view.locations[ankan_tiles[0]].wall == CJ4_LOCATION_NONE);
}

void
cj4_test_player_view(
    void)
{
    test_player_view_hides_hidden_information();
    test_player_view_exposes_only_kan_target_and_public_meld_location();
}
