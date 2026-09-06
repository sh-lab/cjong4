#ifndef CJ4P_HAND_ANALYSIS_H
#define CJ4P_HAND_ANALYSIS_H

#include <stdbool.h>

#include "cjong4/core/hand_analysis.h"
#include "cjong4/manager/player_view.h"

#ifdef __cplusplus
extern "C"
{
#endif

    bool
    cj4p_calculate_shanten(
        const cj4_player_view *view,
        cj4_shanten_result *out_result);

    bool
    cj4p_calculate_shanten_after_discard(
        const cj4_player_view *view,
        cj4_tile_id discard,
        cj4_shanten_result *out_result);

    bool
    cj4p_is_shape_tenpai(
        const cj4_player_view *view);

    bool
    cj4p_collect_waiting_tile_types(
        const cj4_player_view *view,
        cj4_waiting_tile_types *out_waits);

    bool
    cj4p_collect_waiting_tile_types_after_discard(
        const cj4_player_view *view,
        cj4_tile_id discard,
        cj4_waiting_tile_types *out_waits);

#ifdef __cplusplus
}
#endif

#endif /* CJ4P_HAND_ANALYSIS_H */
