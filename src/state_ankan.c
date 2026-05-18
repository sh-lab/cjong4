#include "state_ankan.h"
#include "state_query.h"
#include "state_ops.h"
#include <assert.h>

bool
cj4_can_ankan(const cj4_mahjong *state)
{
    cj4_player player = state->current_player;
    if (state->phase != CJ4_PHASE_DRAW)
    {
        return false;
    }

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
    {
        return false;
    }

    cj4_player player = state->current_player;

    cj4_tile_type type = cj4_tile_get_type(tile1);

    if (cj4_tile_get_type(tile2) != type ||
        cj4_tile_get_type(tile3) != type ||
        cj4_tile_get_type(tile4) != type)
    {
        return false;
    }

    /* ensure distinct tile ids */
    if (tile1 == tile2 || tile1 == tile3 || tile1 == tile4 ||
        tile2 == tile3 || tile2 == tile4 || tile3 == tile4)
    {
        return false;
    }

    const cj4_location *l1 = cj4_tile_location_const(state, tile1);
    const cj4_location *l2 = cj4_tile_location_const(state, tile2);
    const cj4_location *l3 = cj4_tile_location_const(state, tile3);
    const cj4_location *l4 = cj4_tile_location_const(state, tile4);

    if (l1->zone != CJ4_ZONE_HAND || l1->owner != player)
    {
        return false;
    }
    if (l2->zone != CJ4_ZONE_HAND || l2->owner != player)
    {
        return false;
    }
    if (l3->zone != CJ4_ZONE_HAND || l3->owner != player)
    {
        return false;
    }
    if (l4->zone != CJ4_ZONE_HAND || l4->owner != player)
    {
        return false;
    }

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

    cj4_state_draw_dead_wall_tile(&next, player);

    cj4_state_add_dora_indicator(&next);

    next.current_player = player;

    next.phase = CJ4_PHASE_DRAW;

    return next;
}
