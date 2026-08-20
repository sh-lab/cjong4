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
        if (!cj4_state_is_riichi(state, (cj4_player)player))
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

    if (!cj4_state_first_turn(state) ||
        state->discard_count < CJ4_PLAYER_COUNT)
    {
        return 0;
    }

    cj4_discard_list discards =
        cj4_location_collect_discards(state->locations);
    for (uint8_t i = 0; i < discards.count; ++i)
    {
        const cj4_discard *discard = &discards.items[i];

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

bool
cj4_can_pass(
    const cj4_mahjong state)
{
    if (cj4_state_phase(&state) != CJ4_PHASE_DISCARD)
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
        if (player == cj4_state_current_player(&state))
            continue;

        if (!cj4_can_ron(&state, (cj4_player)player, rules))
            continue;

        if (cj4_state_is_riichi(&state, (cj4_player)player))
            cj4_state_set_riichi_furiten(&next, (cj4_player)player, true);
        else
            cj4_state_set_temporary_furiten(&next, (cj4_player)player, true);
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
        cj4_state_finish_draw_round(&next, CJ4_ROUND_END_EXHAUSTIVE_DRAW);

        return next;
    }

    cj4_state_set_current_player(&next, cj4_next_player(&state));
    uint8_t turn = cj4_state_draw_turn(&next, cj4_state_current_player(&next));
    if (turn > 0)
        cj4_state_set_first_turn(&next, 0);
    next.draw_tile = cj4_state_draw_tile(&next, cj4_state_current_player(&next));
    cj4_state_set_draw_turn(
        &next,
        cj4_state_current_player(&next),
        turn == 0 ? 1 : 2);
    cj4_state_set_chankan(&next, 0);
    next.pending_kakan_tile = CJ4_TILE_ID_INVALID;

    cj4_state_set_phase(&next, CJ4_PHASE_DRAW);

    return next;
}
