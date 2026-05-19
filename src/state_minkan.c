#include "state_minkan.h"
#include "state_query.h"
#include "state_ops.h"

bool
cj4_can_minkan(const cj4_mahjong *state, cj4_player player)
{
    if (state->phase != CJ4_PHASE_DISCARD)
    {
        return false;
    }
    if (player == state->current_player)
    {
        return false;
    }

    cj4_tile_type last_discard_tile_type = cj4_tile_get_type(cj4_get_last_discard_tile(state));

    if (cj4_count_hand(state, player, last_discard_tile_type) < 3)
    {
        return false;
    }

    return true;
}

bool
cj4_can_minkan_with_tile(const cj4_mahjong *state, cj4_player player, cj4_tile_id tile1, cj4_tile_id tile2, cj4_tile_id tile3)
{
    if (!cj4_can_minkan(state, player))
    {
        return false;
    }

    cj4_tile_id last = cj4_get_last_discard_tile(state);
    cj4_tile_type type = cj4_tile_get_type(last);
    if (cj4_tile_get_type(tile1) != type ||
        cj4_tile_get_type(tile2) != type ||
        cj4_tile_get_type(tile3) != type)
    {
        return false;
    } 

    const cj4_location *l1 = cj4_tile_location_const(state, tile1);
    const cj4_location *l2 = cj4_tile_location_const(state, tile2);
    const cj4_location *l3 = cj4_tile_location_const(state, tile3); 
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

    cj4_state_call_post_process(&next);

    cj4_state_draw_dead_wall_tile(&next, player);

    cj4_state_add_dora_indicator(&next);

    next.current_player = player;

    next.phase = CJ4_PHASE_DRAW;

    return next;
}