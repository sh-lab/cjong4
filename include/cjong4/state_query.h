#ifndef CJ4_STATE_QUERY_H
#define CJ4_STATE_QUERY_H

#include "state.h"

#ifdef __cplusplus
extern "C"
{
#endif

    static inline const cj4_location *
    cj4_tile_location_const(const cj4_mahjong *state, cj4_tile_id tile)
    {
        return &state->locations[tile];
    }

    static inline cj4_player
    cj4_next_player(const cj4_mahjong *state)
    {
        return (cj4_player)((state->current_player + 1) % CJ4_PLAYER_COUNT);
    }

    uint8_t
    cj4_count_hand(
        const cj4_mahjong *state, cj4_player player,
        cj4_tile_type type);

    cj4_tile_id
    cj4_get_last_discard_tile(const cj4_mahjong *state);

#ifdef __cplusplus
}
#endif

#endif /* CJ4_STATE_QUERY_H */