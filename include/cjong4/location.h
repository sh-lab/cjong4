#ifndef CJ4_LOCATION_H
#define CJ4_LOCATION_H

#include <stdint.h>

#include "player.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Zone of a tile (current location category)
 */
typedef enum {
    CJ4_ZONE_WALL,   /* in wall (live wall or dead wall) */
    CJ4_ZONE_HAND,   /* in player's hand */
    CJ4_ZONE_RIVER,  /* in river (discarded, still active) */
    CJ4_ZONE_MELD    /* part of a meld (chi/pon/kan) */
} cj4_zone;

/*
 * Location of a tile
 *
 * Describes where a tile currently is.
 */
typedef struct {
    cj4_zone   zone;   /* which zone the tile is in */
    cj4_player owner;  /* owner player (undefined for wall) */
} cj4_location;

#ifdef __cplusplus
}
#endif

#endif /* CJ4_LOCATION_H */