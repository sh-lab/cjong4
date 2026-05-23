#include "state_yaku.h"
#include "hand_check.h"

#include <stdbool.h>

bool cj4_has_yaku(
    const cj4_mahjong *state,
    cj4_player player,
    const cj4_rules *rules
)
{
    if (!cj4_is_complete_hand(state, player))
        return false;

    if (state->is_riichi[player])
        return true;

    if (rules && rules->ippatsu && state->is_ippatsu[player])
        return true;

    if (state->meld_count[player] == 0)
        return true;

    return false;
}
