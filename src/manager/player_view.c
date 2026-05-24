#include "cjong4/manager/manager.h"

#include "state_query.h"

#include <string.h>

cj4_player_view
cj4m_make_player_view(
    const cj4_mahjong *state,
    cj4_player player)
{
    cj4_player_view view;

    memset(&view, 0, sizeof(view));

    view.player = player;
    view.phase = state->phase;
    view.current_player = state->current_player;
    view.dealer = state->dealer;
    view.round_wind = state->round_wind;
    view.temporary_furiten = state->temporary_furiten[player];
    view.riichi_furiten = state->riichi_furiten[player];
    view.first_turn_uninterrupted = state->first_turn_uninterrupted;
    view.draw_tile = CJ4_TILE_ID_INVALID;
    view.last_discard = cj4_get_last_discard_tile(state);
    view.pending_kakan_tile = state->phase == CJ4_PHASE_KAKAN_RESOLVE
                                  ? state->pending_kakan_tile
                                  : CJ4_TILE_ID_INVALID;

    memcpy(view.scores, state->scores, sizeof(view.scores));
    memcpy(view.is_riichi, state->is_riichi, sizeof(view.is_riichi));
    memcpy(view.discards, state->discards, sizeof(view.discards));
    view.discard_count = state->discard_count;
    memcpy(view.melds, state->melds, sizeof(view.melds));
    memcpy(view.meld_count, state->meld_count, sizeof(view.meld_count));

    for (cj4_tile_id tile = CJ4_TILE_ID_MIN; tile <= CJ4_TILE_ID_MAX; ++tile)
    {
        const cj4_location *location = cj4_tile_location_const(state, tile);

        if (location->zone != CJ4_ZONE_HAND || location->owner != player)
            continue;

        if (view.hand_count < CJ4M_MAX_HAND_TILES)
            view.hand[view.hand_count++] = tile;
    }

    if (state->draw_tile != CJ4_TILE_ID_INVALID)
    {
        const cj4_location *draw_location =
            cj4_tile_location_const(state, state->draw_tile);

        if (draw_location->zone == CJ4_ZONE_HAND &&
            draw_location->owner == player)
        {
            view.draw_tile = state->draw_tile;
        }
    }

    return view;
}
