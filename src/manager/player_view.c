#include "cjong4/manager/manager.h"

#include "state_internal.h"
#include "state_query.h"

#include <string.h>

cj4_player_view
cj4m_make_player_view(
    const cj4_mahjong *state,
    cj4_player player)
{
    cj4_player_view view;

    memset(&view, 0, sizeof(view));
    memset(view.locations, CJ4_LOCATION_NONE, sizeof(view.locations));

    view.player = player;
    view.phase = cj4_state_phase(state);
    view.current_player = cj4_state_current_player(state);
    view.dealer = state->dealer;
    view.round_wind = state->round_wind;
    view.honba = state->honba;
    view.riichi_sticks = state->riichi_sticks;
    view.temporary_furiten = (uint8_t)cj4_state_temporary_furiten(state, player);
    view.riichi_furiten = (uint8_t)cj4_state_riichi_furiten(state, player);
    view.first_turn_uninterrupted = (uint8_t)cj4_state_first_turn(state);
    view.live_wall_remaining = cj4_live_wall_remaining(state);
    view.draw_tile = CJ4_TILE_ID_INVALID;
    view.last_discard = cj4_get_last_discard_tile(state);
    view.kan_tile = CJ4_TILE_ID_INVALID;

    if (view.phase == CJ4_PHASE_KAKAN_RESOLVE)
        view.kan_tile = state->pending_kakan_tile;
    else if (view.phase == CJ4_PHASE_ANKAN_RESOLVE)
        view.kan_tile = state->pending_ankan_tile;

    memcpy(view.scores, state->scores, sizeof(view.scores));
    for (uint8_t p = 0; p < CJ4_PLAYER_COUNT; ++p)
        view.is_riichi[p] = (uint8_t)cj4_state_is_riichi(state, (cj4_player)p);

    for (uint16_t tile = 0; tile < CJ4_TILE_ID_COUNT; ++tile)
    {
        const cj4_location *source = &state->locations[tile];
        cj4_location visible = *source;
        bool own_hand =
            cj4_location_is_hand(source->placement) &&
            cj4_location_placement_player(source->placement) == player;
        bool is_discard = cj4_location_is_discard(source->discard);
        bool is_meld = cj4_location_is_meld(source->placement);
        bool wall_visible = own_hand;

        if (is_discard &&
            (cj4_location_discard_is_tsumogiri(source->discard) ||
             cj4_location_discard_player(source->discard) == player))
        {
            wall_visible = true;
        }

        if (is_meld &&
            cj4_location_placement_player(source->placement) == player)
        {
            wall_visible = true;
        }

        for (uint8_t i = 0; i < state->dora_count &&
                            i < CJ4_MAX_DORA_INDICATORS;
             ++i)
        {
            if (source->wall == CJ4_DORA_INDICES[i])
                wall_visible = true;
        }

        if (!own_hand && !is_discard && !is_meld && !wall_visible)
            memset(&visible, CJ4_LOCATION_NONE, sizeof(visible));
        else if (!wall_visible)
            visible.wall = CJ4_LOCATION_NONE;

        view.locations[tile] = visible;
    }

    if (cj4_tile_id_is_valid(state->draw_tile))
    {
        cj4_location draw_location =
            cj4_location_get(state->locations, state->draw_tile);

        if (cj4_location_is_hand(draw_location.placement) &&
            cj4_location_placement_player(draw_location.placement) == player)
        {
            view.draw_tile = state->draw_tile;
        }
    }

    return view;
}
