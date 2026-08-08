#ifndef CJ4_STATE_INTERNAL_H
#define CJ4_STATE_INTERNAL_H

#include "state.h"

#if defined(__cplusplus)
extern "C"
{
#endif

#define CJ4_MAX_DORA 5

    static const uint8_t CJ4_RINSHAN_INDICES[4] = {
        134,
        135,
        132,
        133};

    static const uint8_t CJ4_DORA_INDICES[5] = {
        130,
        128,
        126,
        124,
        122};

    static const uint8_t CJ4_URA_DORA_INDICES[5] = {
        131,
        129,
        127,
        125,
        123};

#if defined(__cplusplus)
}
#endif

#endif /* CJ4_STATE_INTERNAL_H */