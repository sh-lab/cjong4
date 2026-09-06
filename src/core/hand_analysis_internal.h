#ifndef CJ4_HAND_ANALYSIS_INTERNAL_H
#define CJ4_HAND_ANALYSIS_INTERNAL_H

#include "cjong4/core/hand_analysis.h"

typedef struct
{
    uint8_t concealed[CJ4_TILE_TYPE_COUNT];
    uint8_t visible[CJ4_TILE_TYPE_COUNT];
    uint8_t meld_count;
} cj4_hand_analysis_input;

bool
cj4_hand_analysis_calculate(
    const cj4_hand_analysis_input *input,
    cj4_shanten_result *out_result);

bool
cj4_hand_analysis_calculate_after_discard(
    const cj4_hand_analysis_input *input,
    cj4_tile_type discard,
    cj4_shanten_result *out_result);

bool
cj4_hand_analysis_collect_waits(
    const cj4_hand_analysis_input *input,
    cj4_waiting_tile_types *out_waits);

bool
cj4_hand_analysis_collect_waits_after_discard(
    const cj4_hand_analysis_input *input,
    cj4_tile_type discard,
    cj4_waiting_tile_types *out_waits);

#endif /* CJ4_HAND_ANALYSIS_INTERNAL_H */
