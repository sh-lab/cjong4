#include "cjong4/player/hand_analysis.h"

#include "hand_analysis_internal.h"
#include "state_query.h"

#include <string.h>

static bool
cj4p_hand_analysis_input_from_view(
    const cj4_player_view *view,
    cj4_hand_analysis_input *out_input)
{
    cj4_meld_list melds;

    if (!view || view->player >= CJ4_PLAYER_COUNT || !out_input)
        return false;

    memset(out_input, 0, sizeof(*out_input));

    for (uint16_t tile = 0; tile < CJ4_TILE_ID_COUNT; ++tile)
    {
        cj4_tile_type type = cj4_tile_get_type((cj4_tile_id)tile);
        const cj4_location *location = &view->locations[tile];

        if (cj4_location_is_hand(location->placement) &&
            cj4_location_placement_player(location->placement) == view->player)
        {
            out_input->concealed[type]++;
        }

        if (!cj4_location_is_unknown(location))
            out_input->visible[type]++;
    }

    melds = cj4_location_collect_melds(view->locations, view->player);
    out_input->meld_count = melds.count;
    return true;
}

static bool
cj4p_view_player_owns_tile(
    const cj4_player_view *view,
    cj4_tile_id tile)
{
    return view && cj4_tile_id_is_valid(tile) &&
           cj4_location_is_hand(view->locations[tile].placement) &&
           cj4_location_placement_player(view->locations[tile].placement) ==
               view->player;
}

bool
cj4p_calculate_shanten(
    const cj4_player_view *view,
    cj4_shanten_result *out_result)
{
    cj4_hand_analysis_input input;

    if (!cj4p_hand_analysis_input_from_view(view, &input))
        return false;
    return cj4_hand_analysis_calculate(&input, out_result);
}

bool
cj4p_calculate_shanten_after_discard(
    const cj4_player_view *view,
    cj4_tile_id discard,
    cj4_shanten_result *out_result)
{
    cj4_hand_analysis_input input;

    if (!cj4p_view_player_owns_tile(view, discard) ||
        !cj4p_hand_analysis_input_from_view(view, &input))
    {
        return false;
    }

    return cj4_hand_analysis_calculate_after_discard(
        &input,
        cj4_tile_get_type(discard),
        out_result);
}

bool
cj4p_is_shape_tenpai(
    const cj4_player_view *view)
{
    cj4_shanten_result result;

    return cj4p_calculate_shanten(view, &result) &&
           (result.standard == 0 ||
            result.chiitoitsu == 0 ||
            result.kokushi == 0);
}

bool
cj4p_collect_waiting_tile_types(
    const cj4_player_view *view,
    cj4_waiting_tile_types *out_waits)
{
    cj4_hand_analysis_input input;

    if (!cj4p_hand_analysis_input_from_view(view, &input))
        return false;
    return cj4_hand_analysis_collect_waits(&input, out_waits);
}

bool
cj4p_collect_waiting_tile_types_after_discard(
    const cj4_player_view *view,
    cj4_tile_id discard,
    cj4_waiting_tile_types *out_waits)
{
    cj4_hand_analysis_input input;

    if (!cj4p_view_player_owns_tile(view, discard) ||
        !cj4p_hand_analysis_input_from_view(view, &input))
    {
        return false;
    }

    return cj4_hand_analysis_collect_waits_after_discard(
        &input,
        cj4_tile_get_type(discard),
        out_waits);
}
