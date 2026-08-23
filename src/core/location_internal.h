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
    return (uint8_t)((is_tsumogiri
                          ? CJ4_LOCATION_DISCARD_TSUMOGIRI_FLAG
                          : 0u) |
                     ((uint8_t)player << CJ4_LOCATION_PLAYER_SHIFT) | index);
}

static inline uint8_t
cj4_location_make_hand(
    cj4_player player)
{
    assert(player < CJ4_PLAYER_COUNT);
    return (uint8_t)((uint8_t)player << CJ4_LOCATION_PLAYER_SHIFT);
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
    return (uint8_t)(CJ4_LOCATION_PLACEMENT_MELD_FLAG |
                     ((uint8_t)player << CJ4_LOCATION_PLAYER_SHIFT) |
                     (group << CJ4_LOCATION_MELD_GROUP_SHIFT) |
                     (uint8_t)type);
}

static inline uint8_t
cj4_location_make_discard_history(
    uint8_t index,
    bool is_riichi)
{
    assert(index <= CJ4_DISCARD_HISTORY_MAX);
    return (uint8_t)((is_riichi ? CJ4_LOCATION_DISCARD_RIICHI_FLAG : 0u) |
                     index);
}

#endif /* CJ4_LOCATION_INTERNAL_H */
