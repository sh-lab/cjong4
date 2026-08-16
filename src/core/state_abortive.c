#include "state_abortive.h"

#include "state_ops.h"
#include "state_query.h"

#include <assert.h>

static uint8_t
cj4_count_distinct_yaochu_in_hand(
    const cj4_mahjong *state,
    cj4_player player)
{
    uint8_t seen[CJ4_TILE_TYPE_COUNT] = {0};
    uint8_t count = 0;

    for (cj4_tile_id tile = CJ4_TILE_ID_MIN; tile <= CJ4_TILE_ID_MAX; ++tile)
    {
        const cj4_location *loc = cj4_tile_location_const(state, tile);
        cj4_tile_type type;

        if (!cj4_location_is_hand(loc->placement) ||
            cj4_location_placement_player(loc->placement) != player)
            continue;

        type = cj4_tile_get_type(tile);
        if (!cj4_tile_type_is_yaochu(type) || seen[type])
            continue;

        seen[type] = 1;
        count++;
    }

    return count;
}

bool
cj4_can_kyuushu_kyuuhai(
    const cj4_mahjong *state,
    const cj4_rules *rules)
{
    cj4_player player;

    if (!state || !rules || !rules->abortive_kyuushu_kyuuhai)
        return false;

    if (cj4_state_phase(state) != CJ4_PHASE_DRAW ||
        !cj4_state_first_turn(state) ||
        state->draw_tile == CJ4_TILE_ID_INVALID)
    {
        return false;
    }

    player = cj4_state_current_player(state);
    if (cj4_state_draw_turn(state, player) != 1)
        return false;

    return cj4_count_distinct_yaochu_in_hand(state, player) >= 9;
}

cj4_mahjong
cj4_do_kyuushu_kyuuhai(
    const cj4_mahjong state)
{
    cj4_mahjong next = state;

    assert(cj4_state_phase(&state) == CJ4_PHASE_DRAW);
    assert(state.draw_tile != CJ4_TILE_ID_INVALID);

    cj4_state_clear_draw_tile(&next);
    cj4_state_finish_abortive_draw(
        &next,
        CJ4_ABORTIVE_DRAW_KYUUSHU_KYUUHAI);

    return next;
}
