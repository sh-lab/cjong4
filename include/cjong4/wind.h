#ifndef CJONG4_WIND_H
#define CJONG4_WIND_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Wind identifier.
 *
 * cj4_wind:
 *   Wind index identifier.
 *   Range: 0-3
 *
 */
typedef uint8_t cj4_wind;

enum cj4_wind_e {
    CJ4_WIND_EAST   = 0, /* East */
    CJ4_WIND_SOUTH  = 1, /* South */
    CJ4_WIND_WEST   = 2, /* West */
    CJ4_WIND_NORTH  = 3, /* North */

    CJ4_WIND_COUNT  = 4,
};

#ifdef __cplusplus
}
#endif

#endif /* CJONG4_WIND_H */
