#include "state_pon.h"
#include "state_query.h"

bool cj4_can_pon(const cj4_mahjong *state, cj4_player player)
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

    if (cj4_count_hand(state, player, last_discard_tile_type) < 2)
    {
        return false;
    }

    return true;
}

bool cj4_can_pon_with_tile(const cj4_mahjong *state, cj4_player player, cj4_tile_id tile1, cj4_tile_id tile2)
{
    if (!cj4_can_pon(state, player))
    {
        return false;
    }

    cj4_tile_id last = cj4_get_last_discard_tile(state);
    cj4_tile_type type = cj4_tile_get_type(last);

    if (cj4_tile_get_type(tile1) != type ||
        cj4_tile_get_type(tile2) != type)
    {
        return false;
    }

    const cj4_location *l1 = cj4_tile_location_const(state, tile1);
    const cj4_location *l2 = cj4_tile_location_const(state, tile2);

    if (l1->zone != CJ4_ZONE_HAND || l1->owner != player)
    {
        return false;
    }

    if (l2->zone != CJ4_ZONE_HAND || l2->owner != player)
    {
        return false;
    }

    return true;
}

cj4_mahjong
cj4_do_pon(const cj4_mahjong state, cj4_player player, cj4_tile_id tile1, cj4_tile_id tile2)
{
    assert(cj4_can_pon_with_tile(&state, player, tile1, tile2));

    cj4_mahjong next = state;

    cj4_tile_id last = cj4_get_last_discard_tile(&state);

    cj4_meld *m = &next.melds[player][next.meld_count[player]++];
    m->type = CJ4_MELD_PON;
    m->tiles[0] = last;
    m->tiles[1] = tile1;
    m->tiles[2] = tile2;
    m->size = 3;
    m->from_player = state.current_player;
    m->called_index = 0;

    next.locations[last].zone = CJ4_ZONE_MELD;
    next.locations[last].owner = player;
    next.locations[tile1].zone = CJ4_ZONE_MELD;
    next.locations[tile1].owner = player;
    next.locations[tile2].zone = CJ4_ZONE_MELD;
    next.locations[tile2].owner = player;

    cj4_state_call_post_process(&next);

    next.current_player = player;

    next.phase = CJ4_PHASE_AFTER_CALL;

    return next;
}
