#ifndef CJ4_ACTION_H
#define CJ4_ACTION_H

#include <stdint.h>

#include "player.h"
#include "tile.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /*
     * Action types.
     *
     * Represents player choices.
     */
    typedef enum
    {
        CJ4_ACTION_DISCARD,
        CJ4_ACTION_CHI,
        CJ4_ACTION_PON,
        CJ4_ACTION_ANKAN,
        CJ4_ACTION_MINKAN,
        CJ4_ACTION_KAKAN,
        CJ4_ACTION_RIICHI,
        CJ4_ACTION_TSUMO,
        CJ4_ACTION_RON,
        CJ4_ACTION_PASS
    } cj4_action_type;

    /*
     * Action.
     *
     * Represents a single decision made by a player.
     */
    typedef struct
    {
        cj4_action_type type;
        cj4_player player;

        cj4_tile_id tile; /* primary tile (e.g. discard, ron) */

        cj4_tile_id tiles[4]; /* used for chi/pon/kan */
        uint8_t tile_count;   /* number of tiles used */

    } cj4_action;

#ifdef __cplusplus
}
#endif

#endif /* CJ4_ACTION_H */