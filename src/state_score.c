#include "state_score.h"

#include "state_query.h"
#include "state_yaku.h"

static cj4_tile_id
cj4_score_find_test_tile(const cj4_mahjong *state, cj4_tile_type type, cj4_player player)
{
    for (uint8_t i = 0; i < CJ4_TILE_PER_TYPE; ++i)
    {
        cj4_tile_id tile = cj4_tile_make(type, i);
        const cj4_location *loc = cj4_tile_location_const(state, tile);

        if (!(loc->zone == CJ4_ZONE_HAND && loc->owner == player))
            return tile;
    }

    return CJ4_TILE_ID_INVALID;
}

bool
cj4_player_is_tenpai(
    const cj4_mahjong *state,
    cj4_player player,
    const cj4_rules *rules)
{
    for (cj4_tile_type type = CJ4_TILE_TYPE_MIN;
         type <= CJ4_TILE_TYPE_MAX;
         ++type)
    {
        cj4_tile_id tile = cj4_score_find_test_tile(state, type, player);

        if (tile == CJ4_TILE_ID_INVALID)
            continue;

        cj4_mahjong tmp = *state;
        tmp.locations[tile].zone = CJ4_ZONE_HAND;
        tmp.locations[tile].owner = player;
        tmp.current_player = player;
        tmp.draw_tile = tile;
        tmp.phase = CJ4_PHASE_DRAW;
        tmp.winning_from_chankan = 0;
        tmp.pending_kakan_tile = CJ4_TILE_ID_INVALID;

        if (cj4_has_yaku(&tmp, player, rules))
            return true;
    }

    return false;
}
