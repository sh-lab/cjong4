#include "cjong4/manager/manager.h"

#include "state_chi.h"
#include "state_discard.h"
#include "state_kan.h"
#include "state_pass.h"
#include "state_pon.h"
#include "state_riichi.h"
#include "state_ron.h"
#include "state_round.h"
#include "state_settle.h"
#include "state_tsumo.h"

#include <assert.h>

static uint8_t
cj4m_action_equals(
    const cj4_action *lhs,
    const cj4_action *rhs)
{
    if (lhs->type != rhs->type ||
        lhs->player != rhs->player ||
        lhs->tile != rhs->tile ||
        lhs->tile_count != rhs->tile_count)
    {
        return 0;
    }

    for (uint8_t i = 0; i < lhs->tile_count; ++i)
    {
        if (lhs->tiles[i] != rhs->tiles[i])
            return 0;
    }

    return 1;
}

static uint8_t
cj4m_action_is_offered(
    const cj4_action *selected,
    const cj4_action *actions,
    uint8_t action_count)
{
    for (uint8_t i = 0; i < action_count; ++i)
    {
        if (cj4m_action_equals(selected, &actions[i]))
            return 1;
    }

    return 0;
}

static uint8_t
cj4m_distance_from_current(
    const cj4_mahjong *state,
    cj4_player player)
{
    return (uint8_t)((player + CJ4_PLAYER_COUNT - state->current_player) %
                     CJ4_PLAYER_COUNT);
}

static cj4_action
cj4m_select_action(
    const cj4m_player_delegate *delegate,
    const cj4_player_view *view,
    const cj4_action *actions,
    uint8_t action_count)
{
    cj4_action selected;

    assert(delegate != 0);
    assert(delegate->decide != 0);
    assert(action_count > 0);

    selected = delegate->decide(delegate->ctx, view, actions, action_count);
    assert(cj4m_action_is_offered(&selected, actions, action_count));

    return selected;
}

static cj4_mahjong
cj4m_apply_single_action(
    const cj4_mahjong *state,
    const cj4_rules *rules,
    const cj4_action *action)
{
    switch (action->type)
    {
    case CJ4_ACTION_DISCARD:
        return cj4_do_discard(*state, action->tile);
    case CJ4_ACTION_CHI:
        return cj4_do_chi(*state, action->tiles[0], action->tiles[1]);
    case CJ4_ACTION_PON:
        return cj4_do_pon(*state, action->player, action->tiles[0], action->tiles[1]);
    case CJ4_ACTION_ANKAN:
        return cj4_do_ankan(
            *state,
            action->tiles[0],
            action->tiles[1],
            action->tiles[2],
            action->tiles[3]);
    case CJ4_ACTION_MINKAN:
        return cj4_do_minkan(
            *state,
            action->player,
            action->tiles[0],
            action->tiles[1],
            action->tiles[2]);
    case CJ4_ACTION_KAKAN:
        return cj4_do_kakan(*state, action->tile);
    case CJ4_ACTION_RIICHI:
        return cj4_do_riichi(*state, action->tile);
    case CJ4_ACTION_TSUMO:
        return cj4_do_tsumo(*state);
    case CJ4_ACTION_RON:
    {
        cj4_player players[1] = {action->player};
        return cj4_do_ron_multi(*state, players, 1, rules);
    }
    case CJ4_ACTION_PASS:
    default:
        break;
    }

    return *state;
}

static cj4_mahjong
cj4m_step_turn_phase(
    const cj4_mahjong *state,
    const cj4_rules *rules,
    const cj4m_player_delegate delegates[CJ4_PLAYER_COUNT])
{
    cj4_action actions[CJ4M_MAX_ACTIONS];
    cj4_player player = state->current_player;
    cj4_player_view view = cj4m_make_player_view(state, player);
    uint8_t action_count = cj4m_collect_actions(
        state,
        rules,
        player,
        actions,
        CJ4M_MAX_ACTIONS);
    cj4_action selected = cj4m_select_action(
        &delegates[player],
        &view,
        actions,
        action_count);

    return cj4m_apply_single_action(state, rules, &selected);
}

static cj4_mahjong
cj4m_step_discard_phase(
    const cj4_mahjong *state,
    const cj4_rules *rules,
    const cj4m_player_delegate delegates[CJ4_PLAYER_COUNT])
{
    cj4_player ron_players[CJ4_PLAYER_COUNT];
    uint8_t ron_count = 0;
    uint8_t have_call = 0;
    uint8_t have_chi = 0;
    cj4_action best_call;
    cj4_action best_chi;

    for (cj4_player player = CJ4_PLAYER_0; player < CJ4_PLAYER_COUNT; ++player)
    {
        cj4_action actions[CJ4M_MAX_ACTIONS];
        cj4_player_view view;
        uint8_t action_count;
        cj4_action selected;

        if (player == state->current_player)
            continue;

        view = cj4m_make_player_view(state, player);
        action_count = cj4m_collect_actions(
            state,
            rules,
            player,
            actions,
            CJ4M_MAX_ACTIONS);
        selected = cj4m_select_action(
            &delegates[player],
            &view,
            actions,
            action_count);

        if (selected.type == CJ4_ACTION_RON)
        {
            ron_players[ron_count++] = player;
            continue;
        }

        if (selected.type == CJ4_ACTION_CHI)
        {
            best_chi = selected;
            have_chi = 1;
            continue;
        }

        if (selected.type != CJ4_ACTION_PON &&
            selected.type != CJ4_ACTION_MINKAN)
        {
            continue;
        }

        if (!have_call ||
            cj4m_distance_from_current(state, selected.player) <
                cj4m_distance_from_current(state, best_call.player))
        {
            best_call = selected;
            have_call = 1;
        }
    }

    if (ron_count > 0)
        return cj4_do_ron_multi(*state, ron_players, ron_count, rules);

    if (have_call)
        return cj4m_apply_single_action(state, rules, &best_call);

    if (have_chi)
        return cj4m_apply_single_action(state, rules, &best_chi);

    return cj4_do_pass(*state, rules);
}

static cj4_mahjong
cj4m_step_kakan_resolve_phase(
    const cj4_mahjong *state,
    const cj4_rules *rules,
    const cj4m_player_delegate delegates[CJ4_PLAYER_COUNT])
{
    cj4_player ron_players[CJ4_PLAYER_COUNT];
    uint8_t ron_count = 0;

    for (cj4_player player = CJ4_PLAYER_0; player < CJ4_PLAYER_COUNT; ++player)
    {
        cj4_action actions[CJ4M_MAX_ACTIONS];
        cj4_player_view view;
        uint8_t action_count;
        cj4_action selected;

        if (player == state->current_player)
            continue;

        view = cj4m_make_player_view(state, player);
        action_count = cj4m_collect_actions(
            state,
            rules,
            player,
            actions,
            CJ4M_MAX_ACTIONS);
        selected = cj4m_select_action(
            &delegates[player],
            &view,
            actions,
            action_count);

        if (selected.type == CJ4_ACTION_RON)
            ron_players[ron_count++] = player;
    }

    if (ron_count > 0)
        return cj4_do_ron_multi(*state, ron_players, ron_count, rules);

    return cj4_do_rinshan_draw(*state, rules);
}

cj4_mahjong
cj4m_step(
    const cj4_mahjong *state,
    const cj4_rules *rules,
    const cj4m_player_delegate delegates[CJ4_PLAYER_COUNT])
{
    assert(state != 0);

    switch (state->phase)
    {
    case CJ4_PHASE_DRAW:
    case CJ4_PHASE_AFTER_CALL:
        return cj4m_step_turn_phase(state, rules, delegates);
    case CJ4_PHASE_DISCARD:
        return cj4m_step_discard_phase(state, rules, delegates);
    case CJ4_PHASE_ANKAN_RESOLVE:
        return cj4_do_rinshan_draw(*state, rules);
    case CJ4_PHASE_KAKAN_RESOLVE:
        return cj4m_step_kakan_resolve_phase(state, rules, delegates);
    case CJ4_PHASE_ROUND_END:
        return cj4_do_settle(*state, rules);
    case CJ4_PHASE_SETTLE:
        if (cj4_can_game_end(*state))
            return cj4_do_game_end(*state);
        break;
    case CJ4_PHASE_GAME_END:
    default:
        break;
    }

    return *state;
}
