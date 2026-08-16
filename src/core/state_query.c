#include "state_query.h"
#include "state_internal.h"

#include <string.h>

uint8_t
cj4_count_hand(
    const cj4_mahjong *state,
    cj4_player player,
    cj4_tile_type type)
{
    uint8_t count = 0;

    if (!state || player >= CJ4_PLAYER_COUNT ||
        !cj4_tile_type_is_valid(type))
        return 0;

    for (uint8_t i = 0; i < CJ4_TILE_PER_TYPE; ++i)
    {
        cj4_tile_id id = (cj4_tile_id)(type * CJ4_TILE_PER_TYPE + i);
        uint8_t placement = state->locations[id].placement;
        if (cj4_location_is_hand(placement) &&
            cj4_location_placement_player(placement) == player)
            count++;
    }
    return count;
}

cj4_tile_id
cj4_get_last_discard_tile(
    const cj4_mahjong *state)
{
    if (!state || state->discard_count == 0 ||
        !cj4_tile_id_is_valid(state->last_discard_tile))
        return CJ4_TILE_ID_INVALID;

    return state->last_discard_tile;
}

cj4_tile_id
cj4_get_wall_tile(
    const cj4_mahjong *state,
    uint8_t wall_position)
{
    if (!state || wall_position == CJ4_LOCATION_NONE)
        return CJ4_TILE_ID_INVALID;

    for (uint16_t tile = 0; tile < CJ4_TILE_ID_COUNT; ++tile)
    {
        if (state->locations[tile].wall == wall_position)
            return (cj4_tile_id)tile;
    }
    return CJ4_TILE_ID_INVALID;
}

uint8_t
cj4_location_collect_hand(
    const cj4_mahjong *state,
    cj4_player player,
    cj4_tile_id out_tiles[CJ4_MAX_HAND_TILES])
{
    uint8_t count = 0;

    if (out_tiles)
        memset(out_tiles, CJ4_TILE_ID_INVALID, sizeof(cj4_tile_id) * CJ4_MAX_HAND_TILES);
    if (!state || player >= CJ4_PLAYER_COUNT)
        return 0;

    for (uint16_t tile = 0; tile < CJ4_TILE_ID_COUNT; ++tile)
    {
        uint8_t placement = state->locations[tile].placement;

        if (!cj4_location_is_hand(placement) ||
            cj4_location_placement_player(placement) != player)
            continue;
        if (count >= CJ4_MAX_HAND_TILES)
            break;
        if (out_tiles)
            out_tiles[count] = (cj4_tile_id)tile;
        count++;
    }

    return count;
}

uint8_t
cj4_location_collect_discards(
    const cj4_mahjong *state,
    cj4_discard out_discards[CJ4_MAX_DISCARDS])
{
    cj4_discard collected[CJ4_MAX_DISCARDS];
    uint8_t present[CJ4_MAX_DISCARDS] = {0};
    uint8_t found = 0;
    uint8_t count;

    if (out_discards)
        memset(out_discards, 0, sizeof(cj4_discard) * CJ4_MAX_DISCARDS);
    if (!state)
        return 0;

    count = state->discard_count;
    if (count > CJ4_MAX_DISCARDS)
        count = CJ4_MAX_DISCARDS;

    memset(collected, 0, sizeof(collected));

    for (uint16_t tile = 0; tile < CJ4_TILE_ID_COUNT; ++tile)
    {
        const cj4_location *loc = &state->locations[tile];
        uint8_t index;
        cj4_discard *discard;

        if (!cj4_location_is_discard(loc->discard) ||
            !cj4_location_is_discard_history(loc->discard_history))
            continue;

        index = cj4_location_discard_history_index(loc->discard_history);
        if (index >= count || present[index])
            continue;

        present[index] = 1;
        discard = &collected[index];
        discard->tile = (cj4_tile_id)tile;
        discard->player = cj4_location_discard_player(loc->discard);
        discard->is_tsumogiri = (uint8_t)cj4_location_discard_is_tsumogiri(loc->discard);
        discard->is_riichi = (uint8_t)cj4_location_discard_is_riichi(loc->discard_history);
        discard->is_active = (uint8_t)!cj4_location_is_meld(loc->placement);
    }

    for (uint8_t index = 0; index < count; ++index)
    {
        if (!present[index])
            continue;
        if (out_discards)
            out_discards[found] = collected[index];
        found++;
    }

    return found;
}

bool
cj4_get_meld(
    const cj4_mahjong *state,
    cj4_player player,
    uint8_t group,
    cj4_meld *out_meld)
{
    cj4_meld meld;
    bool found = false;

    if (!state || player >= CJ4_PLAYER_COUNT || group > CJ4_MELD_GROUP_MAX)
        return false;

    memset(&meld, 0, sizeof(meld));
    meld.from_player = player;
    meld.called_index = CJ4_CALLED_INDEX_NONE;

    for (uint16_t tile = 0; tile < CJ4_TILE_ID_COUNT; ++tile)
    {
        const cj4_location *loc = &state->locations[tile];
        if (!cj4_location_is_meld(loc->placement) ||
            cj4_location_placement_player(loc->placement) != player ||
            cj4_location_meld_group(loc->placement) != group)
            continue;

        if (!found)
        {
            meld.type = cj4_location_meld_type(loc->placement);
            found = true;
        }
        if (meld.size < 4)
        {
            if (cj4_location_is_discard(loc->discard))
            {
                meld.called_index = meld.size;
                meld.from_player = cj4_location_discard_player(loc->discard);
            }
            meld.tiles[meld.size++] = (cj4_tile_id)tile;
        }
    }

    if (found && out_meld)
        *out_meld = meld;
    return found;
}

uint8_t
cj4_location_collect_melds(
    const cj4_mahjong *state,
    cj4_player player,
    cj4_meld out_melds[CJ4_MAX_MELDS])
{
    cj4_meld groups[CJ4_MAX_MELDS];
    uint8_t found[CJ4_MAX_MELDS] = {0};
    uint8_t count = 0;

    if (out_melds)
        memset(out_melds, 0, sizeof(cj4_meld) * CJ4_MAX_MELDS);
    if (!state || player >= CJ4_PLAYER_COUNT)
        return 0;

    memset(groups, 0, sizeof(groups));
    for (uint8_t group = 0; group < CJ4_MAX_MELDS; ++group)
    {
        groups[group].from_player = player;
        groups[group].called_index = CJ4_CALLED_INDEX_NONE;
    }

    for (uint16_t tile = 0; tile < CJ4_TILE_ID_COUNT; ++tile)
    {
        const cj4_location *loc = &state->locations[tile];
        cj4_meld *meld;
        uint8_t group;

        if (!cj4_location_is_meld(loc->placement) ||
            cj4_location_placement_player(loc->placement) != player)
            continue;

        group = cj4_location_meld_group(loc->placement);
        meld = &groups[group];
        if (!found[group])
        {
            meld->type = cj4_location_meld_type(loc->placement);
            found[group] = 1;
        }
        if (meld->size >= 4)
            continue;
        if (cj4_location_is_discard(loc->discard))
        {
            meld->called_index = meld->size;
            meld->from_player = cj4_location_discard_player(loc->discard);
        }
        meld->tiles[meld->size++] = (cj4_tile_id)tile;
    }

    for (uint8_t group = 0; group < CJ4_MAX_MELDS; ++group)
    {
        if (!found[group])
            continue;
        if (out_melds)
            out_melds[count] = groups[group];
        count++;
    }
    return count;
}

uint8_t
cj4_count_melds(
    const cj4_mahjong *state,
    cj4_player player)
{
    return cj4_location_collect_melds(state, player, NULL);
}

cj4_player
cj4_get_winner(
    const cj4_mahjong *state,
    uint8_t index)
{
    cj4_player loser = cj4_state_current_player(state);
    for (uint8_t distance = 0; distance < CJ4_PLAYER_COUNT; ++distance)
    {
        cj4_player player = (cj4_player)((loser + distance) % CJ4_PLAYER_COUNT);
        if (!cj4_state_is_winner(state, player))
            continue;
        if (index == 0)
            return player;
        index--;
    }
    return CJ4_PLAYER_COUNT;
}

bool
cj4_is_nagashi_mangan(
    const cj4_mahjong *state,
    cj4_player player)
{
    bool discarded = false;

    if (!state || player >= CJ4_PLAYER_COUNT)
        return false;

    for (uint16_t tile = 0; tile < CJ4_TILE_ID_COUNT; ++tile)
    {
        const cj4_location *loc = &state->locations[tile];
        if (!cj4_location_is_discard(loc->discard) ||
            cj4_location_discard_player(loc->discard) != player)
            continue;
        discarded = true;
        if (!cj4_tile_is_yaochu((cj4_tile_id)tile) ||
            cj4_location_is_meld(loc->placement))
            return false;
    }
    return discarded;
}

cj4_mahjong
cj4_make_player_state(
    const cj4_mahjong *state,
    cj4_player player)
{
    cj4_mahjong masked = *state;
    for (uint16_t tile = 0; tile < CJ4_TILE_ID_COUNT; ++tile)
    {
        cj4_location *loc = &masked.locations[tile];
        bool own_hand = cj4_location_is_hand(loc->placement) &&
                        cj4_location_placement_player(loc->placement) == player;
        bool visible = cj4_location_is_discard(loc->discard) ||
                       cj4_location_is_meld(loc->placement);
        for (uint8_t i = 0; !visible && i < state->dora_count && i < CJ4_MAX_DORA; ++i)
            visible = loc->wall == CJ4_DORA_INDICES[i];
        if (!own_hand && !visible)
            memset(loc, CJ4_LOCATION_NONE, sizeof(*loc));
    }
    if (cj4_tile_id_is_valid(masked.draw_tile))
    {
        const cj4_location *loc = &masked.locations[masked.draw_tile];
        if (!cj4_location_is_hand(loc->placement) ||
            cj4_location_placement_player(loc->placement) != player)
            masked.draw_tile = CJ4_TILE_ID_INVALID;
    }
    else
        masked.draw_tile = CJ4_TILE_ID_INVALID;
    if (cj4_state_current_player(&masked) != player)
    {
        masked.pending_ankan_tile = CJ4_TILE_ID_INVALID;
        for (uint8_t i = 0; i < 4; ++i)
            masked.pending_ankan_tiles[i] = CJ4_TILE_ID_INVALID;
    }
    return masked;
}
