#include "state_score.h"

#include "hand_check.h"
#include "state_query.h"
#include "state_yaku.h"

bool
cj4_player_is_shape_tenpai(
    const cj4_mahjong *state,
    cj4_player player)
{
    uint8_t waits[CJ4_TILE_TYPE_COUNT];

    return cj4_collect_waiting_tile_types(state, player, waits) > 0;
}
