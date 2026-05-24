#include "cjong4/manager/manager.h"

#include "state_chi.h"
#include "state_discard.h"
#include "state_kan.h"
#include "state_ops.h"
#include "state_pass.h"
#include "state_pon.h"
#include "state_query.h"
#include "state_riichi.h"
#include "state_ron.h"
#include "state_tsumo.h"

#include <assert.h>
#include <string.h>

static void
cj4m_push_action(
    cj4_action *actions,
    uint8_t capacity,
    uint8_t *count,
    const cj4_action *action)
{
    assert(*count < capacity);
    if (*count >= capacity)
        return;

    actions[*count] = *action;
    (*count)++;
}

static cj4_action
cj4m_make_action(cj4_action_type type, cj4_player player)
{
    cj4_action action;

    memset(&action, 0, sizeof(action));
    action.type = type;
    action.player = player;
    action.tile = CJ4_TILE_ID_INVALID;

    return action;
}

static uint8_t
cj4m_collect_hand_tiles_of_type(
    const cj4_mahjong *state,
    cj4_player player,
    cj4_tile_type type,
    cj4_tile_id out[4])
{
    uint8_t count = 0;

    for (uint8_t index = 0; index < CJ4_TILE_PER_TYPE; ++index)
    {
        cj4_tile_id tile = cj4_tile_make(type, index);
        if (!cj4_state_tile_is_in_hand(state, player, tile))
            continue;

        out[count++] = tile;
    }

    return count;
}

static void
cj4m_collect_turn_actions(
    const cj4_mahjong *state,
    const cj4_rules *rules,
    cj4_player player,
    cj4_action *actions,
    uint8_t capacity,
    uint8_t *count)
{
    for (cj4_tile_id tile = CJ4_TILE_ID_MIN; tile <= CJ4_TILE_ID_MAX; ++tile)
    {
        cj4_action action;

        if (!cj4_can_discard(*state, tile))
            continue;

        action = cj4m_make_action(CJ4_ACTION_DISCARD, player);
        action.tile = tile;
        action.tiles[0] = tile;
        action.tile_count = 1;
        cj4m_push_action(actions, capacity, count, &action);
    }

    if (state->phase != CJ4_PHASE_DRAW)
        return;

    for (cj4_tile_id tile = CJ4_TILE_ID_MIN; tile <= CJ4_TILE_ID_MAX; ++tile)
    {
        cj4_action action;

        if (!cj4_can_riichi(state, tile))
            continue;

        action = cj4m_make_action(CJ4_ACTION_RIICHI, player);
        action.tile = tile;
        action.tiles[0] = tile;
        action.tile_count = 1;
        cj4m_push_action(actions, capacity, count, &action);
    }

    if (cj4_can_tsumo(state, rules))
    {
        cj4_action action = cj4m_make_action(CJ4_ACTION_TSUMO, player);
        action.tile = state->draw_tile;
        cj4m_push_action(actions, capacity, count, &action);
    }

    for (cj4_tile_type type = CJ4_TILE_TYPE_MIN; type <= CJ4_TILE_TYPE_MAX; ++type)
    {
        cj4_tile_id tiles[4];
        cj4_action action;

        if (cj4m_collect_hand_tiles_of_type(state, player, type, tiles) < 4)
            continue;

        if (!cj4_can_ankan_with_tile(state, tiles[0], tiles[1], tiles[2], tiles[3]))
            continue;

        action = cj4m_make_action(CJ4_ACTION_ANKAN, player);
        action.tile = tiles[0];
        memcpy(action.tiles, tiles, sizeof(tiles));
        action.tile_count = 4;
        cj4m_push_action(actions, capacity, count, &action);
    }

    for (cj4_tile_id tile = CJ4_TILE_ID_MIN; tile <= CJ4_TILE_ID_MAX; ++tile)
    {
        cj4_action action;

        if (!cj4_can_kakan_with_tile(state, tile))
            continue;

        action = cj4m_make_action(CJ4_ACTION_KAKAN, player);
        action.tile = tile;
        action.tiles[0] = tile;
        action.tile_count = 1;
        cj4m_push_action(actions, capacity, count, &action);
    }
}

static void
cj4m_collect_discard_reaction_actions(
    const cj4_mahjong *state,
    const cj4_rules *rules,
    cj4_player player,
    cj4_action *actions,
    uint8_t capacity,
    uint8_t *count)
{
    cj4_action pass_action = cj4m_make_action(CJ4_ACTION_PASS, player);
    cj4_tile_id last;
    cj4_tile_type type;
    cj4_tile_id hand_tiles[4];
    uint8_t hand_count;

    cj4m_push_action(actions, capacity, count, &pass_action);

    if (cj4_can_ron(state, player, rules))
    {
        cj4_action action = cj4m_make_action(CJ4_ACTION_RON, player);
        action.tile = cj4_get_last_discard_tile(state);
        cj4m_push_action(actions, capacity, count, &action);
    }

    last = cj4_get_last_discard_tile(state);
    type = cj4_tile_get_type(last);

    if (cj4_can_pon(state, player))
    {
        hand_count = cj4m_collect_hand_tiles_of_type(state, player, type, hand_tiles);
        for (uint8_t i = 0; i < hand_count; ++i)
        {
            for (uint8_t j = (uint8_t)(i + 1); j < hand_count; ++j)
            {
                cj4_action action;

                if (!cj4_can_pon_with_tile(state, player, hand_tiles[i], hand_tiles[j]))
                    continue;

                action = cj4m_make_action(CJ4_ACTION_PON, player);
                action.tile = last;
                action.tiles[0] = hand_tiles[i];
                action.tiles[1] = hand_tiles[j];
                action.tile_count = 2;
                cj4m_push_action(actions, capacity, count, &action);
            }
        }
    }

    if (cj4_can_minkan(state, player))
    {
        cj4_action action;

        hand_count = cj4m_collect_hand_tiles_of_type(state, player, type, hand_tiles);
        assert(hand_count >= 3);

        action = cj4m_make_action(CJ4_ACTION_MINKAN, player);
        action.tile = last;
        action.tiles[0] = hand_tiles[0];
        action.tiles[1] = hand_tiles[1];
        action.tiles[2] = hand_tiles[2];
        action.tile_count = 3;
        cj4m_push_action(actions, capacity, count, &action);
    }

    if (player == cj4_next_player(state) && cj4_can_chi(state))
    {
        for (cj4_tile_id tile1 = CJ4_TILE_ID_MIN; tile1 <= CJ4_TILE_ID_MAX; ++tile1)
        {
            if (!cj4_state_tile_is_in_hand(state, player, tile1))
                continue;

            for (cj4_tile_id tile2 = (cj4_tile_id)(tile1 + 1);
                 tile2 <= CJ4_TILE_ID_MAX;
                 ++tile2)
            {
                cj4_action action;

                if (!cj4_state_tile_is_in_hand(state, player, tile2))
                    continue;

                if (!cj4_can_chi_with_tile(state, tile1, tile2))
                    continue;

                action = cj4m_make_action(CJ4_ACTION_CHI, player);
                action.tile = last;
                action.tiles[0] = tile1;
                action.tiles[1] = tile2;
                action.tile_count = 2;
                cj4m_push_action(actions, capacity, count, &action);
            }
        }
    }
}

static void
cj4m_collect_kakan_reaction_actions(
    const cj4_mahjong *state,
    const cj4_rules *rules,
    cj4_player player,
    cj4_action *actions,
    uint8_t capacity,
    uint8_t *count)
{
    cj4_action pass_action = cj4m_make_action(CJ4_ACTION_PASS, player);

    cj4m_push_action(actions, capacity, count, &pass_action);

    if (cj4_can_ron(state, player, rules))
    {
        cj4_action action = cj4m_make_action(CJ4_ACTION_RON, player);
        action.tile = state->pending_kakan_tile;
        cj4m_push_action(actions, capacity, count, &action);
    }
}

uint8_t
cj4m_collect_actions(
    const cj4_mahjong *state,
    const cj4_rules *rules,
    cj4_player player,
    cj4_action *actions,
    uint8_t capacity)
{
    uint8_t count = 0;

    assert(state != 0);
    assert(actions != 0 || capacity == 0);

    if (capacity == 0)
        return 0;

    switch (state->phase)
    {
    case CJ4_PHASE_DRAW:
    case CJ4_PHASE_AFTER_CALL:
        if (player == state->current_player)
        {
            cj4m_collect_turn_actions(
                state,
                rules,
                player,
                actions,
                capacity,
                &count);
        }
        break;
    case CJ4_PHASE_DISCARD:
        if (player != state->current_player)
        {
            cj4m_collect_discard_reaction_actions(
                state,
                rules,
                player,
                actions,
                capacity,
                &count);
        }
        break;
    case CJ4_PHASE_KAKAN_RESOLVE:
        if (player != state->current_player)
        {
            cj4m_collect_kakan_reaction_actions(
                state,
                rules,
                player,
                actions,
                capacity,
                &count);
        }
        break;
    default:
        break;
    }

    return count;
}
