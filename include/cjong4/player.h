#ifndef CJ4_PLAYER_H
#define CJ4_PLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*
 * Player identifier.
 *
 * cj4_player:
 *   Player index identifier.
 *   Range: 0-3
 *
 * Notes:
 *   - Represents player identity within a 4-player game.
 *   - Used as an index for arrays.
 *   - Does NOT encode seat wind (east/south/west/north).
 */
typedef uint8_t cj4_player;

enum {
    CJ4_PLAYER_0 = 0,
    CJ4_PLAYER_1 = 1,
    CJ4_PLAYER_2 = 2,
    CJ4_PLAYER_3 = 3,

    CJ4_PLAYER_COUNT = 4
};

#ifdef __cplusplus
}
#endif

#endif /* CJ4_PLAYER_H */