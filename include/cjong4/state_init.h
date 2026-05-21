#ifndef CJ4_STATE_INIT_H
#define CJ4_STATE_INIT_H

#include "state.h"
#include "rules.h"

#ifdef __cplusplus
extern "C" {
#endif

cj4_mahjong
cj4_create_initial_state(
    const cj4_tile_id wall[CJ4_TILE_ID_COUNT],
    const cj4_rules* rules
);

#ifdef __cplusplus
}
#endif

#endif /* CJ4_STATE_INIT_H */