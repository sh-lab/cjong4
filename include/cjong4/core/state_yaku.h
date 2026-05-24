#ifndef CJ4_STATE_YAKU_H
#define CJ4_STATE_YAKU_H

#include "player.h"
#include "rules.h"
#include "state.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    bool cj4_has_yaku(
        const cj4_mahjong *state,
        cj4_player player,
        const cj4_rules *rules);

#ifdef __cplusplus
}
#endif

#endif /* CJ4_STATE_YAKU_H */
