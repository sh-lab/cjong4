#include "cjong4/core/hand_analysis.h"

#include "hand_analysis_internal.h"
#include "state_internal.h"
#include "state_query.h"

#include <string.h>

static uint8_t
cj4_hand_analysis_concealed_count(
    const cj4_hand_analysis_input *input)
{
    uint8_t total = 0;

    for (uint8_t type = 0; type < CJ4_TILE_TYPE_COUNT; ++type)
        total = (uint8_t)(total + input->concealed[type]);

    return total;
}

static uint8_t
cj4_hand_analysis_expected_after_discard(
    const cj4_hand_analysis_input *input)
{
    return (uint8_t)((CJ4_MAX_MELDS - input->meld_count) * 3 + 1);
}

static bool
cj4_hand_analysis_input_is_valid(
    const cj4_hand_analysis_input *input)
{
    if (!input || input->meld_count > CJ4_MAX_MELDS)
        return false;

    for (uint8_t type = 0; type < CJ4_TILE_TYPE_COUNT; ++type)
    {
        if (input->concealed[type] > CJ4_TILE_PER_TYPE ||
            input->visible[type] > CJ4_TILE_PER_TYPE ||
            input->visible[type] < input->concealed[type])
        {
            return false;
        }
    }

    return true;
}

static void
cj4_standard_shanten_search(
    int counts[CJ4_TILE_TYPE_COUNT],
    uint8_t index,
    uint8_t melds,
    uint8_t taatsu,
    uint8_t pair,
    int *best)
{
    while (index < CJ4_TILE_TYPE_COUNT && counts[index] == 0)
        index++;

    if (index == CJ4_TILE_TYPE_COUNT)
    {
        int usable_taatsu = taatsu;
        int slots = CJ4_MAX_MELDS - melds;
        int shanten;

        if (usable_taatsu > slots)
            usable_taatsu = slots;

        shanten = 8 - melds * 2 - usable_taatsu - pair;
        if (shanten < *best)
            *best = shanten;
        return;
    }

    if (*best == -1)
        return;

    if (melds < CJ4_MAX_MELDS && counts[index] >= 3)
    {
        counts[index] -= 3;
        cj4_standard_shanten_search(
            counts,
            index,
            (uint8_t)(melds + 1),
            taatsu,
            pair,
            best);
        counts[index] += 3;
    }

    if (melds < CJ4_MAX_MELDS && index < 27 && index % 9 <= 6 &&
        counts[index + 1] > 0 && counts[index + 2] > 0)
    {
        counts[index]--;
        counts[index + 1]--;
        counts[index + 2]--;
        cj4_standard_shanten_search(
            counts,
            index,
            (uint8_t)(melds + 1),
            taatsu,
            pair,
            best);
        counts[index]++;
        counts[index + 1]++;
        counts[index + 2]++;
    }

    if (!pair && counts[index] >= 2)
    {
        counts[index] -= 2;
        cj4_standard_shanten_search(
            counts,
            index,
            melds,
            taatsu,
            1,
            best);
        counts[index] += 2;
    }

    if (taatsu < CJ4_MAX_MELDS && counts[index] >= 2)
    {
        counts[index] -= 2;
        cj4_standard_shanten_search(
            counts,
            index,
            melds,
            (uint8_t)(taatsu + 1),
            pair,
            best);
        counts[index] += 2;
    }

    if (taatsu < CJ4_MAX_MELDS && index < 27 && index % 9 <= 7 &&
        counts[index + 1] > 0)
    {
        counts[index]--;
        counts[index + 1]--;
        cj4_standard_shanten_search(
            counts,
            index,
            melds,
            (uint8_t)(taatsu + 1),
            pair,
            best);
        counts[index]++;
        counts[index + 1]++;
    }

    if (taatsu < CJ4_MAX_MELDS && index < 27 && index % 9 <= 6 &&
        counts[index + 2] > 0)
    {
        counts[index]--;
        counts[index + 2]--;
        cj4_standard_shanten_search(
            counts,
            index,
            melds,
            (uint8_t)(taatsu + 1),
            pair,
            best);
        counts[index]++;
        counts[index + 2]++;
    }

    counts[index]--;
    cj4_standard_shanten_search(
        counts,
        index,
        melds,
        taatsu,
        pair,
        best);
    counts[index]++;
}

static int8_t
cj4_standard_shanten(
    const uint8_t concealed[CJ4_TILE_TYPE_COUNT],
    uint8_t meld_count)
{
    int counts[CJ4_TILE_TYPE_COUNT];
    int best = 8;

    for (uint8_t type = 0; type < CJ4_TILE_TYPE_COUNT; ++type)
        counts[type] = concealed[type];

    cj4_standard_shanten_search(counts, 0, meld_count, 0, 0, &best);
    return (int8_t)best;
}

static int8_t
cj4_chiitoitsu_shanten(
    const uint8_t concealed[CJ4_TILE_TYPE_COUNT],
    uint8_t meld_count)
{
    uint8_t pairs = 0;
    uint8_t distinct = 0;

    if (meld_count != 0)
        return CJ4_SHANTEN_NOT_APPLICABLE;

    for (uint8_t type = 0; type < CJ4_TILE_TYPE_COUNT; ++type)
    {
        if (concealed[type] > 0)
            distinct++;
        if (concealed[type] >= 2)
            pairs++;
    }

    return (int8_t)(6 - pairs + (distinct < 7 ? 7 - distinct : 0));
}

static uint8_t
cj4_is_kokushi_type(
    uint8_t type)
{
    return type == 0 || type == 8 ||
           type == 9 || type == 17 ||
           type == 18 || type == 26 ||
           type >= 27;
}

static int8_t
cj4_kokushi_shanten(
    const uint8_t concealed[CJ4_TILE_TYPE_COUNT],
    uint8_t meld_count)
{
    uint8_t unique = 0;
    uint8_t pair = 0;

    if (meld_count != 0)
        return CJ4_SHANTEN_NOT_APPLICABLE;

    for (uint8_t type = 0; type < CJ4_TILE_TYPE_COUNT; ++type)
    {
        if (!cj4_is_kokushi_type(type))
            continue;
        if (concealed[type] > 0)
            unique++;
        if (concealed[type] >= 2)
            pair = 1;
    }

    return (int8_t)(13 - unique - pair);
}

static cj4_shanten_result
cj4_hand_analysis_calculate_raw(
    const uint8_t concealed[CJ4_TILE_TYPE_COUNT],
    uint8_t meld_count)
{
    cj4_shanten_result result;

    result.standard = cj4_standard_shanten(concealed, meld_count);
    result.chiitoitsu = cj4_chiitoitsu_shanten(concealed, meld_count);
    result.kokushi = cj4_kokushi_shanten(concealed, meld_count);
    return result;
}

static void
cj4_shanten_result_set_not_applicable(
    cj4_shanten_result *result)
{
    result->standard = CJ4_SHANTEN_NOT_APPLICABLE;
    result->chiitoitsu = CJ4_SHANTEN_NOT_APPLICABLE;
    result->kokushi = CJ4_SHANTEN_NOT_APPLICABLE;
}

static void
cj4_shanten_result_take_minimum(
    cj4_shanten_result *result,
    const cj4_shanten_result *candidate)
{
    if (candidate->standard < result->standard)
        result->standard = candidate->standard;
    if (candidate->chiitoitsu < result->chiitoitsu)
        result->chiitoitsu = candidate->chiitoitsu;
    if (candidate->kokushi < result->kokushi)
        result->kokushi = candidate->kokushi;
}

static int8_t
cj4_shanten_result_minimum(
    const cj4_shanten_result *result)
{
    int8_t minimum = result->standard;

    if (result->chiitoitsu < minimum)
        minimum = result->chiitoitsu;
    if (result->kokushi < minimum)
        minimum = result->kokushi;
    return minimum;
}

bool
cj4_hand_analysis_calculate(
    const cj4_hand_analysis_input *input,
    cj4_shanten_result *out_result)
{
    uint8_t concealed[CJ4_TILE_TYPE_COUNT];
    uint8_t total;
    uint8_t expected;

    if (!out_result)
        return false;
    cj4_shanten_result_set_not_applicable(out_result);

    if (!cj4_hand_analysis_input_is_valid(input))
        return false;

    total = cj4_hand_analysis_concealed_count(input);
    expected = cj4_hand_analysis_expected_after_discard(input);

    if (total == expected)
    {
        *out_result = cj4_hand_analysis_calculate_raw(
            input->concealed,
            input->meld_count);
        return true;
    }

    if (total != (uint8_t)(expected + 1))
        return false;

    memcpy(concealed, input->concealed, sizeof(concealed));
    for (uint8_t type = 0; type < CJ4_TILE_TYPE_COUNT; ++type)
    {
        cj4_shanten_result candidate;

        if (concealed[type] == 0)
            continue;
        concealed[type]--;
        candidate = cj4_hand_analysis_calculate_raw(
            concealed,
            input->meld_count);
        cj4_shanten_result_take_minimum(out_result, &candidate);
        concealed[type]++;
    }

    return true;
}

bool
cj4_hand_analysis_calculate_after_discard(
    const cj4_hand_analysis_input *input,
    cj4_tile_type discard,
    cj4_shanten_result *out_result)
{
    cj4_hand_analysis_input after;
    uint8_t expected;

    if (!out_result)
        return false;
    cj4_shanten_result_set_not_applicable(out_result);

    if (!cj4_hand_analysis_input_is_valid(input) ||
        !cj4_tile_type_is_valid(discard))
    {
        return false;
    }

    expected = cj4_hand_analysis_expected_after_discard(input);
    if (cj4_hand_analysis_concealed_count(input) != (uint8_t)(expected + 1) ||
        input->concealed[discard] == 0)
    {
        return false;
    }

    after = *input;
    after.concealed[discard]--;
    *out_result = cj4_hand_analysis_calculate_raw(
        after.concealed,
        after.meld_count);
    return true;
}

static bool
cj4_hand_analysis_collect_waits_normalized(
    const cj4_hand_analysis_input *input,
    cj4_waiting_tile_types *out_waits)
{
    uint8_t concealed[CJ4_TILE_TYPE_COUNT];

    memset(out_waits, 0, sizeof(*out_waits));
    memcpy(concealed, input->concealed, sizeof(concealed));

    for (uint8_t type = 0; type < CJ4_TILE_TYPE_COUNT; ++type)
    {
        cj4_shanten_result completed;

        if (concealed[type] >= CJ4_TILE_PER_TYPE)
            continue;

        concealed[type]++;
        completed = cj4_hand_analysis_calculate_raw(
            concealed,
            input->meld_count);
        concealed[type]--;

        if (cj4_shanten_result_minimum(&completed) != -1)
            continue;

        out_waits->types[type] = 1;
        out_waits->count[type] =
            input->visible[type] >= CJ4_TILE_PER_TYPE
                ? 0
                : (uint8_t)(CJ4_TILE_PER_TYPE - input->visible[type]);
    }

    return true;
}

bool
cj4_hand_analysis_collect_waits(
    const cj4_hand_analysis_input *input,
    cj4_waiting_tile_types *out_waits)
{
    if (!out_waits)
        return false;
    memset(out_waits, 0, sizeof(*out_waits));

    if (!cj4_hand_analysis_input_is_valid(input) ||
        cj4_hand_analysis_concealed_count(input) !=
            cj4_hand_analysis_expected_after_discard(input))
    {
        return false;
    }

    return cj4_hand_analysis_collect_waits_normalized(input, out_waits);
}

bool
cj4_hand_analysis_collect_waits_after_discard(
    const cj4_hand_analysis_input *input,
    cj4_tile_type discard,
    cj4_waiting_tile_types *out_waits)
{
    cj4_hand_analysis_input after;
    uint8_t expected;

    if (!out_waits)
        return false;
    memset(out_waits, 0, sizeof(*out_waits));

    if (!cj4_hand_analysis_input_is_valid(input) ||
        !cj4_tile_type_is_valid(discard))
    {
        return false;
    }

    expected = cj4_hand_analysis_expected_after_discard(input);
    if (cj4_hand_analysis_concealed_count(input) != (uint8_t)(expected + 1) ||
        input->concealed[discard] == 0)
    {
        return false;
    }

    after = *input;
    after.concealed[discard]--;
    return cj4_hand_analysis_collect_waits_normalized(&after, out_waits);
}

static uint8_t
cj4_state_tile_is_visible_to_player(
    const cj4_mahjong *state,
    cj4_player player,
    cj4_tile_id tile)
{
    const cj4_location *location = &state->locations[tile];

    if ((cj4_location_is_hand(location->placement) &&
         cj4_location_placement_player(location->placement) == player) ||
        cj4_location_is_meld(location->placement) ||
        cj4_location_is_discard(location->discard))
    {
        return 1;
    }

    for (uint8_t i = 0; i < state->dora_count && i < CJ4_MAX_DORA; ++i)
        if (location->wall == CJ4_DORA_INDICES[i])
            return 1;

    return 0;
}

static bool
cj4_hand_analysis_input_from_state(
    const cj4_mahjong *state,
    cj4_player player,
    cj4_hand_analysis_input *out_input)
{
    cj4_meld_list melds;

    if (!state || player >= CJ4_PLAYER_COUNT || !out_input)
        return false;

    memset(out_input, 0, sizeof(*out_input));

    for (uint16_t tile = 0; tile < CJ4_TILE_ID_COUNT; ++tile)
    {
        cj4_tile_type type = cj4_tile_get_type((cj4_tile_id)tile);
        const cj4_location *location = &state->locations[tile];

        if (cj4_location_is_hand(location->placement) &&
            cj4_location_placement_player(location->placement) == player)
        {
            out_input->concealed[type]++;
        }

        if (cj4_state_tile_is_visible_to_player(
                state,
                player,
                (cj4_tile_id)tile))
        {
            out_input->visible[type]++;
        }
    }

    melds = cj4_location_collect_melds(state->locations, player);
    out_input->meld_count = melds.count;
    return cj4_hand_analysis_input_is_valid(out_input);
}

static bool
cj4_state_player_owns_tile(
    const cj4_mahjong *state,
    cj4_player player,
    cj4_tile_id tile)
{
    return state && cj4_tile_id_is_valid(tile) &&
           cj4_location_is_hand(state->locations[tile].placement) &&
           cj4_location_placement_player(state->locations[tile].placement) == player;
}

bool
cj4_calculate_shanten(
    const cj4_mahjong *state,
    cj4_player player,
    cj4_shanten_result *out_result)
{
    cj4_hand_analysis_input input;

    if (!cj4_hand_analysis_input_from_state(state, player, &input))
        return false;
    return cj4_hand_analysis_calculate(&input, out_result);
}

bool
cj4_calculate_shanten_after_discard(
    const cj4_mahjong *state,
    cj4_player player,
    cj4_tile_id discard,
    cj4_shanten_result *out_result)
{
    cj4_hand_analysis_input input;

    if (!cj4_state_player_owns_tile(state, player, discard) ||
        !cj4_hand_analysis_input_from_state(state, player, &input))
    {
        return false;
    }

    return cj4_hand_analysis_calculate_after_discard(
        &input,
        cj4_tile_get_type(discard),
        out_result);
}

bool
cj4_is_shape_tenpai(
    const cj4_mahjong *state,
    cj4_player player)
{
    cj4_shanten_result result;

    return cj4_calculate_shanten(state, player, &result) &&
           cj4_shanten_result_minimum(&result) == 0;
}

bool
cj4_collect_waiting_tile_types(
    const cj4_mahjong *state,
    cj4_player player,
    cj4_waiting_tile_types *out_waits)
{
    cj4_hand_analysis_input input;

    if (!cj4_hand_analysis_input_from_state(state, player, &input))
        return false;
    return cj4_hand_analysis_collect_waits(&input, out_waits);
}

bool
cj4_collect_waiting_tile_types_after_discard(
    const cj4_mahjong *state,
    cj4_player player,
    cj4_tile_id discard,
    cj4_waiting_tile_types *out_waits)
{
    cj4_hand_analysis_input input;

    if (!cj4_state_player_owns_tile(state, player, discard) ||
        !cj4_hand_analysis_input_from_state(state, player, &input))
    {
        return false;
    }

    return cj4_hand_analysis_collect_waits_after_discard(
        &input,
        cj4_tile_get_type(discard),
        out_waits);
}
