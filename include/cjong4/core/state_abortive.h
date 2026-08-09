#ifndef CJ4_STATE_ABORTIVE_H
#define CJ4_STATE_ABORTIVE_H

#include <stdbool.h>

#include "rules.h"
#include "state.h"

#ifdef __cplusplus
extern "C"
{
#endif

    bool
    cj4_can_kyuushu_kyuuhai(
        const cj4_mahjong *state,
        const cj4_rules *rules);

    cj4_mahjong
    cj4_do_kyuushu_kyuuhai(const cj4_mahjong state);

#ifdef __cplusplus
}
#endif

#endif /* CJ4_STATE_ABORTIVE_H */
