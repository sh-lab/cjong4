#include "state_kan.h"
#include "state_query.h"
#include "state_ops.h"
#include "state_internal.h"
#include <assert.h>

/* Minkan implementation (open kan using last discard) */
bool
cj4_can_minkan(const cj4_mahjong *state, cj4_player player)
{
    if (state->phase != CJ4_PHASE_DISCARD)
        return false;
    if (player == state->current_player)
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

    const cj4_location *l1 = cj4_tile_location_const(state, tile1);
    const cj4_location *l2 = cj4_tile_location_const(state, tile2);
    const cj4_location *l3 = cj4_tile_location_const(state, tile3);
    if (l1->zone != CJ4_ZONE_HAND || l1->owner != player)
        return false;
    if (l2->zone != CJ4_ZONE_HAND || l2->owner != player)
        return false;
    if (l3->zone != CJ4_ZONE_HAND || l3->owner != player)
        return false;

    return true;
}

cj4_mahjong
cj4_do_minkan(const cj4_mahjong state, cj4_player player, cj4_tile_id tile1, cj4_tile_id tile2, cj4_tile_id tile3)
{
    assert(cj4_can_minkan_with_tile(&state, player, tile1, tile2, tile3));
    cj4_mahjong next = state;
    cj4_tile_id last = cj4_get_last_discard_tile(&state);

    cj4_meld *m = &next.melds[player][next.meld_count[player]++];
    m->type = CJ4_MELD_MINKAN;
    m->tiles[0] = last;
    m->tiles[1] = tile1;
    m->tiles[2] = tile2;
    m->tiles[3] = tile3;
    m->size = 4;
    m->from_player = state.current_player;
    m->called_index = 0;

    next.locations[last].zone = CJ4_ZONE_MELD;
    next.locations[last].owner = player;
    next.locations[tile1].zone = CJ4_ZONE_MELD;
    next.locations[tile1].owner = player;
    next.locations[tile2].zone = CJ4_ZONE_MELD;
    next.locations[tile2].owner = player;
    next.locations[tile3].zone = CJ4_ZONE_MELD;
    next.locations[tile3].owner = player;

    /* Post-process the call (reset draw_tile, mark discard consumed) */
    cj4_state_call_post_process(&next);

    /* Defer dora increase and rinshan draw to resolve step
       set current_player to the player who made the minkan so resolve draws to them */
    next.current_player = player;
    next.phase = CJ4_PHASE_ANKAN_RESOLVE;

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

    const cj4_location *l1 = cj4_tile_location_const(state, tile1);
    const cj4_location *l2 = cj4_tile_location_const(state, tile2);
    const cj4_location *l3 = cj4_tile_location_const(state, tile3);
    const cj4_location *l4 = cj4_tile_location_const(state, tile4);

    if (l1->zone != CJ4_ZONE_HAND || l1->owner != player)
        return false;
    if (l2->zone != CJ4_ZONE_HAND || l2->owner != player)
        return false;
    if (l3->zone != CJ4_ZONE_HAND || l3->owner != player)
        return false;
    if (l4->zone != CJ4_ZONE_HAND || l4->owner != player)
        return false;

    return true;
}

cj4_mahjong
cj4_do_ankan(const cj4_mahjong state, cj4_tile_id tile1, cj4_tile_id tile2, cj4_tile_id tile3, cj4_tile_id tile4)
{
    assert(cj4_can_ankan_with_tile(&state, tile1, tile2, tile3, tile4));
    cj4_mahjong next = state;
    cj4_player player = state.current_player;

    cj4_meld *m = &next.melds[player][next.meld_count[player]++];
    m->type = CJ4_MELD_ANKAN;
    m->tiles[0] = tile1;
    m->tiles[1] = tile2;
    m->tiles[2] = tile3;
    m->tiles[3] = tile4;
    m->size = 4;
    m->from_player = player;
    m->called_index = CJ4_CALLED_INDEX_NONE;

    next.locations[tile1].zone = CJ4_ZONE_MELD;
    next.locations[tile1].owner = player;
    next.locations[tile2].zone = CJ4_ZONE_MELD;
    next.locations[tile2].owner = player;
    next.locations[tile3].zone = CJ4_ZONE_MELD;
    next.locations[tile3].owner = player;
    next.locations[tile4].zone = CJ4_ZONE_MELD;
    next.locations[tile4].owner = player;

    cj4_state_call_post_process(&next);

    cj4_state_draw_dead_wall_tile(&next, player);

    cj4_state_add_dora_indicator(&next);

    next.current_player = player;

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
    const cj4_location *loc = cj4_tile_location_const(state, tile);
    if (loc->zone != CJ4_ZONE_HAND || loc->owner != player)
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

            next.locations[tile].zone = CJ4_ZONE_MELD;
            next.locations[tile].owner = player;

            break;
        }
    }

    /* Do not add dora or draw here; resolve in separate phase */
    /* current_player stays unchanged for kakan */
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

    next.phase = CJ4_PHASE_DRAW;

    return next;
}
