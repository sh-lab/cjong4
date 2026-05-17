#ifndef CJ4_STATE_QUERY_H
#define CJ4_STATE_QUERY_H

#include "state.h"


#ifdef __cplusplus
extern "C" {
#endif

uint8_t
cj4_count_hand(
    const cj4_mahjong state,cj4_player player,
    cj4_tile_type type
);

cj4_tile_id
cj4_get_last_discard_tile(const cj4_mahjong state);

#ifdef __cplusplus
}
#endif

#endif /* CJ4_STATE_QUERY_H */