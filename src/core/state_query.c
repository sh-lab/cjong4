#include "state_query.h"

uint8_t
cj4_count_hand(
    const cj4_mahjong *state,
    cj4_player player,
    cj4_tile_type type)
{
    uint8_t count = 0;
    for (uint8_t i = 0; i < CJ4_TILE_PER_TYPE; ++i)
    {
        cj4_tile_id id = type * CJ4_TILE_PER_TYPE + i;
        const cj4_location *loc = cj4_tile_location_const(state, id);
        if (loc->zone == CJ4_ZONE_HAND && loc->owner == player)
        {
            count++;
        }
    }
    return count;
}

cj4_tile_id
cj4_get_last_discard_tile(const cj4_mahjong *state)
{
    if (state->discard_count == 0)
    {
        return CJ4_TILE_ID_INVALID;
    }
    return state->discards[state->discard_count - 1].tile;
}