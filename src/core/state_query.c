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

cj4_hand
cj4_location_collect_hand(
    const cj4_location locations[CJ4_TILE_ID_COUNT],
    cj4_player player)
{
    cj4_hand hand;

    memset(hand.items, CJ4_TILE_ID_INVALID, sizeof(hand.items));
    hand.count = 0;
    if (!locations || player >= CJ4_PLAYER_COUNT)
        return hand;

    for (uint16_t tile = 0; tile < CJ4_TILE_ID_COUNT; ++tile)
    {
        uint8_t placement = locations[tile].placement;

        if (!cj4_location_is_hand(placement) ||
            cj4_location_placement_player(placement) != player)
            continue;
        if (hand.count >= CJ4_MAX_HAND_TILES)
            break;
        hand.items[hand.count++] = (cj4_tile_id)tile;
    }

    return hand;
}

static cj4_discard
cj4_location_empty_discard(
    void)
{
    cj4_discard discard;

    memset(&discard, 0, sizeof(discard));
    discard.tile = CJ4_TILE_ID_INVALID;
    discard.player = CJ4_PLAYER_COUNT;
    return discard;
}

static cj4_discard
cj4_location_make_collected_discard(
    cj4_tile_id tile,
    const cj4_location *location)
{
    cj4_discard discard = cj4_location_empty_discard();

    discard.tile = tile;
    discard.player = cj4_location_discard_player(location->discard);
    discard.is_tsumogiri =
        (uint8_t)cj4_location_discard_is_tsumogiri(location->discard);
    discard.is_riichi =
        (uint8_t)cj4_location_discard_is_riichi(location->discard_history);
    discard.is_active = (uint8_t)!cj4_location_is_meld(location->placement);
    return discard;
}

static cj4_discard_list
cj4_location_empty_discard_list(
    void)
{
    cj4_discard_list list;

    list.count = 0;
    for (uint8_t i = 0; i < CJ4_MAX_DISCARDS; ++i)
        list.items[i] = cj4_location_empty_discard();
    return list;
}

cj4_discard_list
cj4_location_collect_discards(
    const cj4_location locations[CJ4_TILE_ID_COUNT])
{
    cj4_discard collected[CJ4_MAX_DISCARDS];
    uint8_t present[CJ4_MAX_DISCARDS] = {0};
    cj4_discard_list list = cj4_location_empty_discard_list();

    if (!locations)
        return list;

    for (uint16_t tile = 0; tile < CJ4_TILE_ID_COUNT; ++tile)
    {
        const cj4_location *loc = &locations[tile];
        uint8_t index;

        if (!cj4_location_is_discard(loc->discard) ||
            !cj4_location_is_discard_history(loc->discard_history))
            continue;

        index = cj4_location_discard_history_index(loc->discard_history);
        if (index >= CJ4_MAX_DISCARDS || present[index])
            continue;

        present[index] = 1;
        collected[index] =
            cj4_location_make_collected_discard((cj4_tile_id)tile, loc);
    }

    for (uint8_t index = 0; index < CJ4_MAX_DISCARDS; ++index)
    {
        if (!present[index])
            continue;
        list.items[list.count++] = collected[index];
    }

    return list;
}

cj4_discard_list
cj4_location_collect_player_discards(
    const cj4_location locations[CJ4_TILE_ID_COUNT],
    cj4_player player)
{
    cj4_discard collected[CJ4_DISCARD_INDEX_MAX + 1];
    uint8_t present[CJ4_DISCARD_INDEX_MAX + 1] = {0};
    cj4_discard_list list = cj4_location_empty_discard_list();

    if (!locations || player >= CJ4_PLAYER_COUNT)
        return list;

    for (uint16_t tile = 0; tile < CJ4_TILE_ID_COUNT; ++tile)
    {
        const cj4_location *loc = &locations[tile];
        uint8_t index;

        if (!cj4_location_is_discard(loc->discard) ||
            cj4_location_discard_player(loc->discard) != player)
            continue;

        index = cj4_location_discard_index(loc->discard);
        if (index > CJ4_DISCARD_INDEX_MAX || present[index])
            continue;

        present[index] = 1;
        collected[index] =
            cj4_location_make_collected_discard((cj4_tile_id)tile, loc);
    }

    for (uint8_t index = 0; index <= CJ4_DISCARD_INDEX_MAX; ++index)
    {
        if (present[index])
            list.items[list.count++] = collected[index];
    }

    return list;
}

static cj4_meld_list
cj4_location_empty_meld_list(
    void)
{
    cj4_meld_list list;

    list.count = 0;
    for (uint8_t i = 0; i < CJ4_MAX_MELDS; ++i)
    {
        memset(list.items[i].tiles, CJ4_TILE_ID_INVALID, sizeof(list.items[i].tiles));
        list.items[i].size = 0;
        list.items[i].type = CJ4_MELD_INVALID;
        list.items[i].from_player = CJ4_PLAYER_COUNT;
        list.items[i].called_index = CJ4_CALLED_INDEX_NONE;
    }
    return list;
}

cj4_meld_list
cj4_location_collect_melds(
    const cj4_location locations[CJ4_TILE_ID_COUNT],
    cj4_player player)
{
    cj4_meld groups[CJ4_MAX_MELDS];
    uint8_t found[CJ4_MAX_MELDS] = {0};
    cj4_meld_list list = cj4_location_empty_meld_list();

    if (!locations || player >= CJ4_PLAYER_COUNT)
        return list;

    for (uint8_t group = 0; group < CJ4_MAX_MELDS; ++group)
    {
        memset(groups[group].tiles, CJ4_TILE_ID_INVALID, sizeof(groups[group].tiles));
        groups[group].size = 0;
        groups[group].type = CJ4_MELD_INVALID;
        groups[group].from_player = player;
        groups[group].called_index = CJ4_CALLED_INDEX_NONE;
    }

    for (uint16_t tile = 0; tile < CJ4_TILE_ID_COUNT; ++tile)
    {
        const cj4_location *loc = &locations[tile];
        cj4_meld *meld;
        uint8_t group;

        if (!cj4_location_is_meld(loc->placement) ||
            cj4_location_placement_player(loc->placement) != player)
            continue;

        group = cj4_location_meld_group(loc->placement);
        if (group >= CJ4_MAX_MELDS)
            continue;
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
        list.items[list.count++] = groups[group];
    }
    return list;
}

cj4_dora_indicator_list
cj4_location_collect_dora_indicators(
    const cj4_location locations[CJ4_TILE_ID_COUNT])
{
    cj4_dora_indicator_list list;

    memset(list.items, CJ4_TILE_ID_INVALID, sizeof(list.items));
    list.count = 0;
    if (!locations)
        return list;

    for (uint8_t i = 0; i < CJ4_MAX_DORA_INDICATORS; ++i)
    {
        for (uint16_t tile = 0; tile < CJ4_TILE_ID_COUNT; ++tile)
        {
            if (locations[tile].wall != CJ4_DORA_INDICES[i])
                continue;

            list.items[list.count++] = (cj4_tile_id)tile;
            break;
        }
    }

    return list;
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
