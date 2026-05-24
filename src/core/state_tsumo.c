#include "state_tsumo.h"
#include "state_ops.h"
#include "state_query.h"
#include "state_yaku.h"
#include <assert.h>

bool cj4_can_tsumo(const cj4_mahjong *state, const cj4_rules *rules)
{
    if (state->phase != CJ4_PHASE_DRAW)
        return false;

    if (state->draw_tile == CJ4_TILE_ID_INVALID)
        return false;

    return cj4_has_yaku(state, state->current_player, rules);
}

cj4_mahjong cj4_do_tsumo(const cj4_mahjong state)
{

    cj4_mahjong next = state;
    cj4_state_finish_tsumo(&next, state.current_player, state.draw_tile);

    return next;
}
