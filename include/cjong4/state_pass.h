#ifndef CJ4_STATE_PASS_H
#define CJ4_STATE_PASS_H

#include "state.h"
#include "rules.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


bool
cj4_can_pass(const cj4_mahjong state);

cj4_mahjong
cj4_do_pass(const cj4_mahjong state);

#ifdef __cplusplus
}
#endif

#endif /* CJ4_STATE_PASS_H */