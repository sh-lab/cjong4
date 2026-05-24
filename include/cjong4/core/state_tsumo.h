#ifndef CJ4_STATE_TSUMO_H
#define CJ4_STATE_TSUMO_H

#include "rules.h"
#include "state.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    bool cj4_can_tsumo(const cj4_mahjong *state, const cj4_rules *rules);

    cj4_mahjong cj4_do_tsumo(const cj4_mahjong state);

#ifdef __cplusplus
}
#endif

#endif /* CJ4_STATE_TSUMO_H */
