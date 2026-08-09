#include "state_discard.h"
#include "state_ops.h"

#include <assert.h>
#include <stddef.h>

static uint8_t
cj4_kuikae_is_forbidden(
    const cj4_mahjong *state,
    const cj4_rules *rules,
    cj4_tile_id tile)
{
    cj4_player player = state->current_player;
    const cj4_meld *meld;
    cj4_tile_type discard_type;
    cj4_tile_type called_type;

    if (!rules || !rules->kuikae_forbidden)
        return 0;

    if (state->phase != CJ4_PHASE_AFTER_CALL ||
        state->meld_count[player] == 0)
    {
        return 0;
    }

    meld = &state->melds[player][state->meld_count[player] - 1];

    if (meld->type != CJ4_MELD_PON && meld->type != CJ4_MELD_CHI)
        return 0;

    discard_type = cj4_tile_get_type(tile);
    called_type = cj4_tile_get_type(meld->tiles[meld->called_index]);

    if (discard_type == called_type)
        return 1;

    if (meld->type == CJ4_MELD_CHI)
    {
        cj4_tile_type min_type = cj4_tile_get_type(meld->tiles[0]);

        for (uint8_t i = 1; i < meld->size; ++i)
        {
            cj4_tile_type type = cj4_tile_get_type(meld->tiles[i]);
            if (type < min_type)
                min_type = type;
        }

        if (cj4_tile_type_get_suit(called_type) == CJ4_TILE_SUIT_HONOR)
            return 0;

        if (called_type == min_type)
        {
            cj4_tile_type outer = (cj4_tile_type)(min_type + 3);
            if (outer <= CJ4_TILE_TYPE_MAX &&
                cj4_tile_type_get_suit(outer) == cj4_tile_type_get_suit(called_type) &&
                discard_type == outer)
            {
                return 1;
            }
        }
        else if (called_type == (cj4_tile_type)(min_type + 2) &&
                 min_type > CJ4_TILE_TYPE_MIN)
        {
            cj4_tile_type outer = (cj4_tile_type)(min_type - 1);
            if (cj4_tile_type_get_suit(outer) == cj4_tile_type_get_suit(called_type) &&
                discard_type == outer)
            {
                return 1;
            }
        }
    }

    return 0;
}

bool
cj4_can_discard(
    const cj4_mahjong state,
    cj4_tile_id tile)
{
    return cj4_can_discard_with_rules(state, NULL, tile);
}

bool
cj4_can_discard_with_rules(
    const cj4_mahjong state,
    const cj4_rules *rules,
    cj4_tile_id tile)
{
    cj4_player player = state.current_player;

    if (state.phase != CJ4_PHASE_DRAW && state.phase != CJ4_PHASE_AFTER_CALL)
    {
        return false;
    }

    if (tile > CJ4_TILE_ID_MAX)
    {
        return false;
    }

    if (!cj4_state_tile_is_in_hand(&state, player, tile))
    {
        return false;
    }

    if (state.is_riichi[player] && tile != state.draw_tile)
    {
        return false;
    }

    if (cj4_kuikae_is_forbidden(&state, rules, tile))
        return false;

    return true;
}

cj4_mahjong
cj4_do_discard(
    const cj4_mahjong state,
    cj4_tile_id tile)
{
    return cj4_do_discard_with_rules(state, NULL, tile);
}

cj4_mahjong
cj4_do_discard_with_rules(
    const cj4_mahjong state,
    const cj4_rules *rules,
    cj4_tile_id tile)
{
    assert(cj4_can_discard_with_rules(state, rules, tile));

    cj4_mahjong next = state;

    cj4_state_reveal_pending_kan_dora(&next);
    cj4_state_record_discard(&next, tile, (uint8_t)(tile == state.draw_tile));
    cj4_state_clear_draw_tile(&next);
    next.winning_from_chankan = 0;
    next.pending_kakan_tile = CJ4_TILE_ID_INVALID;
    next.pending_ankan_tile = CJ4_TILE_ID_INVALID;
    if (state.is_ippatsu[state.current_player])
        next.is_ippatsu[state.current_player] = 0;

    if (state.is_riichi[state.current_player] &&
        state.first_turn_uninterrupted &&
        state.draw_turn_count[state.current_player] == 1)
    {
        next.riichi_declared_on_first_turn[state.current_player] = 1;
    }

    next.phase = CJ4_PHASE_DISCARD;

    return next;
}
