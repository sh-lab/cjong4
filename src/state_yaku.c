#include "state_yaku.h"
#include "hand_check.h"
#include "state_query.h"
#include "tile.h"
#include "tile_const.h"

#include <stdbool.h>
#include <string.h>

typedef struct
{
    int concealed_counts[CJ4_TILE_TYPE_COUNT];
    int total_counts[CJ4_TILE_TYPE_COUNT];
    uint8_t has_open_meld;
    uint8_t has_sequence_meld;
    uint8_t has_terminal;
    uint8_t has_honor;
    uint8_t has_simple;
    uint8_t suit_mask;
} cj4_yaku_context;

static uint8_t
cj4_yaku_is_closed_hand(const cj4_mahjong *state, cj4_player player)
{
    for (uint8_t i = 0; i < state->meld_count[player]; ++i)
    {
        if (state->melds[player][i].type != CJ4_MELD_ANKAN)
            return 0;
    }

    return 1;
}

static cj4_wind
cj4_yaku_seat_wind(const cj4_mahjong *state, cj4_player player)
{
    return (cj4_wind)((player + CJ4_WIND_COUNT - state->dealer) % CJ4_WIND_COUNT);
}

static cj4_tile_type
cj4_yaku_wind_tile_type(cj4_wind wind)
{
    return (cj4_tile_type)(27 + wind);
}

static void
cj4_yaku_note_type(cj4_yaku_context *ctx, cj4_tile_type type)
{
    cj4_tile_suit suit = cj4_tile_type_get_suit(type);

    if (suit == CJ4_TILE_SUIT_HONOR)
    {
        ctx->has_honor = 1;
    }
    else
    {
        ctx->suit_mask |= (uint8_t)(1u << suit);

        if (cj4_tile_type_is_yaochu(type))
            ctx->has_terminal = 1;
        else
            ctx->has_simple = 1;
    }
}

static void
cj4_yaku_collect_context(
    const cj4_mahjong *state,
    cj4_player player,
    cj4_yaku_context *ctx)
{
    memset(ctx, 0, sizeof(*ctx));

    for (int tid = 0; tid < CJ4_TILE_ID_COUNT; ++tid)
    {
        const cj4_location *loc = cj4_tile_location_const(state, (cj4_tile_id)tid);

        if (loc->zone == CJ4_ZONE_HAND && loc->owner == player)
        {
            cj4_tile_type type = cj4_tile_get_type((cj4_tile_id)tid);
            ctx->concealed_counts[type]++;
            ctx->total_counts[type]++;
            cj4_yaku_note_type(ctx, type);
        }
    }

    for (uint8_t i = 0; i < state->meld_count[player]; ++i)
    {
        const cj4_meld *meld = &state->melds[player][i];

        if (meld->type == CJ4_MELD_CHI)
            ctx->has_sequence_meld = 1;

        if (meld->type != CJ4_MELD_ANKAN)
            ctx->has_open_meld = 1;

        for (uint8_t j = 0; j < meld->size; ++j)
        {
            cj4_tile_type type = cj4_tile_get_type(meld->tiles[j]);
            ctx->total_counts[type]++;
            cj4_yaku_note_type(ctx, type);
        }
    }
}

static int
cj4_yaku_tile_count(const int counts[CJ4_TILE_TYPE_COUNT])
{
    int total = 0;

    for (int i = 0; i < CJ4_TILE_TYPE_COUNT; ++i)
    {
        total += counts[i];
    }

    return total;
}

static uint8_t
cj4_yaku_is_chiitoi(const int counts[CJ4_TILE_TYPE_COUNT])
{
    int pair_count = 0;

    if (cj4_yaku_tile_count(counts) != 14)
        return 0;

    for (int i = 0; i < CJ4_TILE_TYPE_COUNT; ++i)
    {
        pair_count += counts[i] / 2;
    }

    return pair_count == 7;
}

static uint8_t
cj4_yaku_is_kokushi(const int counts[CJ4_TILE_TYPE_COUNT])
{
    static const int terminals[13] = {
        0, 8,
        9, 17,
        18, 26,
        27, 28, 29, 30, 31, 32, 33
    };
    int found = 0;
    int duplicates = 0;

    if (cj4_yaku_tile_count(counts) != 14)
        return 0;

    for (int i = 0; i < 13; ++i)
    {
        int type = terminals[i];

        if (counts[type] >= 1)
            found++;
        if (counts[type] >= 2)
            duplicates++;
    }

    return found == 13 && duplicates == 1;
}

static uint8_t
cj4_yaku_is_tanyao(
    const cj4_yaku_context *ctx,
    const cj4_rules *rules)
{
    if (ctx->has_honor || ctx->has_terminal || !ctx->has_simple)
        return 0;

    if (ctx->has_open_meld && !(rules && rules->kuitan))
        return 0;

    return 1;
}

static uint8_t
cj4_yaku_has_yakuhai(
    const cj4_mahjong *state,
    cj4_player player,
    const cj4_yaku_context *ctx)
{
    cj4_tile_type seat_type =
        cj4_yaku_wind_tile_type(cj4_yaku_seat_wind(state, player));
    cj4_tile_type round_type =
        cj4_yaku_wind_tile_type(state->round_wind);

    if (ctx->total_counts[CJ4_TILE_TYPE_HAKU] >= 3 ||
        ctx->total_counts[CJ4_TILE_TYPE_HATSU] >= 3 ||
        ctx->total_counts[CJ4_TILE_TYPE_CHUN] >= 3)
        return 1;

    if (ctx->total_counts[seat_type] >= 3)
        return 1;

    if (ctx->total_counts[round_type] >= 3)
        return 1;

    return 0;
}

static uint8_t
cj4_yaku_is_honroutou(const cj4_yaku_context *ctx)
{
    return ctx->has_terminal || ctx->has_honor
        ? (uint8_t)!ctx->has_simple
        : 0;
}

static uint8_t
cj4_yaku_is_honitsu(const cj4_yaku_context *ctx)
{
    return ctx->has_honor && ctx->suit_mask != 0 &&
           (ctx->suit_mask & (uint8_t)(ctx->suit_mask - 1)) == 0;
}

static uint8_t
cj4_yaku_is_chinitsu(const cj4_yaku_context *ctx)
{
    return !ctx->has_honor &&
           (ctx->suit_mask & (uint8_t)(ctx->suit_mask - 1)) == 0 &&
           ctx->suit_mask != 0;
}

static uint8_t
cj4_yaku_is_toitoi(
    const cj4_mahjong *state,
    cj4_player player,
    const cj4_yaku_context *ctx)
{
    int counts[CJ4_TILE_TYPE_COUNT];
    int concealed_tiles = cj4_yaku_tile_count(ctx->concealed_counts);
    int melds_needed = 4 - state->meld_count[player];

    if (ctx->has_sequence_meld)
        return 0;

    if (concealed_tiles != melds_needed * 3 + 2)
        return 0;

    memcpy(counts, ctx->concealed_counts, sizeof(counts));

    for (int pair = 0; pair < CJ4_TILE_TYPE_COUNT; ++pair)
    {
        uint8_t ok = 1;

        if (counts[pair] < 2)
            continue;

        counts[pair] -= 2;

        for (int i = 0; i < CJ4_TILE_TYPE_COUNT; ++i)
        {
            if (counts[i] % 3 != 0)
            {
                ok = 0;
                break;
            }
        }

        counts[pair] += 2;

        if (ok)
            return 1;
    }

    return 0;
}

bool
cj4_has_yaku(
    const cj4_mahjong *state,
    cj4_player player,
    const cj4_rules *rules)
{
    cj4_yaku_context ctx;

    if (!cj4_is_complete_hand(state, player))
        return false;

    cj4_yaku_collect_context(state, player, &ctx);

    if (state->is_riichi[player])
        return true;

    if (rules && rules->ippatsu && state->is_ippatsu[player])
        return true;

    if (state->phase == CJ4_PHASE_DRAW &&
        state->current_player == player &&
        state->draw_tile != CJ4_TILE_ID_INVALID &&
        cj4_yaku_is_closed_hand(state, player))
        return true;

    if (state->meld_count[player] == 0)
    {
        if (cj4_yaku_is_kokushi(ctx.concealed_counts))
            return true;

        if (cj4_yaku_is_chiitoi(ctx.concealed_counts))
            return true;
    }

    if (cj4_yaku_is_tanyao(&ctx, rules))
        return true;

    if (cj4_yaku_has_yakuhai(state, player, &ctx))
        return true;

    if (cj4_yaku_is_honroutou(&ctx))
        return true;

    if (cj4_yaku_is_toitoi(state, player, &ctx))
        return true;

    if (cj4_yaku_is_honitsu(&ctx))
        return true;

    if (cj4_yaku_is_chinitsu(&ctx))
        return true;

    return false;
}
