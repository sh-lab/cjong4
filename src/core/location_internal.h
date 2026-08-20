#ifndef CJ4_LOCATION_INTERNAL_H
#define CJ4_LOCATION_INTERNAL_H

#include "location.h"

#include <assert.h>

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

static inline uint8_t
cj4_location_make_discard_history(
    uint8_t index,
    bool is_riichi)
{
    assert(index <= CJ4_DISCARD_HISTORY_MAX);
    return (uint8_t)((is_riichi ? 0x80u : 0u) | index);
}

#endif /* CJ4_LOCATION_INTERNAL_H */
