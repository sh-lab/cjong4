#include "state_round.h"

#include "state_round_init.h"

#include <assert.h>

bool cj4_can_next_round(const cj4_mahjong state)
{
    return state.phase == CJ4_PHASE_SETTLE &&
           !state.settlement_should_end;
}

cj4_mahjong
cj4_do_next_round(
    const cj4_mahjong state,
    const cj4_tile_id wall[CJ4_TILE_ID_COUNT],
    const cj4_rules *rules)
{
    assert(cj4_can_next_round(state));
    assert(wall != 0);

    return cj4_state_create_round(
        wall,
        rules,
        state.scores,
        state.next_round_wind,
        state.next_dealer,
        state.honba,
        state.riichi_sticks);
}

bool cj4_can_game_end(const cj4_mahjong state)
{
    return state.phase == CJ4_PHASE_SETTLE &&
           state.settlement_should_end;
}

cj4_mahjong
cj4_do_game_end(const cj4_mahjong state)
{
    assert(cj4_can_game_end(state));

    cj4_mahjong next = state;
    next.phase = CJ4_PHASE_GAME_END;

    return next;
}
