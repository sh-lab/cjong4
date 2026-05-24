#ifndef CJ4_STATE_PASS_H
#define CJ4_STATE_PASS_H

#include "rules.h"
#include "state.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    bool
    cj4_can_pass(const cj4_mahjong state);

    cj4_mahjong
    cj4_do_pass(
        const cj4_mahjong state,
        const cj4_rules *rules);

#ifdef __cplusplus
}
#endif

#endif /* CJ4_STATE_PASS_H */