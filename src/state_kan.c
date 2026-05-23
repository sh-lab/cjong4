#include "state_kan.h"
#include "state_query.h"
#include "state_ops.h"
#include "state_internal.h"
#include <assert.h>

/* Minkan implementation (open kan using last discard) */
bool
cj4_can_minkan(const cj4_mahjong *state, cj4_player player)
{
    if (!cj4_state_can_claim_discard(state, player))
        return false;

    cj4_tile_type last_discard_tile_type = cj4_tile_get_type(cj4_get_last_discard_tile(state));

    if (cj4_count_hand(state, player, last_discard_tile_type) < 3)
        return false;

    return true;
}

bool
cj4_can_minkan_with_tile(const cj4_mahjong *state, cj4_player player, cj4_tile_id tile1, cj4_tile_id tile2, cj4_tile_id tile3)
{
    if (!cj4_can_minkan(state, player))
        return false;

    cj4_tile_id last = cj4_get_last_discard_tile(state);
    cj4_tile_type type = cj4_tile_get_type(last);
    if (cj4_tile_get_type(tile1) != type ||
        cj4_tile_get_type(tile2) != type ||
        cj4_tile_get_type(tile3) != type)
        return false;

    if (!cj4_state_tile_is_in_hand(state, player, tile1) ||
        !cj4_state_tile_is_in_hand(state, player, tile2) ||
        !cj4_state_tile_is_in_hand(state, player, tile3))
        return false;

    return true;
}

cj4_mahjong
cj4_do_minkan(const cj4_mahjong state, cj4_player player, cj4_tile_id tile1, cj4_tile_id tile2, cj4_tile_id tile3)
{
    assert(cj4_can_minkan_with_tile(&state, player, tile1, tile2, tile3));
    cj4_mahjong next = state;
    cj4_tile_id last = cj4_get_last_discard_tile(&state);
    const cj4_tile_id meld_tiles[4] = { last, tile1, tile2, tile3 };

    cj4_state_add_meld(
        &next,
        player,
        CJ4_MELD_MINKAN,
        meld_tiles,
        4,
        state.current_player,
        0);
    cj4_state_finish_open_call(&next, player, CJ4_PHASE_ANKAN_RESOLVE);
    next.first_turn_uninterrupted = 0;
    next.winning_from_chankan = 0;
    next.pending_kakan_tile = CJ4_TILE_ID_INVALID;

    return next;
}

/* Ankan (closed kan) */
bool
cj4_can_ankan(const cj4_mahjong *state)
{
    cj4_player player = state->current_player;
    if (state->phase != CJ4_PHASE_DRAW)
        return false;

    for (cj4_tile_type type = 0; type < CJ4_TILE_TYPE_COUNT; ++type)
    {
        if (cj4_count_hand(state, player, type) >= 4)
            return true;
    }

    return false;
}

bool
cj4_can_ankan_with_tile(const cj4_mahjong *state, cj4_tile_id tile1, cj4_tile_id tile2, cj4_tile_id tile3, cj4_tile_id tile4)
{
    if (!cj4_can_ankan(state))
        return false;

    cj4_player player = state->current_player;

    cj4_tile_type type = cj4_tile_get_type(tile1);

    if (cj4_tile_get_type(tile2) != type ||
        cj4_tile_get_type(tile3) != type ||
        cj4_tile_get_type(tile4) != type)
        return false;

    /* ensure distinct tile ids */
    if (tile1 == tile2 || tile1 == tile3 || tile1 == tile4 ||
        tile2 == tile3 || tile2 == tile4 || tile3 == tile4)
        return false;

    if (!cj4_state_tile_is_in_hand(state, player, tile1) ||
        !cj4_state_tile_is_in_hand(state, player, tile2) ||
        !cj4_state_tile_is_in_hand(state, player, tile3) ||
        !cj4_state_tile_is_in_hand(state, player, tile4))
        return false;

    return true;
}

cj4_mahjong
cj4_do_ankan(const cj4_mahjong state, cj4_tile_id tile1, cj4_tile_id tile2, cj4_tile_id tile3, cj4_tile_id tile4)
{
    assert(cj4_can_ankan_with_tile(&state, tile1, tile2, tile3, tile4));
    cj4_mahjong next = state;
    cj4_player player = state.current_player;
    const cj4_tile_id meld_tiles[4] = { tile1, tile2, tile3, tile4 };

    cj4_state_add_meld(
        &next,
        player,
        CJ4_MELD_ANKAN,
        meld_tiles,
        4,
        player,
        CJ4_CALLED_INDEX_NONE);
    cj4_state_clear_draw_tile(&next);
    cj4_state_clear_all_ippatsu(&next);

    next.draw_tile = cj4_state_draw_dead_wall_tile(&next, player);

    cj4_state_add_dora_indicator(&next);

    next.current_player = player;
    next.first_turn_uninterrupted = 0;
    next.winning_from_chankan = 0;
    next.pending_kakan_tile = CJ4_TILE_ID_INVALID;

    next.phase = CJ4_PHASE_DRAW;

    return next;
}

/* Kakan (added kan to existing pon) */
bool
cj4_can_kakan(const cj4_mahjong *state)
{
    if (state->phase != CJ4_PHASE_DRAW)
        return false;

    cj4_player player = state->current_player;

    for (uint8_t i = 0; i < state->meld_count[player]; ++i)
    {
        const cj4_meld *m = &state->melds[player][i];
        if (m->type == CJ4_MELD_PON)
        {
            cj4_tile_type ttype = cj4_tile_get_type(m->tiles[0]);
            if (cj4_count_hand(state, player, ttype) >= 1)
                return true;
        }
    }

    return false;
}

bool
cj4_can_kakan_with_tile(const cj4_mahjong *state, cj4_tile_id tile)
{
    if (state->phase != CJ4_PHASE_DRAW)
        return false;

    cj4_player player = state->current_player;
    if (!cj4_state_tile_is_in_hand(state, player, tile))
        return false;

    cj4_tile_type type = cj4_tile_get_type(tile);

    for (uint8_t i = 0; i < state->meld_count[player]; ++i)
    {
        const cj4_meld *m = &state->melds[player][i];
        if (m->type == CJ4_MELD_PON && cj4_tile_get_type(m->tiles[0]) == type)
            return true;
    }

    return false;
}

cj4_mahjong
cj4_do_kakan(const cj4_mahjong state, cj4_tile_id tile)
{
    assert(cj4_can_kakan_with_tile(&state, tile));
    cj4_mahjong next = state;
    cj4_player player = state.current_player;

    /* find the pon meld of matching type and convert it */
    for (uint8_t i = 0; i < next.meld_count[player]; ++i)
    {
        cj4_meld *m = &next.melds[player][i];
        if (m->type == CJ4_MELD_PON && cj4_tile_get_type(m->tiles[0]) == cj4_tile_get_type(tile))
        {
            m->tiles[3] = tile;
            m->size = 4;
            m->type = CJ4_MELD_KAKAN; /* convert from PON to KAKAN */
            /* called_index stays unchanged */

            cj4_state_set_location(&next, tile, CJ4_ZONE_MELD, player);

            break;
        }
    }

    /* Do not add dora or draw here; resolve in separate phase */
    /* current_player stays unchanged for kakan */
    cj4_state_clear_draw_tile(&next);
    cj4_state_clear_all_ippatsu(&next);
    next.first_turn_uninterrupted = 0;
    next.winning_from_chankan = 0;
    next.pending_kakan_tile = tile;
    next.phase = CJ4_PHASE_KAKAN_RESOLVE;

    return next;
}

bool
cj4_can_rinshan_draw(const cj4_mahjong* state)
{
    return state->phase == CJ4_PHASE_ANKAN_RESOLVE ||
           state->phase == CJ4_PHASE_KAKAN_RESOLVE;
}

/* Kan resolution: run once after any kan that defers draw/dora */
cj4_mahjong
cj4_do_rinshan_draw(const cj4_mahjong state)
{
    assert(cj4_can_rinshan_draw(&state));
    cj4_mahjong next = state;
    cj4_player player = state.current_player;

    /* Draw rinshan tile to hand of current player */
    cj4_tile_id t = cj4_state_draw_dead_wall_tile(&next, player);
    next.draw_tile = t;

    /* Increase dora indicator if possible */
    cj4_state_add_dora_indicator(&next);

    next.winning_from_chankan = 0;
    next.pending_kakan_tile = CJ4_TILE_ID_INVALID;
    next.phase = CJ4_PHASE_DRAW;

    return next;
}
