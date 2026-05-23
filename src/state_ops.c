#include "state_ops.h"
#include "state_internal.h"
#include "state_query.h"

#include <assert.h>

bool
cj4_state_has_last_discard(const cj4_mahjong *state)
{
    return state->discard_count > 0 &&
           cj4_get_last_discard_tile(state) != CJ4_TILE_ID_INVALID;
}

bool
cj4_state_can_claim_discard(
    const cj4_mahjong *state,
    cj4_player player)
{
    return state->phase == CJ4_PHASE_DISCARD &&
           player != state->current_player &&
           cj4_state_has_last_discard(state);
}

bool
cj4_state_tile_is_in_hand(
    const cj4_mahjong *state,
    cj4_player player,
    cj4_tile_id tile)
{
    if (tile > CJ4_TILE_ID_MAX)
        return false;

    const cj4_location *loc = cj4_tile_location_const(state, tile);
    return loc->zone == CJ4_ZONE_HAND && loc->owner == player;
}

void
cj4_state_set_location(
    cj4_mahjong *state,
    cj4_tile_id tile,
    cj4_zone zone,
    cj4_player owner)
{
    state->locations[tile].zone = zone;
    state->locations[tile].owner = owner;
}

cj4_tile_id
cj4_state_draw_tile(
    cj4_mahjong *state,
    cj4_player player)
{
    cj4_tile_id t = state->wall[state->wall_pos++];

    cj4_state_set_location(state, t, CJ4_ZONE_HAND, player);

    state->draw_count++;

    return t;
}

cj4_tile_id
cj4_state_draw_dead_wall_tile(
    cj4_mahjong *state,
    cj4_player player)
{
    cj4_tile_id t = state->wall[CJ4_RINSHAN_INDICES[state->dead_wall_draw_count++]];
    cj4_state_set_location(state, t, CJ4_ZONE_HAND, player);

    state->draw_count++;

    return t;
}

void
cj4_state_clear_draw_tile(cj4_mahjong *state)
{
    state->draw_tile = CJ4_TILE_ID_INVALID;
}

void
cj4_state_record_discard(
    cj4_mahjong *state,
    cj4_tile_id tile,
    uint8_t is_tsumogiri)
{
    cj4_discard *d = &state->discards[state->discard_count++];

    cj4_state_set_location(state, tile, CJ4_ZONE_RIVER, state->current_player);

    d->tile = tile;
    d->player = state->current_player;
    d->is_active = 1;
    d->is_tsumogiri = is_tsumogiri;
}

void
cj4_state_consume_last_discard(cj4_mahjong *state)
{
    if (state->discard_count == 0)
        return;

    state->discards[state->discard_count - 1].is_active = 0;
}

void
cj4_state_add_meld(
    cj4_mahjong *state,
    cj4_player player,
    cj4_meld_type type,
    const cj4_tile_id *tiles,
    uint8_t size,
    cj4_player from_player,
    uint8_t called_index)
{
    assert(size == 3 || size == 4);
    assert(state->meld_count[player] < CJ4_MAX_MELDS);

    cj4_meld *m = &state->melds[player][state->meld_count[player]++];
    m->type = type;
    m->size = size;
    m->from_player = from_player;
    m->called_index = called_index;

    for (uint8_t i = 0; i < size; ++i)
    {
        m->tiles[i] = tiles[i];
        cj4_state_set_location(state, tiles[i], CJ4_ZONE_MELD, player);
    }
}

void
cj4_state_finish_open_call(
    cj4_mahjong *state,
    cj4_player player,
    cj4_phase next_phase)
{
    cj4_state_clear_draw_tile(state);
    cj4_state_consume_last_discard(state);
    state->current_player = player;
    state->phase = next_phase;
}

void
cj4_state_add_dora_indicator(cj4_mahjong *state)
{
    if (state->dora_indicators_count < CJ4_MAX_DORA)
        state->dora_indicators_count++;
}

void
cj4_state_finish_tsumo(
    cj4_mahjong *state,
    cj4_player winner,
    cj4_tile_id winning_tile)
{
    state->winner = winner;
    state->winners[0] = winner;
    state->winner_count = 1;
    state->winning_tile = winning_tile;
    state->phase = CJ4_PHASE_ROUND_END;
}

void
cj4_state_finish_multi_ron(
    cj4_mahjong *state,
    const cj4_player *players,
    uint8_t count,
    cj4_tile_id winning_tile)
{
    if (count > CJ4_PLAYER_COUNT)
        count = CJ4_PLAYER_COUNT;

    state->loser = state->current_player;
    state->winner_count = count;

    for (uint8_t i = 0; i < count; ++i)
        state->winners[i] = players[i];

    state->winner = state->winners[0];
    state->winning_tile = winning_tile;
    state->phase = CJ4_PHASE_ROUND_END;
}