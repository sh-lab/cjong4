#ifndef CJ4_LOCATION_H
#define CJ4_LOCATION_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "player.h"

#ifdef __cplusplus
extern "C"
{
#endif

    enum
    {
        CJ4_LOCATION_NONE = 0xff,
        CJ4_DISCARD_INDEX_MAX = 30,
        CJ4_DISCARD_HISTORY_MAX = 85,
        CJ4_MELD_GROUP_MAX = 3
    };

    typedef enum
    {
        CJ4_MELD_CHI,
        CJ4_MELD_PON,
        CJ4_MELD_MINKAN,
        CJ4_MELD_ANKAN,
        CJ4_MELD_KAKAN
    } cj4_meld_type;

    /* Canonical, endian-independent location of one physical tile. */
    typedef struct
    {
        uint8_t wall;
        uint8_t discard;
        uint8_t placement;
        uint8_t discard_history;
    } cj4_location;

    static inline uint8_t
    cj4_location_make_discard(
        cj4_player player,
        uint8_t index,
        bool is_tsumogiri)
    {
        assert(player < CJ4_PLAYER_COUNT);
        assert(index <= CJ4_DISCARD_INDEX_MAX);
        return (uint8_t)((is_tsumogiri ? 0x80u : 0u) |
                         ((uint8_t)player << 5) | index);
    }

    static inline cj4_player
    cj4_location_discard_player(
        uint8_t discard)
    {
        assert(discard != CJ4_LOCATION_NONE);
        return (cj4_player)((discard >> 5) & 0x03u);
    }

    static inline uint8_t
    cj4_location_discard_index(
        uint8_t discard)
    {
        assert(discard != CJ4_LOCATION_NONE);
        return discard & 0x1fu;
    }

    static inline bool
    cj4_location_discard_is_tsumogiri(
        uint8_t discard)
    {
        assert(discard != CJ4_LOCATION_NONE);
        return (discard & 0x80u) != 0;
    }

    static inline uint8_t
    cj4_location_make_hand(
        cj4_player player)
    {
        assert(player < CJ4_PLAYER_COUNT);
        return (uint8_t)((uint8_t)player << 5);
    }

    static inline uint8_t
    cj4_location_make_meld(
        cj4_player player,
        uint8_t group,
        cj4_meld_type type)
    {
        assert(player < CJ4_PLAYER_COUNT);
        assert(group <= CJ4_MELD_GROUP_MAX);
        assert(type <= CJ4_MELD_KAKAN);
        return (uint8_t)(0x80u | ((uint8_t)player << 5) |
                         (group << 3) | (uint8_t)type);
    }

    static inline bool
    cj4_location_is_hand(
        uint8_t placement)
    {
        return (placement & 0x9fu) == 0;
    }

    static inline bool
    cj4_location_is_meld(
        uint8_t placement)
    {
        return (placement & 0x80u) != 0 &&
               (placement & 0x07u) <= CJ4_MELD_KAKAN;
    }

    static inline cj4_player
    cj4_location_placement_player(
        uint8_t placement)
    {
        assert(placement != CJ4_LOCATION_NONE);
        return (cj4_player)((placement >> 5) & 0x03u);
    }

    static inline uint8_t
    cj4_location_meld_group(
        uint8_t placement)
    {
        assert(cj4_location_is_meld(placement));
        return (placement >> 3) & 0x03u;
    }

    static inline cj4_meld_type
    cj4_location_meld_type(
        uint8_t placement)
    {
        assert(cj4_location_is_meld(placement));
        return (cj4_meld_type)(placement & 0x07u);
    }

    static inline uint8_t
    cj4_location_make_discard_history(
        uint8_t index,
        bool is_riichi)
    {
        assert(index <= CJ4_DISCARD_HISTORY_MAX);
        return (uint8_t)((is_riichi ? 0x80u : 0u) | index);
    }

    static inline uint8_t
    cj4_location_discard_history_index(
        uint8_t history)
    {
        assert(history != CJ4_LOCATION_NONE);
        return history & 0x7fu;
    }

    static inline bool
    cj4_location_discard_is_riichi(
        uint8_t history)
    {
        assert(history != CJ4_LOCATION_NONE);
        return (history & 0x80u) != 0;
    }

#if defined(__cplusplus)
    static_assert(sizeof(cj4_location) == 4, "cj4_location must be 4 bytes");
#else
_Static_assert(sizeof(cj4_location) == 4, "cj4_location must be 4 bytes");
#endif

#ifdef __cplusplus
}
#endif

#endif /* CJ4_LOCATION_H */
