#include "state_pass.h"
#include "state_ops.h"
#include "state_query.h"
#include "state_ron.h"
#include "tile_const.h"

#include <assert.h>

static uint8_t
cj4_all_players_riichi(
    const cj4_mahjong *state)
{
    for (uint8_t player = 0; player < CJ4_PLAYER_COUNT; ++player)
    {
        if (!state->is_riichi[player])
            return 0;
    }

    return 1;
}

static uint8_t
cj4_is_suufon_renda(
    const cj4_mahjong *state)
{
    uint8_t discard_count[CJ4_PLAYER_COUNT] = {0};
    cj4_tile_type first_type[CJ4_PLAYER_COUNT] = {0};
    cj4_tile_type type;

    if (!state->first_turn_uninterrupted ||
        state->discard_count < CJ4_PLAYER_COUNT)
    {
        return 0;
    }

    for (uint8_t i = 0; i < state->discard_count; ++i)
    {
        const cj4_discard *discard = &state->discards[i];

        if (discard_count[discard->player] == 0)
            first_type[discard->player] = cj4_tile_get_type(discard->tile);

        discard_count[discard->player]++;
    }

    for (uint8_t player = 0; player < CJ4_PLAYER_COUNT; ++player)
    {
        if (discard_count[player] != 1)
            return 0;
    }

    type = first_type[0];
    if (type < CJ4_TILE_TYPE_EAST || type > CJ4_TILE_TYPE_NORTH)
        return 0;

    for (uint8_t player = 1; player < CJ4_PLAYER_COUNT; ++player)
    {
        if (first_type[player] != type)
            return 0;
    }

    return 1;
}

static uint8_t
cj4_player_has_nagashi_mangan(
    const cj4_mahjong *state,
    cj4_player player)
{
    uint8_t has_discard = 0;

    for (uint8_t i = 0; i < state->discard_count; ++i)
    {
        const cj4_discard *discard = &state->discards[i];

        if (discard->player != player)
            continue;

        has_discard = 1;
        if (!discard->is_active || !cj4_tile_is_yaochu(discard->tile))
            return 0;
    }

    return has_discard;
}

static void
cj4_mark_nagashi_mangan(
    cj4_mahjong *state,
    const cj4_rules *rules)
{
    if (!rules || !rules->nagashi_mangan)
        return;

    for (uint8_t player = 0; player < CJ4_PLAYER_COUNT; ++player)
    {
        state->nagashi_mangan[player] =
            cj4_player_has_nagashi_mangan(state, (cj4_player)player);
    }
}

bool
cj4_can_pass(
    const cj4_mahjong state)
{
    if (state.phase != CJ4_PHASE_DISCARD)
    {
        return false;
    }

    return true;
}

cj4_mahjong
cj4_do_pass(
    const cj4_mahjong state,
    const cj4_rules *rules)
{
    assert(cj4_can_pass(state));

    cj4_mahjong next = state;

    for (uint8_t player = 0; player < CJ4_PLAYER_COUNT; ++player)
    {
        if (player == state.current_player)
            continue;

        if (!cj4_can_ron(&state, (cj4_player)player, rules))
            continue;

        if (state.is_riichi[player])
            next.riichi_furiten[player] = 1;
        else
            next.temporary_furiten[player] = 1;
    }

    cj4_state_establish_pending_riichi(&next);

    if (rules &&
        rules->abortive_suufon_renda &&
        cj4_is_suufon_renda(&next))
    {
        cj4_state_finish_abortive_draw(&next, CJ4_ABORTIVE_DRAW_SUUFON_RENDA);
        return next;
    }

    if (rules &&
        rules->abortive_four_riichi &&
        cj4_all_players_riichi(&next))
    {
        cj4_state_finish_abortive_draw(&next, CJ4_ABORTIVE_DRAW_FOUR_RIICHI);
        return next;
    }

    if (cj4_state_live_wall_remaining(&next) == 0)
    {
        cj4_mark_nagashi_mangan(&next, rules);
        cj4_state_finish_draw_round(&next, CJ4_ROUND_END_EXHAUSTIVE_DRAW);

        return next;
    }

    next.current_player = cj4_next_player(&state);
    if (next.draw_turn_count[next.current_player] > 0)
        next.first_turn_uninterrupted = 0;
    next.draw_tile = cj4_state_draw_tile(&next, next.current_player);
    next.draw_turn_count[next.current_player]++;
    next.winning_from_chankan = 0;
    next.pending_kakan_tile = CJ4_TILE_ID_INVALID;

    next.phase = CJ4_PHASE_DRAW;

    return next;
}
