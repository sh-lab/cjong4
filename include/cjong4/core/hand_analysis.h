#ifndef CJ4_HAND_ANALYSIS_H
#define CJ4_HAND_ANALYSIS_H

#include <stdbool.h>
#include <stdint.h>

#include "state.h"

#ifdef __cplusplus
extern "C"
{
#endif

    enum
    {
        CJ4_SHANTEN_NOT_APPLICABLE = 127
    };

    typedef struct
    {
        int8_t standard;
        int8_t chiitoitsu;
        int8_t kokushi;
    } cj4_shanten_result;

    typedef struct
    {
        uint8_t types[CJ4_TILE_TYPE_COUNT];
        uint8_t count[CJ4_TILE_TYPE_COUNT];
    } cj4_waiting_tile_types;

    /*
     * A 13-tile-equivalent hand is evaluated as-is. For a 14-tile-equivalent
     * hand, each field is the minimum after discarding one concealed tile.
     * Chiitoitsu and kokushi are not applicable to an open hand.
     */
    bool
    cj4_calculate_shanten(
        const cj4_mahjong *state,
        cj4_player player,
        cj4_shanten_result *out_result);

    bool
    cj4_calculate_shanten_after_discard(
        const cj4_mahjong *state,
        cj4_player player,
        cj4_tile_id discard,
        cj4_shanten_result *out_result);

    bool
    cj4_is_shape_tenpai(
        const cj4_mahjong *state,
        cj4_player player);

    /*
     * Waiting tiles are shape-only: yaku, furiten, and availability are not
     * considered. types[type] is 1 for a wait. count[type] is the number not
     * visible to the player (own hand plus public tiles and dora indicators).
     * A dead wait is therefore represented by types[type] == 1 and
     * count[type] == 0.
     */
    bool
    cj4_collect_waiting_tile_types(
        const cj4_mahjong *state,
        cj4_player player,
        cj4_waiting_tile_types *out_waits);

    bool
    cj4_collect_waiting_tile_types_after_discard(
        const cj4_mahjong *state,
        cj4_player player,
        cj4_tile_id discard,
        cj4_waiting_tile_types *out_waits);

#ifdef __cplusplus
}
#endif

#endif /* CJ4_HAND_ANALYSIS_H */
