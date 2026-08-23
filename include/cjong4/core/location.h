#ifndef CJ4_LOCATION_H
#define CJ4_LOCATION_H

#include <stdbool.h>
#include <stdint.h>

#include "player.h"
#include "tile.h"

#ifdef __cplusplus
extern "C"
{
#endif

    enum
    {
        CJ4_LOCATION_BYTE_COUNT = 4,
        CJ4_LOCATION_NONE = 0xff,
        CJ4_DISCARD_INDEX_MAX = 30,
        CJ4_DISCARD_INDEX_NONE = CJ4_DISCARD_INDEX_MAX + 1,
        CJ4_DISCARD_HISTORY_MAX = 85,
        CJ4_DISCARD_HISTORY_INDEX_NONE = CJ4_DISCARD_HISTORY_MAX + 1,
        CJ4_MELD_GROUP_MAX = 3,
        CJ4_MELD_GROUP_NONE = CJ4_MELD_GROUP_MAX + 1,

        CJ4_LOCATION_PLAYER_SHIFT = 5,
        CJ4_LOCATION_PLAYER_MASK = 0x60,

        CJ4_LOCATION_DISCARD_INDEX_MASK = 0x1f,
        CJ4_LOCATION_DISCARD_TSUMOGIRI_FLAG = 0x80,

        CJ4_LOCATION_PLACEMENT_HAND_MASK = 0x9f,
        CJ4_LOCATION_PLACEMENT_MELD_FLAG = 0x80,
        CJ4_LOCATION_MELD_GROUP_SHIFT = 3,
        CJ4_LOCATION_MELD_GROUP_MASK = 0x18,
        CJ4_LOCATION_MELD_TYPE_MASK = 0x07,

        CJ4_LOCATION_DISCARD_HISTORY_INDEX_MASK = 0x7f,
        CJ4_LOCATION_DISCARD_RIICHI_FLAG = 0x80
    };

    typedef enum
    {
        CJ4_MELD_CHI = 0,
        CJ4_MELD_PON = 1,
        CJ4_MELD_MINKAN = 2,
        CJ4_MELD_ANKAN = 3,
        CJ4_MELD_KAKAN = 4,
        CJ4_MELD_INVALID = 7
    } cj4_meld_type;

    /* Canonical, endian-independent location of one physical tile. */
    typedef struct
    {
        uint8_t wall;
        uint8_t discard;
        uint8_t placement;
        uint8_t discard_history;
    } cj4_location;

    static inline bool
    cj4_location_is_unknown(
        const cj4_location *location)
    {
        return location && location->wall == CJ4_LOCATION_NONE &&
               location->discard == CJ4_LOCATION_NONE &&
               location->placement == CJ4_LOCATION_NONE &&
               location->discard_history == CJ4_LOCATION_NONE;
    }

    static inline bool
    cj4_location_is_discard(
        uint8_t discard)
    {
        return (discard & CJ4_LOCATION_DISCARD_INDEX_MASK) <=
               CJ4_DISCARD_INDEX_MAX;
    }

    static inline cj4_player
    cj4_location_discard_player(
        uint8_t discard)
    {
        if (!cj4_location_is_discard(discard))
            return CJ4_PLAYER_COUNT;
        return (cj4_player)((discard & CJ4_LOCATION_PLAYER_MASK) >>
                            CJ4_LOCATION_PLAYER_SHIFT);
    }

    static inline uint8_t
    cj4_location_discard_index(
        uint8_t discard)
    {
        if (!cj4_location_is_discard(discard))
            return CJ4_DISCARD_INDEX_NONE;
        return discard & CJ4_LOCATION_DISCARD_INDEX_MASK;
    }

    static inline bool
    cj4_location_discard_is_tsumogiri(
        uint8_t discard)
    {
        if (!cj4_location_is_discard(discard))
            return false;
        return (discard & CJ4_LOCATION_DISCARD_TSUMOGIRI_FLAG) != 0;
    }

    static inline bool
    cj4_location_is_hand(
        uint8_t placement)
    {
        return (placement & CJ4_LOCATION_PLACEMENT_HAND_MASK) == 0;
    }

    static inline bool
    cj4_location_is_meld(
        uint8_t placement)
    {
        return (placement & CJ4_LOCATION_PLACEMENT_MELD_FLAG) != 0 &&
               (placement & CJ4_LOCATION_MELD_TYPE_MASK) <= CJ4_MELD_KAKAN;
    }

    static inline cj4_player
    cj4_location_placement_player(
        uint8_t placement)
    {
        if (!cj4_location_is_hand(placement) &&
            !cj4_location_is_meld(placement))
            return CJ4_PLAYER_COUNT;
        return (cj4_player)((placement & CJ4_LOCATION_PLAYER_MASK) >>
                            CJ4_LOCATION_PLAYER_SHIFT);
    }

    static inline uint8_t
    cj4_location_meld_group(
        uint8_t placement)
    {
        if (!cj4_location_is_meld(placement))
            return CJ4_MELD_GROUP_NONE;
        return (placement & CJ4_LOCATION_MELD_GROUP_MASK) >>
               CJ4_LOCATION_MELD_GROUP_SHIFT;
    }

    static inline cj4_meld_type
    cj4_location_meld_type(
        uint8_t placement)
    {
        if (!cj4_location_is_meld(placement))
            return CJ4_MELD_INVALID;
        return (cj4_meld_type)(placement & CJ4_LOCATION_MELD_TYPE_MASK);
    }

    static inline bool
    cj4_location_is_discard_history(
        uint8_t history)
    {
        return (history & CJ4_LOCATION_DISCARD_HISTORY_INDEX_MASK) <=
               CJ4_DISCARD_HISTORY_MAX;
    }

    static inline uint8_t
    cj4_location_discard_history_index(
        uint8_t history)
    {
        if (!cj4_location_is_discard_history(history))
            return CJ4_DISCARD_HISTORY_INDEX_NONE;
        return history & CJ4_LOCATION_DISCARD_HISTORY_INDEX_MASK;
    }

    static inline bool
    cj4_location_discard_is_riichi(
        uint8_t history)
    {
        if (!cj4_location_is_discard_history(history))
            return false;
        return (history & CJ4_LOCATION_DISCARD_RIICHI_FLAG) != 0;
    }

    static inline cj4_location
    cj4_location_get(
        const cj4_location locations[CJ4_TILE_ID_COUNT],
        cj4_tile_id tile)
    {
        const cj4_location unknown = {
            CJ4_LOCATION_NONE,
            CJ4_LOCATION_NONE,
            CJ4_LOCATION_NONE,
            CJ4_LOCATION_NONE};

        if (!locations || !cj4_tile_id_is_valid(tile))
            return unknown;

        return locations[tile];
    }

#if defined(__cplusplus)
    static_assert(sizeof(cj4_location) == CJ4_LOCATION_BYTE_COUNT,
                  "cj4_location must be 4 bytes");
#else
_Static_assert(sizeof(cj4_location) == CJ4_LOCATION_BYTE_COUNT,
               "cj4_location must be 4 bytes");
#endif

#ifdef __cplusplus
}
#endif

#endif /* CJ4_LOCATION_H */
