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

    view.player = player;
    view.phase = cj4_state_phase(state);
    view.current_player = cj4_state_current_player(state);
    view.dealer = state->dealer;
    view.round_wind = state->round_wind;
    view.temporary_furiten = (uint8_t)cj4_state_temporary_furiten(state, player);
    view.riichi_furiten = (uint8_t)cj4_state_riichi_furiten(state, player);
    view.first_turn_uninterrupted = (uint8_t)cj4_state_first_turn(state);
    view.draw_tile = CJ4_TILE_ID_INVALID;
    view.last_discard = cj4_get_last_discard_tile(state);
    view.pending_kakan_tile =
        cj4_state_phase(state) == CJ4_PHASE_KAKAN_RESOLVE &&
                cj4_tile_id_is_valid(state->pending_kakan_tile)
            ? state->pending_kakan_tile
            : CJ4_TILE_ID_INVALID;

    memcpy(view.scores, state->scores, sizeof(view.scores));
    for (uint8_t p = 0; p < CJ4_PLAYER_COUNT; ++p)
        view.is_riichi[p] = (uint8_t)cj4_state_is_riichi(state, (cj4_player)p);

    for (uint8_t i = 0; i < state->dora_indicators_count &&
                        i < CJ4M_MAX_DORA_INDICATORS;
         ++i)
    {
        cj4_tile_id indicator = cj4_get_wall_tile(state, CJ4_DORA_INDICES[i]);

        if (cj4_tile_id_is_valid(indicator))
            view.dora_indicators[view.dora_indicators_count++] = indicator;
    }

    view.discard_count =
        cj4_location_collect_discards(state, view.discards);
    for (uint8_t p = 0; p < CJ4_PLAYER_COUNT; ++p)
        view.meld_count[p] = cj4_location_collect_melds(
            state,
            (cj4_player)p,
            view.melds[p]);

    view.hand_count = cj4_location_collect_hand(state, player, view.hand);

    if (cj4_tile_id_is_valid(state->draw_tile))
    {
        const cj4_location *draw_location =
            cj4_tile_location_const(state, state->draw_tile);

        if (cj4_location_is_hand(draw_location->placement) &&
            cj4_location_placement_player(draw_location->placement) == player)
        {
            view.draw_tile = state->draw_tile;
        }
    }

    return view;
}
