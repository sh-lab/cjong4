#include "state_kakan.h"
#include "state_query.h"
#include "state_ops.h"
#include "state_internal.h"
#include <assert.h>

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
    next.phase = CJ4_PHASE_KAN_RESOLVE;

    return next;
}

cj4_mahjong
cj4_resolve_kan(const cj4_mahjong state)
{
    assert(state.phase == CJ4_PHASE_KAN_RESOLVE);
    cj4_mahjong next = state;
    cj4_player player = state.current_player;

    cj4_state_add_dora_indicator(&next);

    /* Draw rinshan tile to hand of current player */
    cj4_tile_id t = cj4_state_draw_dead_wall_tile(&next, player);
    next.draw_tile = t;

    next.phase = CJ4_PHASE_DRAW;

    return next;
}
