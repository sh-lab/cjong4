#ifndef CJ4_STATE_SETTLE_H
#define CJ4_STATE_SETTLE_H

#include <stdbool.h>

#include "state.h"
#include "rules.h"

#ifdef __cplusplus
extern "C" {
#endif

bool
cj4_can_settle(const cj4_mahjong state);

cj4_mahjong
cj4_do_settle(const cj4_mahjong state, const cj4_rules *rules);

#ifdef __cplusplus
}
#endif

#endif /* CJ4_STATE_SETTLE_H */
