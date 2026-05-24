#ifndef CJ4_STATE_RON_H
#define CJ4_STATE_RON_H

#include <stdbool.h>
#include "state.h"
#include "rules.h"
#include "player.h"

#ifdef __cplusplus
extern "C" {
#endif

bool
cj4_can_ron(
    const cj4_mahjong* state,
    cj4_player player,
    const cj4_rules* rules
);

cj4_mahjong
cj4_do_ron_multi(
    const cj4_mahjong state,
    const cj4_player* players,
    int count,
    const cj4_rules *rules
);

#ifdef __cplusplus
}
#endif

#endif /* CJ4_STATE_RON_H */
