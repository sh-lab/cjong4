#include "state_yaku.h"
#include "hand_check.h"
#include "state_score.h"
#include "state_internal.h"
#include "state_query.h"
#include "tile.h"
#include "tile_const.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define CJ4_LIVE_WALL_END 122

typedef uint64_t cj4_yaku_flags;

#define CJ4_YAKU_RIICHI             ((cj4_yaku_flags)1ULL << 0)
#define CJ4_YAKU_IPPATSU            ((cj4_yaku_flags)1ULL << 1)
#define CJ4_YAKU_MENZEN_TSUMO       ((cj4_yaku_flags)1ULL << 2)
#define CJ4_YAKU_TANYAO             ((cj4_yaku_flags)1ULL << 3)
#define CJ4_YAKU_YAKUHAI            ((cj4_yaku_flags)1ULL << 4)
#define CJ4_YAKU_CHIITOI            ((cj4_yaku_flags)1ULL << 5)
#define CJ4_YAKU_KOKUSHI            ((cj4_yaku_flags)1ULL << 6)
#define CJ4_YAKU_TOITOI             ((cj4_yaku_flags)1ULL << 7)
#define CJ4_YAKU_HONROUTOU          ((cj4_yaku_flags)1ULL << 8)
#define CJ4_YAKU_HONITSU            ((cj4_yaku_flags)1ULL << 9)
#define CJ4_YAKU_CHINITSU           ((cj4_yaku_flags)1ULL << 10)
#define CJ4_YAKU_PINFU              ((cj4_yaku_flags)1ULL << 11)
#define CJ4_YAKU_IIPEIKOU           ((cj4_yaku_flags)1ULL << 12)
#define CJ4_YAKU_RYANPEIKOU         ((cj4_yaku_flags)1ULL << 13)
#define CJ4_YAKU_SANSHOKU_DOUJUN    ((cj4_yaku_flags)1ULL << 14)
#define CJ4_YAKU_ITTSUU             ((cj4_yaku_flags)1ULL << 15)
#define CJ4_YAKU_CHANTA             ((cj4_yaku_flags)1ULL << 16)
#define CJ4_YAKU_JUNCHAN            ((cj4_yaku_flags)1ULL << 17)
#define CJ4_YAKU_SANANKOU           ((cj4_yaku_flags)1ULL << 18)
#define CJ4_YAKU_SHOUSANGEN         ((cj4_yaku_flags)1ULL << 19)
#define CJ4_YAKU_DAISANGEN          ((cj4_yaku_flags)1ULL << 20)
#define CJ4_YAKU_SHOUSUUSHII        ((cj4_yaku_flags)1ULL << 21)
#define CJ4_YAKU_DAISUUSHII         ((cj4_yaku_flags)1ULL << 22)
#define CJ4_YAKU_TSUUIISOU          ((cj4_yaku_flags)1ULL << 23)
#define CJ4_YAKU_RYUUIISOU          ((cj4_yaku_flags)1ULL << 24)
#define CJ4_YAKU_CHINROUTOU         ((cj4_yaku_flags)1ULL << 25)
#define CJ4_YAKU_SANKANTSU          ((cj4_yaku_flags)1ULL << 26)
#define CJ4_YAKU_SUUKANTSU          ((cj4_yaku_flags)1ULL << 27)
#define CJ4_YAKU_SANSHOKU_DOUKOU    ((cj4_yaku_flags)1ULL << 28)
#define CJ4_YAKU_SUUANKOU           ((cj4_yaku_flags)1ULL << 29)
#define CJ4_YAKU_CHUUREN            ((cj4_yaku_flags)1ULL << 30)
#define CJ4_YAKU_RINSHAN            ((cj4_yaku_flags)1ULL << 31)
#define CJ4_YAKU_HAITEI             ((cj4_yaku_flags)1ULL << 32)
#define CJ4_YAKU_HOUTEI             ((cj4_yaku_flags)1ULL << 33)
#define CJ4_YAKU_KOKUSHI_13         ((cj4_yaku_flags)1ULL << 34)
#define CJ4_YAKU_SUUANKOU_TANKI     ((cj4_yaku_flags)1ULL << 35)
#define CJ4_YAKU_JUNSEI_CHUUREN     ((cj4_yaku_flags)1ULL << 36)
#define CJ4_YAKU_DAISUUSHII_DOUBLE  ((cj4_yaku_flags)1ULL << 37)
#define CJ4_YAKU_DOUBLE_RIICHI      ((cj4_yaku_flags)1ULL << 38)
#define CJ4_YAKU_CHANKAN            ((cj4_yaku_flags)1ULL << 39)
#define CJ4_YAKU_TENHOU             ((cj4_yaku_flags)1ULL << 40)
#define CJ4_YAKU_CHIIHOU            ((cj4_yaku_flags)1ULL << 41)

typedef enum
{
    CJ4_WIN_NONE,
    CJ4_WIN_TSUMO,
    CJ4_WIN_RON
} cj4_win_type;

typedef enum
{
    CJ4_GROUP_SEQUENCE,
    CJ4_GROUP_TRIPLET,
    CJ4_GROUP_QUAD
} cj4_group_kind;

typedef struct
{
    cj4_group_kind kind;
    cj4_tile_type base_type;
    uint8_t is_open;
    uint8_t uses_winning_tile;
    uint8_t winning_position;
} cj4_yaku_group;

typedef struct
{
    cj4_yaku_group groups[4];
    uint8_t group_count;
    cj4_tile_type pair_type;
    uint8_t has_pair;
    uint8_t pair_uses_winning_tile;
} cj4_yaku_decomposition;

typedef struct
{
    int concealed_counts[CJ4_TILE_TYPE_COUNT];
    int total_counts[CJ4_TILE_TYPE_COUNT];
    uint8_t suit_mask;
    uint8_t has_terminal;
    uint8_t has_honor;
    uint8_t has_simple;
    uint8_t has_open_meld;
    uint8_t is_closed_hand;
    uint8_t quad_count;
    cj4_win_type win_type;
    cj4_tile_id winning_tile;
    cj4_tile_type winning_type_id;
    uint8_t has_winning_tile;
    uint8_t is_rinshan;
    uint8_t is_haitei;
    uint8_t is_houtei;
    uint8_t is_chankan;
    cj4_yaku_flags flags;
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
    return (cj4_tile_type)(CJ4_TILE_TYPE_EAST + wind);
}

static uint8_t
cj4_yaku_tile_count(const int counts[CJ4_TILE_TYPE_COUNT])
{
    int total = 0;

    for (int i = 0; i < CJ4_TILE_TYPE_COUNT; ++i)
        total += counts[i];

    return (uint8_t)total;
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

static uint8_t
cj4_yaku_find_tile_wall_index(const cj4_mahjong *state, cj4_tile_id tile)
{
    for (uint8_t i = 0; i < CJ4_TILE_ID_COUNT; ++i)
    {
        if (state->wall[i] == tile)
            return i;
    }

    return CJ4_TILE_ID_INVALID;
}

static void
cj4_yaku_detect_win_state(
    const cj4_mahjong *state,
    cj4_player player,
    cj4_yaku_context *ctx)
{
    ctx->win_type = CJ4_WIN_NONE;
    ctx->winning_tile = CJ4_TILE_ID_INVALID;
    ctx->winning_type_id = CJ4_TILE_TYPE_COUNT;
    ctx->has_winning_tile = 0;
    ctx->is_rinshan = 0;
    ctx->is_haitei = 0;
    ctx->is_houtei = 0;
    ctx->is_chankan = 0;

    if (state->phase == CJ4_PHASE_DRAW &&
        state->current_player == player &&
        state->draw_tile != CJ4_TILE_ID_INVALID &&
        cj4_tile_location_const(state, state->draw_tile)->zone == CJ4_ZONE_HAND &&
        cj4_tile_location_const(state, state->draw_tile)->owner == player)
    {
        uint8_t wall_index;

        ctx->win_type = CJ4_WIN_TSUMO;
        ctx->winning_tile = state->draw_tile;
        ctx->winning_type_id = cj4_tile_get_type(state->draw_tile);
        ctx->has_winning_tile = 1;

        wall_index = cj4_yaku_find_tile_wall_index(state, state->draw_tile);

        if (wall_index != CJ4_TILE_ID_INVALID)
        {
            for (uint8_t i = 0; i < state->dead_wall_draw_count && i < 4; ++i)
            {
                if (wall_index == CJ4_RINSHAN_INDICES[i])
                {
                    ctx->is_rinshan = 1;
                    break;
                }
            }
        }

        if (!ctx->is_rinshan && state->wall_pos == CJ4_LIVE_WALL_END)
            ctx->is_haitei = 1;

        return;
    }

    if (state->phase == CJ4_PHASE_DISCARD &&
        player != state->current_player &&
        state->discard_count > 0)
    {
        cj4_tile_id last = cj4_get_last_discard_tile(state);

        if (last != CJ4_TILE_ID_INVALID &&
            cj4_tile_location_const(state, last)->zone == CJ4_ZONE_HAND &&
            cj4_tile_location_const(state, last)->owner == player)
        {
            ctx->win_type = CJ4_WIN_RON;
            ctx->winning_tile = last;
            ctx->winning_type_id = cj4_tile_get_type(last);
            ctx->has_winning_tile = 1;

            if (state->wall_pos == CJ4_LIVE_WALL_END)
                ctx->is_houtei = 1;
        }

        return;
    }

    if (state->phase == CJ4_PHASE_KAKAN_RESOLVE &&
        state->winning_from_chankan &&
        player != state->current_player &&
        state->pending_kakan_tile != CJ4_TILE_ID_INVALID)
    {
        cj4_tile_id tile = state->pending_kakan_tile;

        if (cj4_tile_location_const(state, tile)->zone == CJ4_ZONE_HAND &&
            cj4_tile_location_const(state, tile)->owner == player)
        {
            ctx->win_type = CJ4_WIN_RON;
            ctx->winning_tile = tile;
            ctx->winning_type_id = cj4_tile_get_type(tile);
            ctx->has_winning_tile = 1;
            ctx->is_chankan = 1;
        }
    }
}

static void
cj4_yaku_collect_context(
    const cj4_mahjong *state,
    cj4_player player,
    cj4_yaku_context *ctx)
{
    memset(ctx, 0, sizeof(*ctx));

    ctx->is_closed_hand = cj4_yaku_is_closed_hand(state, player);
    cj4_yaku_detect_win_state(state, player, ctx);

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

        if (meld->type != CJ4_MELD_ANKAN)
            ctx->has_open_meld = 1;

        if (meld->size == 4)
            ctx->quad_count++;

        for (uint8_t j = 0; j < meld->size; ++j)
        {
            cj4_tile_type type = cj4_tile_get_type(meld->tiles[j]);
            ctx->total_counts[type]++;
            cj4_yaku_note_type(ctx, type);
        }
    }
}

static uint8_t
cj4_yaku_is_chiitoi(const int counts[CJ4_TILE_TYPE_COUNT])
{
    int pair_count = 0;

    if (cj4_yaku_tile_count(counts) != 14)
        return 0;

    for (int i = 0; i < CJ4_TILE_TYPE_COUNT; ++i)
    {
        if (counts[i] != 0 && counts[i] != 2)
            return 0;

        if (counts[i] == 2)
            pair_count++;
    }

    return pair_count == 7;
}

static uint8_t
cj4_yaku_is_kokushi(const int counts[CJ4_TILE_TYPE_COUNT])
{
    static const int terminals[13] = {
        CJ4_TILE_TYPE_1M, CJ4_TILE_TYPE_9M,
        CJ4_TILE_TYPE_1P, CJ4_TILE_TYPE_9P,
        CJ4_TILE_TYPE_1S, CJ4_TILE_TYPE_9S,
        CJ4_TILE_TYPE_EAST, CJ4_TILE_TYPE_SOUTH, CJ4_TILE_TYPE_WEST,
        CJ4_TILE_TYPE_NORTH, CJ4_TILE_TYPE_HAKU, CJ4_TILE_TYPE_HATSU,
        CJ4_TILE_TYPE_CHUN
    };
    int found = 0;
    int duplicates = 0;

    if (cj4_yaku_tile_count(counts) != 14)
        return 0;

    for (int i = 0; i < CJ4_TILE_TYPE_COUNT; ++i)
    {
        uint8_t is_terminal = 0;

        for (int j = 0; j < 13; ++j)
        {
            if (i == terminals[j])
            {
                is_terminal = 1;
                break;
            }
        }

        if (!is_terminal && counts[i] != 0)
            return 0;
    }

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
cj4_yaku_is_kokushi_13_wait(const cj4_yaku_context *ctx)
{
    int counts[CJ4_TILE_TYPE_COUNT];
    static const int terminals[13] = {
        CJ4_TILE_TYPE_1M, CJ4_TILE_TYPE_9M,
        CJ4_TILE_TYPE_1P, CJ4_TILE_TYPE_9P,
        CJ4_TILE_TYPE_1S, CJ4_TILE_TYPE_9S,
        CJ4_TILE_TYPE_EAST, CJ4_TILE_TYPE_SOUTH, CJ4_TILE_TYPE_WEST,
        CJ4_TILE_TYPE_NORTH, CJ4_TILE_TYPE_HAKU, CJ4_TILE_TYPE_HATSU,
        CJ4_TILE_TYPE_CHUN
    };

    if (!ctx->has_winning_tile || ctx->winning_type_id >= CJ4_TILE_TYPE_COUNT)
        return 0;

    memcpy(counts, ctx->concealed_counts, sizeof(counts));

    if (counts[ctx->winning_type_id] == 0)
        return 0;

    counts[ctx->winning_type_id]--;

    for (int i = 0; i < CJ4_TILE_TYPE_COUNT; ++i)
    {
        uint8_t is_terminal = 0;

        for (int j = 0; j < 13; ++j)
        {
            if (i == terminals[j])
            {
                is_terminal = 1;
                break;
            }
        }

        if (!is_terminal && counts[i] != 0)
            return 0;
    }

    for (int i = 0; i < 13; ++i)
    {
        if (counts[terminals[i]] != 1)
            return 0;
    }

    return 1;
}

static uint8_t
cj4_yaku_is_chuuren(const cj4_yaku_context *ctx)
{
    int base;
    const int *counts = ctx->concealed_counts;

    if (!ctx->is_closed_hand || ctx->has_open_meld || ctx->has_honor ||
        ctx->suit_mask == 0 || (ctx->suit_mask & (uint8_t)(ctx->suit_mask - 1)) != 0 ||
        cj4_yaku_tile_count(ctx->concealed_counts) != 14)
        return 0;

    if (ctx->suit_mask & (1u << CJ4_TILE_SUIT_MANZU))
        base = CJ4_TILE_TYPE_1M;
    else if (ctx->suit_mask & (1u << CJ4_TILE_SUIT_PINZU))
        base = CJ4_TILE_TYPE_1P;
    else
        base = CJ4_TILE_TYPE_1S;

    return counts[base + 0] >= 3 &&
           counts[base + 1] >= 1 &&
           counts[base + 2] >= 1 &&
           counts[base + 3] >= 1 &&
           counts[base + 4] >= 1 &&
           counts[base + 5] >= 1 &&
           counts[base + 6] >= 1 &&
           counts[base + 7] >= 1 &&
           counts[base + 8] >= 3;
}

static uint8_t
cj4_yaku_is_junsei_chuuren(const cj4_yaku_context *ctx)
{
    int counts[CJ4_TILE_TYPE_COUNT];
    int base;

    if (!ctx->has_winning_tile || !cj4_yaku_is_chuuren(ctx))
        return 0;

    memcpy(counts, ctx->concealed_counts, sizeof(counts));

    if (counts[ctx->winning_type_id] == 0)
        return 0;

    counts[ctx->winning_type_id]--;

    if (ctx->suit_mask & (1u << CJ4_TILE_SUIT_MANZU))
        base = CJ4_TILE_TYPE_1M;
    else if (ctx->suit_mask & (1u << CJ4_TILE_SUIT_PINZU))
        base = CJ4_TILE_TYPE_1P;
    else
        base = CJ4_TILE_TYPE_1S;

    return counts[base + 0] == 3 &&
           counts[base + 1] == 1 &&
           counts[base + 2] == 1 &&
           counts[base + 3] == 1 &&
           counts[base + 4] == 1 &&
           counts[base + 5] == 1 &&
           counts[base + 6] == 1 &&
           counts[base + 7] == 1 &&
           counts[base + 8] == 3;
}

static uint8_t
cj4_yaku_is_tanyao(const cj4_yaku_context *ctx, const cj4_rules *rules)
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
    return (ctx->has_terminal || ctx->has_honor) && !ctx->has_simple;
}

static uint8_t
cj4_yaku_is_honitsu(const cj4_yaku_context *ctx)
{
    return ctx->has_honor &&
           ctx->suit_mask != 0 &&
           (ctx->suit_mask & (uint8_t)(ctx->suit_mask - 1)) == 0;
}

static uint8_t
cj4_yaku_is_chinitsu(const cj4_yaku_context *ctx)
{
    return !ctx->has_honor &&
           ctx->suit_mask != 0 &&
           (ctx->suit_mask & (uint8_t)(ctx->suit_mask - 1)) == 0;
}

static uint8_t
cj4_yaku_is_tsuuiisou(const cj4_yaku_context *ctx)
{
    return ctx->has_honor && ctx->suit_mask == 0;
}

static uint8_t
cj4_yaku_is_ryuuiisou(const cj4_yaku_context *ctx)
{
    static const uint8_t green[CJ4_TILE_TYPE_COUNT] = {
        0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,
        0,1,1,1,0,1,0,1,0,
        0,0,0,0,0,1,0
    };

    for (int i = 0; i < CJ4_TILE_TYPE_COUNT; ++i)
    {
        if (ctx->total_counts[i] > 0 && !green[i])
            return 0;
    }

    return 1;
}

static uint8_t
cj4_yaku_is_chinroutou(const cj4_yaku_context *ctx)
{
    return ctx->has_terminal && !ctx->has_simple && !ctx->has_honor;
}

static uint8_t
cj4_yaku_count_triplets_in_range(
    const cj4_yaku_context *ctx,
    cj4_tile_type start,
    cj4_tile_type end)
{
    uint8_t count = 0;

    for (cj4_tile_type type = start; type <= end; ++type)
    {
        if (ctx->total_counts[type] >= 3)
            count++;
    }

    return count;
}

static uint8_t
cj4_yaku_count_quads(const cj4_yaku_context *ctx)
{
    return ctx->quad_count;
}

static uint8_t
cj4_yaku_dragon_pair_type(const cj4_yaku_decomposition *decomp)
{
    return decomp->has_pair &&
           decomp->pair_type >= CJ4_TILE_TYPE_HAKU &&
           decomp->pair_type <= CJ4_TILE_TYPE_CHUN;
}

static uint8_t
cj4_yaku_wind_pair_type(const cj4_yaku_decomposition *decomp)
{
    return decomp->has_pair &&
           decomp->pair_type >= CJ4_TILE_TYPE_EAST &&
           decomp->pair_type <= CJ4_TILE_TYPE_NORTH;
}

static uint8_t
cj4_yaku_group_has_yaochu(const cj4_yaku_group *group)
{
    if (group->kind == CJ4_GROUP_SEQUENCE)
    {
        uint8_t number = cj4_tile_type_get_number(group->base_type);
        return number == 1 || number == 7;
    }

    return cj4_tile_type_is_yaochu(group->base_type);
}

static uint8_t
cj4_yaku_is_value_pair(
    const cj4_mahjong *state,
    cj4_player player,
    cj4_tile_type pair_type)
{
    cj4_tile_type seat_type =
        cj4_yaku_wind_tile_type(cj4_yaku_seat_wind(state, player));
    cj4_tile_type round_type =
        cj4_yaku_wind_tile_type(state->round_wind);

    return pair_type == CJ4_TILE_TYPE_HAKU ||
           pair_type == CJ4_TILE_TYPE_HATSU ||
           pair_type == CJ4_TILE_TYPE_CHUN ||
           pair_type == seat_type ||
           pair_type == round_type;
}

static uint8_t
cj4_yaku_is_ryanmen_wait(const cj4_yaku_group *group)
{
    uint8_t number;

    if (group->kind != CJ4_GROUP_SEQUENCE || !group->uses_winning_tile)
        return 0;

    number = cj4_tile_type_get_number(group->base_type);

    if (group->winning_position == 1)
        return 0;

    if (group->winning_position == 0)
        return number != 7;

    return number != 1;
}

static uint8_t
cj4_yaku_open_meld_base_type(const cj4_meld *meld)
{
    cj4_tile_type base = cj4_tile_get_type(meld->tiles[0]);

    if (meld->type == CJ4_MELD_CHI)
    {
        for (uint8_t i = 1; i < meld->size; ++i)
        {
            cj4_tile_type type = cj4_tile_get_type(meld->tiles[i]);
            if (type < base)
                base = type;
        }
    }

    return base;
}

static void
cj4_yaku_init_decomposition(
    const cj4_mahjong *state,
    cj4_player player,
    cj4_yaku_decomposition *decomp)
{
    memset(decomp, 0, sizeof(*decomp));

    for (uint8_t i = 0; i < state->meld_count[player]; ++i)
    {
        const cj4_meld *meld = &state->melds[player][i];
        cj4_yaku_group *group = &decomp->groups[decomp->group_count++];

        group->base_type = cj4_yaku_open_meld_base_type(meld);
        group->uses_winning_tile = 0;
        group->winning_position = 0;

        if (meld->type == CJ4_MELD_CHI)
            group->kind = CJ4_GROUP_SEQUENCE;
        else if (meld->size == 4)
            group->kind = CJ4_GROUP_QUAD;
        else
            group->kind = CJ4_GROUP_TRIPLET;

        group->is_open = (uint8_t)(meld->type != CJ4_MELD_ANKAN);
    }
}

static void
cj4_yaku_apply_count_based_yaku(
    const cj4_mahjong *state,
    cj4_player player,
    const cj4_rules *rules,
    const cj4_yaku_context *ctx,
    cj4_yaku_flags *flags)
{
    uint8_t dragon_triplets;
    uint8_t wind_triplets;

    if (state->is_riichi[player])
        *flags |= CJ4_YAKU_RIICHI;

    if (state->riichi_declared_on_first_turn[player])
        *flags |= CJ4_YAKU_DOUBLE_RIICHI;

    if (rules && rules->ippatsu && state->is_ippatsu[player])
        *flags |= CJ4_YAKU_IPPATSU;

    if (ctx->win_type == CJ4_WIN_TSUMO && ctx->is_closed_hand)
        *flags |= CJ4_YAKU_MENZEN_TSUMO;

    if (ctx->is_rinshan)
        *flags |= CJ4_YAKU_RINSHAN;

    if (ctx->is_haitei)
        *flags |= CJ4_YAKU_HAITEI;

    if (ctx->is_houtei)
        *flags |= CJ4_YAKU_HOUTEI;

    if (ctx->is_chankan)
        *flags |= CJ4_YAKU_CHANKAN;

    if (ctx->win_type == CJ4_WIN_TSUMO &&
        state->first_turn_uninterrupted &&
        state->draw_turn_count[player] == 1)
    {
        if (player == state->dealer && state->discard_count == 0)
            *flags |= CJ4_YAKU_TENHOU;
        else if (player != state->dealer && state->discard_count > 0)
            *flags |= CJ4_YAKU_CHIIHOU;
    }

    if (state->meld_count[player] == 0)
    {
        if (cj4_yaku_is_kokushi(ctx->concealed_counts))
        {
            *flags |= CJ4_YAKU_KOKUSHI;

            if (cj4_yaku_is_kokushi_13_wait(ctx))
                *flags |= CJ4_YAKU_KOKUSHI_13;
        }

        if (cj4_yaku_is_chiitoi(ctx->concealed_counts))
            *flags |= CJ4_YAKU_CHIITOI;

        if (cj4_yaku_is_chuuren(ctx))
        {
            *flags |= CJ4_YAKU_CHUUREN;

            if (cj4_yaku_is_junsei_chuuren(ctx))
                *flags |= CJ4_YAKU_JUNSEI_CHUUREN;
        }
    }

    if (cj4_yaku_is_tanyao(ctx, rules))
        *flags |= CJ4_YAKU_TANYAO;

    if (cj4_yaku_has_yakuhai(state, player, ctx))
        *flags |= CJ4_YAKU_YAKUHAI;

    if (cj4_yaku_is_honroutou(ctx))
        *flags |= CJ4_YAKU_HONROUTOU;

    if (cj4_yaku_is_honitsu(ctx))
        *flags |= CJ4_YAKU_HONITSU;

    if (cj4_yaku_is_chinitsu(ctx))
        *flags |= CJ4_YAKU_CHINITSU;

    if (cj4_yaku_is_tsuuiisou(ctx))
        *flags |= CJ4_YAKU_TSUUIISOU;

    if (cj4_yaku_is_ryuuiisou(ctx))
        *flags |= CJ4_YAKU_RYUUIISOU;

    if (cj4_yaku_is_chinroutou(ctx))
        *flags |= CJ4_YAKU_CHINROUTOU;

    if (cj4_yaku_count_quads(ctx) >= 3)
        *flags |= CJ4_YAKU_SANKANTSU;

    if (cj4_yaku_count_quads(ctx) == 4)
        *flags |= CJ4_YAKU_SUUKANTSU;

    dragon_triplets = cj4_yaku_count_triplets_in_range(
        ctx, CJ4_TILE_TYPE_HAKU, CJ4_TILE_TYPE_CHUN);
    wind_triplets = cj4_yaku_count_triplets_in_range(
        ctx, CJ4_TILE_TYPE_EAST, CJ4_TILE_TYPE_NORTH);

    if (dragon_triplets == 3)
        *flags |= CJ4_YAKU_DAISANGEN;

    if (wind_triplets == 4)
    {
        *flags |= CJ4_YAKU_DAISUUSHII;
        *flags |= CJ4_YAKU_DAISUUSHII_DOUBLE;
    }
}

static void
cj4_yaku_evaluate_decomposition(
    const cj4_mahjong *state,
    cj4_player player,
    const cj4_yaku_context *ctx,
    const cj4_yaku_decomposition *decomp,
    cj4_yaku_flags *flags)
{
    uint8_t triplet_like_count = 0;
    uint8_t concealed_triplets = 0;
    uint8_t quad_count = 0;
    uint8_t has_sequence = 0;
    uint8_t all_sequences = 1;
    uint8_t sequence_bases[27] = {0};
    uint8_t triplet_numbers[9][3] = {{0}};
    uint8_t dragon_triplets = 0;
    uint8_t wind_triplets = 0;

    for (uint8_t i = 0; i < 4; ++i)
    {
        const cj4_yaku_group *group = &decomp->groups[i];

        if (group->kind == CJ4_GROUP_SEQUENCE)
        {
            has_sequence = 1;
            if (group->base_type <= CJ4_TILE_TYPE_7S)
                sequence_bases[group->base_type]++;
        }
        else
        {
            triplet_like_count++;
            all_sequences = 0;

            if (!group->is_open)
            {
                concealed_triplets++;

                if (ctx->win_type == CJ4_WIN_RON && group->uses_winning_tile)
                    concealed_triplets--;
            }

            if (group->kind == CJ4_GROUP_QUAD)
                quad_count++;

            if (group->base_type >= CJ4_TILE_TYPE_1M &&
                group->base_type <= CJ4_TILE_TYPE_9S &&
                cj4_tile_type_get_suit(group->base_type) != CJ4_TILE_SUIT_HONOR)
            {
                uint8_t suit = (uint8_t)cj4_tile_type_get_suit(group->base_type);
                uint8_t number = (uint8_t)(cj4_tile_type_get_number(group->base_type) - 1);
                triplet_numbers[number][suit] = 1;
            }

            if (group->base_type >= CJ4_TILE_TYPE_HAKU &&
                group->base_type <= CJ4_TILE_TYPE_CHUN)
                dragon_triplets++;

            if (group->base_type >= CJ4_TILE_TYPE_EAST &&
                group->base_type <= CJ4_TILE_TYPE_NORTH)
                wind_triplets++;
        }
    }

    if (ctx->quad_count > quad_count)
        quad_count = ctx->quad_count;

    if (ctx->is_closed_hand && all_sequences && decomp->has_pair &&
        !decomp->pair_uses_winning_tile &&
        !cj4_yaku_is_value_pair(state, player, decomp->pair_type))
    {
        uint8_t ryanmen = 0;

        for (uint8_t i = 0; i < 4; ++i)
        {
            if (cj4_yaku_is_ryanmen_wait(&decomp->groups[i]))
            {
                ryanmen = 1;
                break;
            }
        }

        if (ryanmen)
            *flags |= CJ4_YAKU_PINFU;
    }

    if (ctx->is_closed_hand)
    {
        uint8_t pair_count = 0;

        for (int base = 0; base <= CJ4_TILE_TYPE_7S; ++base)
            pair_count += sequence_bases[base] / 2;

        if (pair_count >= 2)
            *flags |= CJ4_YAKU_RYANPEIKOU;
        else if (pair_count >= 1)
            *flags |= CJ4_YAKU_IIPEIKOU;
    }

    for (int n = 0; n < 7; ++n)
    {
        if (sequence_bases[CJ4_TILE_TYPE_1M + n] &&
            sequence_bases[CJ4_TILE_TYPE_1P + n] &&
            sequence_bases[CJ4_TILE_TYPE_1S + n])
        {
            *flags |= CJ4_YAKU_SANSHOKU_DOUJUN;
            break;
        }
    }

    for (int suit = 0; suit < 3; ++suit)
    {
        int base = suit * 9;

        if (sequence_bases[base + 0] &&
            sequence_bases[base + 3] &&
            sequence_bases[base + 6])
        {
            *flags |= CJ4_YAKU_ITTSUU;
            break;
        }
    }

    for (int n = 0; n < 9; ++n)
    {
        if (triplet_numbers[n][0] &&
            triplet_numbers[n][1] &&
            triplet_numbers[n][2])
        {
            *flags |= CJ4_YAKU_SANSHOKU_DOUKOU;
            break;
        }
    }

    if (has_sequence)
    {
        uint8_t all_yaochu = 1;

        for (uint8_t i = 0; i < 4; ++i)
        {
            if (!cj4_yaku_group_has_yaochu(&decomp->groups[i]))
            {
                all_yaochu = 0;
                break;
            }
        }

        if (all_yaochu && cj4_tile_type_is_yaochu(decomp->pair_type))
        {
            *flags |= CJ4_YAKU_CHANTA;

            if (!ctx->has_honor)
                *flags |= CJ4_YAKU_JUNCHAN;
        }
    }

    if (concealed_triplets >= 3)
        *flags |= CJ4_YAKU_SANANKOU;

    if (dragon_triplets == 2 && cj4_yaku_dragon_pair_type(decomp))
        *flags |= CJ4_YAKU_SHOUSANGEN;

    if (wind_triplets == 3 && cj4_yaku_wind_pair_type(decomp))
        *flags |= CJ4_YAKU_SHOUSUUSHII;

    if (!ctx->has_open_meld && triplet_like_count == 4 && concealed_triplets == 4)
    {
        *flags |= CJ4_YAKU_SUUANKOU;

        if (decomp->pair_uses_winning_tile)
            *flags |= CJ4_YAKU_SUUANKOU_TANKI;
    }

    if (triplet_like_count == 4)
        *flags |= CJ4_YAKU_TOITOI;
}

static void
cj4_yaku_search_standard(
    const cj4_mahjong *state,
    cj4_player player,
    const cj4_yaku_context *ctx,
    int counts[CJ4_TILE_TYPE_COUNT],
    uint8_t win_available,
    cj4_yaku_decomposition *decomp,
    cj4_yaku_flags *flags)
{
    int first = -1;

    for (int i = 0; i < CJ4_TILE_TYPE_COUNT; ++i)
    {
        if (counts[i] > 0)
        {
            first = i;
            break;
        }
    }

    if (first < 0)
    {
        if (decomp->has_pair &&
            decomp->group_count == 4 &&
            (!ctx->has_winning_tile || !win_available))
        {
            cj4_yaku_evaluate_decomposition(state, player, ctx, decomp, flags);
        }

        return;
    }

    if (!decomp->has_pair && counts[first] >= 2)
    {
        uint8_t winning_here = ctx->has_winning_tile && win_available &&
                               ctx->winning_type_id == (cj4_tile_type)first;

        if (!winning_here || counts[first] >= 3)
        {
            counts[first] -= 2;
            decomp->has_pair = 1;
            decomp->pair_type = (cj4_tile_type)first;
            decomp->pair_uses_winning_tile = 0;
            cj4_yaku_search_standard(
                state, player, ctx, counts, win_available, decomp, flags);
            decomp->has_pair = 0;
            decomp->pair_uses_winning_tile = 0;
            counts[first] += 2;
        }

        if (winning_here)
        {
            counts[first] -= 2;
            decomp->has_pair = 1;
            decomp->pair_type = (cj4_tile_type)first;
            decomp->pair_uses_winning_tile = 1;
            cj4_yaku_search_standard(
                state, player, ctx, counts, 0, decomp, flags);
            decomp->has_pair = 0;
            decomp->pair_uses_winning_tile = 0;
            counts[first] += 2;
        }
    }

    if (decomp->group_count >= 4)
        return;

    if (counts[first] >= 3)
    {
        cj4_yaku_group *group = &decomp->groups[decomp->group_count];
        uint8_t winning_here = ctx->has_winning_tile && win_available &&
                               ctx->winning_type_id == (cj4_tile_type)first;

        if (!winning_here || counts[first] >= 4)
        {
            counts[first] -= 3;
            *group = (cj4_yaku_group){
                CJ4_GROUP_TRIPLET,
                (cj4_tile_type)first,
                0,
                0,
                0
            };
            decomp->group_count++;
            cj4_yaku_search_standard(
                state, player, ctx, counts, win_available, decomp, flags);
            decomp->group_count--;
            counts[first] += 3;
        }

        if (winning_here)
        {
            counts[first] -= 3;
            *group = (cj4_yaku_group){
                CJ4_GROUP_TRIPLET,
                (cj4_tile_type)first,
                0,
                1,
                0
            };
            decomp->group_count++;
            cj4_yaku_search_standard(
                state, player, ctx, counts, 0, decomp, flags);
            decomp->group_count--;
            counts[first] += 3;
        }
    }

    if (first <= CJ4_TILE_TYPE_7S &&
        first % 9 <= 6 &&
        counts[first + 1] > 0 &&
        counts[first + 2] > 0)
    {
        cj4_yaku_group *group = &decomp->groups[decomp->group_count];
        uint8_t winning_here = 0;
        uint8_t winning_position = 0;

        if (ctx->has_winning_tile && win_available &&
            ctx->winning_type_id >= (cj4_tile_type)first &&
            ctx->winning_type_id <= (cj4_tile_type)(first + 2))
        {
            winning_here = 1;
            winning_position = (uint8_t)(ctx->winning_type_id - first);
        }

        if (!winning_here ||
            counts[ctx->winning_type_id] >= 2)
        {
            counts[first]--;
            counts[first + 1]--;
            counts[first + 2]--;
            *group = (cj4_yaku_group){
                CJ4_GROUP_SEQUENCE,
                (cj4_tile_type)first,
                0,
                0,
                0
            };
            decomp->group_count++;
            cj4_yaku_search_standard(
                state, player, ctx, counts, win_available, decomp, flags);
            decomp->group_count--;
            counts[first]++;
            counts[first + 1]++;
            counts[first + 2]++;
        }

        if (winning_here)
        {
            counts[first]--;
            counts[first + 1]--;
            counts[first + 2]--;
            *group = (cj4_yaku_group){
                CJ4_GROUP_SEQUENCE,
                (cj4_tile_type)first,
                0,
                1,
                winning_position
            };
            decomp->group_count++;
            cj4_yaku_search_standard(
                state, player, ctx, counts, 0, decomp, flags);
            decomp->group_count--;
            counts[first]++;
            counts[first + 1]++;
            counts[first + 2]++;
        }
    }
}

static cj4_yaku_flags
cj4_yaku_collect_flags(
    const cj4_mahjong *state,
    cj4_player player,
    const cj4_rules *rules)
{
    cj4_yaku_context ctx;
    cj4_yaku_decomposition decomp;
    int counts[CJ4_TILE_TYPE_COUNT];
    cj4_yaku_flags flags = 0;

    if (!cj4_is_complete_hand(state, player))
        return 0;

    cj4_yaku_collect_context(state, player, &ctx);
    cj4_yaku_apply_count_based_yaku(state, player, rules, &ctx, &flags);

    if (!cj4_yaku_is_chiitoi(ctx.concealed_counts) &&
        !cj4_yaku_is_kokushi(ctx.concealed_counts))
    {
        memcpy(counts, ctx.concealed_counts, sizeof(counts));
        cj4_yaku_init_decomposition(state, player, &decomp);
        cj4_yaku_search_standard(
            state,
            player,
            &ctx,
            counts,
            (uint8_t)ctx.has_winning_tile,
            &decomp,
            &flags);
    }

    return flags;
}

bool
cj4_has_yaku(
    const cj4_mahjong *state,
    cj4_player player,
    const cj4_rules *rules)
{
    return cj4_yaku_collect_flags(state, player, rules) != 0;
}

typedef struct
{
    uint8_t valid;
    int32_t value;
    cj4_hand_score score;
} cj4_yaku_best_score;

static uint8_t
cj4_yaku_is_winner(const cj4_mahjong *state, cj4_player player)
{
    for (uint8_t i = 0; i < state->winner_count; ++i)
    {
        if (state->winners[i] == player)
            return 1;
    }

    return 0;
}

static cj4_tile_type
cj4_yaku_next_dora_type(cj4_tile_type indicator)
{
    if (indicator <= CJ4_TILE_TYPE_9M)
        return (cj4_tile_type)(CJ4_TILE_TYPE_1M + ((indicator - CJ4_TILE_TYPE_1M + 1) % 9));
    if (indicator <= CJ4_TILE_TYPE_9P)
        return (cj4_tile_type)(CJ4_TILE_TYPE_1P + ((indicator - CJ4_TILE_TYPE_1P + 1) % 9));
    if (indicator <= CJ4_TILE_TYPE_9S)
        return (cj4_tile_type)(CJ4_TILE_TYPE_1S + ((indicator - CJ4_TILE_TYPE_1S + 1) % 9));
    if (indicator <= CJ4_TILE_TYPE_NORTH)
        return (cj4_tile_type)(CJ4_TILE_TYPE_EAST + ((indicator - CJ4_TILE_TYPE_EAST + 1) % 4));

    return (cj4_tile_type)(CJ4_TILE_TYPE_HAKU + ((indicator - CJ4_TILE_TYPE_HAKU + 1) % 3));
}

static uint8_t
cj4_yaku_count_aka_dora(
    const cj4_mahjong *state,
    cj4_player player,
    const cj4_rules *rules)
{
    uint8_t count = 0;

    if (!rules)
        return 0;

    for (int tile = 0; tile < CJ4_TILE_ID_COUNT; ++tile)
    {
        const cj4_location *loc = cj4_tile_location_const(state, (cj4_tile_id)tile);

        if (rules->aka_tiles[tile] &&
            loc->owner == player &&
            (loc->zone == CJ4_ZONE_HAND || loc->zone == CJ4_ZONE_MELD))
        {
            count++;
        }
    }

    return count;
}

static uint8_t
cj4_yaku_count_indicator_dora(
    const cj4_mahjong *state,
    const cj4_yaku_context *ctx,
    uint8_t riichi_only)
{
    uint8_t count = 0;
    uint8_t indicator_count = state->dora_indicators_count;
    const uint8_t *indices = riichi_only ? CJ4_URA_DORA_INDICES : CJ4_DORA_INDICES;

    for (uint8_t i = 0; i < indicator_count && i < CJ4_MAX_DORA; ++i)
    {
        cj4_tile_id indicator_tile = state->wall[indices[i]];
        cj4_tile_type dora_type = cj4_yaku_next_dora_type(cj4_tile_get_type(indicator_tile));
        count += (uint8_t)ctx->total_counts[dora_type];
    }

    return count;
}

static uint8_t
cj4_yaku_count_yakuhai_han(
    const cj4_mahjong *state,
    cj4_player player,
    const cj4_yaku_context *ctx)
{
    uint8_t count = 0;
    cj4_tile_type seat_type =
        cj4_yaku_wind_tile_type(cj4_yaku_seat_wind(state, player));
    cj4_tile_type round_type =
        cj4_yaku_wind_tile_type(state->round_wind);

    if (ctx->total_counts[CJ4_TILE_TYPE_HAKU] >= 3)
        count++;
    if (ctx->total_counts[CJ4_TILE_TYPE_HATSU] >= 3)
        count++;
    if (ctx->total_counts[CJ4_TILE_TYPE_CHUN] >= 3)
        count++;
    if (ctx->total_counts[seat_type] >= 3)
        count++;
    if (ctx->total_counts[round_type] >= 3)
        count++;

    return count;
}

static uint8_t
cj4_yaku_count_yakuman(cj4_yaku_flags flags)
{
    uint8_t count = 0;

    if (flags & CJ4_YAKU_KOKUSHI_13)
        count += 2;
    else if (flags & CJ4_YAKU_KOKUSHI)
        count++;

    if (flags & CJ4_YAKU_SUUANKOU_TANKI)
        count += 2;
    else if (flags & CJ4_YAKU_SUUANKOU)
        count++;

    if (flags & CJ4_YAKU_JUNSEI_CHUUREN)
        count += 2;
    else if (flags & CJ4_YAKU_CHUUREN)
        count++;

    if (flags & CJ4_YAKU_DAISUUSHII_DOUBLE)
        count += 2;
    else if (flags & CJ4_YAKU_DAISUUSHII)
        count++;

    if (flags & CJ4_YAKU_DAISANGEN)
        count++;
    if (flags & CJ4_YAKU_SHOUSUUSHII)
        count++;
    if (flags & CJ4_YAKU_TSUUIISOU)
        count++;
    if (flags & CJ4_YAKU_RYUUIISOU)
        count++;
    if (flags & CJ4_YAKU_CHINROUTOU)
        count++;
    if (flags & CJ4_YAKU_SUUKANTSU)
        count++;
    if (flags & CJ4_YAKU_TENHOU)
        count++;
    if (flags & CJ4_YAKU_CHIIHOU)
        count++;

    return count;
}

static uint8_t
cj4_yaku_count_han(
    const cj4_mahjong *state,
    cj4_player player,
    const cj4_rules *rules,
    const cj4_yaku_context *ctx,
    cj4_yaku_flags flags)
{
    uint8_t han = 0;

    if (flags & CJ4_YAKU_DOUBLE_RIICHI)
        han += 2;
    else if (flags & CJ4_YAKU_RIICHI)
        han += 1;

    if (flags & CJ4_YAKU_IPPATSU)
        han += 1;
    if (flags & CJ4_YAKU_MENZEN_TSUMO)
        han += 1;
    if (flags & CJ4_YAKU_TANYAO)
        han += 1;
    if (flags & CJ4_YAKU_YAKUHAI)
        han += cj4_yaku_count_yakuhai_han(state, player, ctx);
    if (flags & CJ4_YAKU_CHIITOI)
        han += 2;
    if (flags & CJ4_YAKU_TOITOI)
        han += 2;
    if (flags & CJ4_YAKU_HONROUTOU)
        han += 2;
    if (flags & CJ4_YAKU_HONITSU)
        han += ctx->has_open_meld ? 2 : 3;
    if (flags & CJ4_YAKU_CHINITSU)
        han += ctx->has_open_meld ? 5 : 6;
    if (flags & CJ4_YAKU_PINFU)
        han += 1;
    if (flags & CJ4_YAKU_IIPEIKOU)
        han += 1;
    if (flags & CJ4_YAKU_RYANPEIKOU)
        han += 3;
    if (flags & CJ4_YAKU_SANSHOKU_DOUJUN)
        han += ctx->has_open_meld ? 1 : 2;
    if (flags & CJ4_YAKU_ITTSUU)
        han += ctx->has_open_meld ? 1 : 2;
    if (flags & CJ4_YAKU_CHANTA)
        han += ctx->has_open_meld ? 1 : 2;
    if (flags & CJ4_YAKU_JUNCHAN)
        han += ctx->has_open_meld ? 2 : 3;
    if (flags & CJ4_YAKU_SANANKOU)
        han += 2;
    if (flags & CJ4_YAKU_SHOUSANGEN)
        han += 2;
    if (flags & CJ4_YAKU_SANKANTSU)
        han += 2;
    if (flags & CJ4_YAKU_SANSHOKU_DOUKOU)
        han += 2;
    if (flags & CJ4_YAKU_RINSHAN)
        han += 1;
    if (flags & CJ4_YAKU_HAITEI)
        han += 1;
    if (flags & CJ4_YAKU_HOUTEI)
        han += 1;
    if (flags & CJ4_YAKU_CHANKAN)
        han += 1;

    han += cj4_yaku_count_indicator_dora(state, ctx, 0);
    han += cj4_yaku_count_aka_dora(state, player, rules);

    if (state->is_riichi[player])
        han += cj4_yaku_count_indicator_dora(state, ctx, 1);

    return han;
}

static uint8_t
cj4_yaku_pair_fu(
    const cj4_mahjong *state,
    cj4_player player,
    cj4_tile_type pair_type)
{
    uint8_t fu = 0;
    cj4_tile_type seat_type =
        cj4_yaku_wind_tile_type(cj4_yaku_seat_wind(state, player));
    cj4_tile_type round_type =
        cj4_yaku_wind_tile_type(state->round_wind);

    if (pair_type >= CJ4_TILE_TYPE_HAKU &&
        pair_type <= CJ4_TILE_TYPE_CHUN)
    {
        fu += 2;
    }

    if (pair_type == seat_type)
        fu += 2;
    if (pair_type == round_type)
        fu += 2;

    return fu;
}

static uint16_t
cj4_yaku_group_fu(
    const cj4_yaku_context *ctx,
    const cj4_yaku_group *group)
{
    uint8_t open;
    uint8_t yaochu;

    if (group->kind == CJ4_GROUP_SEQUENCE)
        return 0;

    open = group->is_open;
    if (!open &&
        ctx->win_type == CJ4_WIN_RON &&
        group->uses_winning_tile)
    {
        open = 1;
    }

    yaochu = cj4_tile_type_is_yaochu(group->base_type);

    if (group->kind == CJ4_GROUP_QUAD)
        return (uint16_t)(open ? (yaochu ? 16 : 8) : (yaochu ? 32 : 16));

    return (uint16_t)(open ? (yaochu ? 4 : 2) : (yaochu ? 8 : 4));
}

static uint16_t
cj4_yaku_wait_fu(
    cj4_yaku_flags flags,
    const cj4_yaku_decomposition *decomp)
{
    if (!decomp || (flags & CJ4_YAKU_PINFU))
        return 0;

    if (decomp->pair_uses_winning_tile)
        return 2;

    for (uint8_t i = 0; i < 4; ++i)
    {
        const cj4_yaku_group *group = &decomp->groups[i];

        if (group->uses_winning_tile &&
            group->kind == CJ4_GROUP_SEQUENCE &&
            !cj4_yaku_is_ryanmen_wait(group))
        {
            return 2;
        }
    }

    return 0;
}

static uint16_t
cj4_yaku_calculate_fu(
    const cj4_mahjong *state,
    cj4_player player,
    const cj4_yaku_context *ctx,
    const cj4_yaku_decomposition *decomp,
    cj4_yaku_flags flags)
{
    uint16_t fu = 20;

    if (flags & CJ4_YAKU_CHIITOI)
        return 25;

    if ((flags & CJ4_YAKU_PINFU) &&
        ctx->win_type == CJ4_WIN_TSUMO)
    {
        return 20;
    }

    if (ctx->win_type == CJ4_WIN_RON &&
        ctx->is_closed_hand)
    {
        fu += 10;
    }

    if (ctx->win_type == CJ4_WIN_TSUMO &&
        !(flags & CJ4_YAKU_PINFU))
    {
        fu += 2;
    }

    if (decomp)
    {
        if (decomp->has_pair)
            fu += cj4_yaku_pair_fu(state, player, decomp->pair_type);

        fu += cj4_yaku_wait_fu(flags, decomp);

        for (uint8_t i = 0; i < 4; ++i)
            fu += cj4_yaku_group_fu(ctx, &decomp->groups[i]);
    }

    if (fu == 20 && ctx->win_type == CJ4_WIN_RON)
        fu = 30;

    return (uint16_t)(((fu + 9) / 10) * 10);
}

static int32_t
cj4_yaku_round_up_100(int32_t value)
{
    return ((value + 99) / 100) * 100;
}

static void
cj4_yaku_fill_basic_points(
    const cj4_yaku_context *ctx,
    uint8_t han,
    uint16_t fu,
    uint8_t yakuman_count,
    cj4_hand_score *out)
{
    int32_t base_points;
    uint8_t is_dealer = ctx->win_type != CJ4_WIN_NONE;

    (void)is_dealer;

    memset(out, 0, sizeof(*out));
    out->is_valid = 1;
    out->han = han;
    out->fu = fu;
    out->yakuman_count = yakuman_count;

    if (yakuman_count > 0)
    {
        base_points = 8000 * yakuman_count;
    }
    else if (han >= 13)
    {
        out->yakuman_count = 1;
        base_points = 8000;
    }
    else if (han >= 11)
    {
        base_points = 6000;
    }
    else if (han >= 8)
    {
        base_points = 4000;
    }
    else if (han >= 6)
    {
        base_points = 3000;
    }
    else
    {
        base_points = fu * (1 << (han + 2));
        if (han >= 5 ||
            (han == 4 && fu >= 40) ||
            (han == 3 && fu >= 70))
        {
            base_points = 2000;
        }
    }

    out->ron_points = base_points;
    out->tsumo_dealer_payment = base_points;
    out->tsumo_non_dealer_payment = base_points;
}

static int32_t
cj4_yaku_score_value(
    const cj4_yaku_context *ctx,
    cj4_player player,
    const cj4_mahjong *state,
    const cj4_hand_score *score)
{
    uint8_t is_dealer = player == state->dealer;

    if (ctx->win_type == CJ4_WIN_TSUMO)
    {
        if (is_dealer)
            return score->tsumo_non_dealer_payment * 3;

        return score->tsumo_dealer_payment +
               score->tsumo_non_dealer_payment * 2;
    }

    return score->ron_points;
}

static void
cj4_yaku_finalize_points(
    const cj4_yaku_context *ctx,
    const cj4_mahjong *state,
    cj4_player player,
    cj4_hand_score *score)
{
    int32_t base = score->ron_points;
    uint8_t is_dealer = player == state->dealer;

    if (ctx->win_type == CJ4_WIN_TSUMO)
    {
        if (is_dealer)
        {
            score->tsumo_non_dealer_payment =
                cj4_yaku_round_up_100(base * 2);
            score->tsumo_dealer_payment = 0;
            score->ron_points = 0;
        }
        else
        {
            score->tsumo_non_dealer_payment =
                cj4_yaku_round_up_100(base);
            score->tsumo_dealer_payment =
                cj4_yaku_round_up_100(base * 2);
            score->ron_points = 0;
        }
    }
    else
    {
        score->ron_points = cj4_yaku_round_up_100(base * (is_dealer ? 6 : 4));
        score->tsumo_non_dealer_payment = 0;
        score->tsumo_dealer_payment = 0;
    }
}

static void
cj4_yaku_consider_score(
    const cj4_mahjong *state,
    cj4_player player,
    const cj4_rules *rules,
    const cj4_yaku_context *ctx,
    const cj4_yaku_decomposition *decomp,
    cj4_yaku_flags flags,
    cj4_yaku_best_score *best)
{
    cj4_hand_score candidate;
    uint8_t yakuman_count = cj4_yaku_count_yakuman(flags);
    uint8_t han;
    uint16_t fu;
    int32_t value;

    if (flags == 0)
        return;

    han = yakuman_count == 0
        ? cj4_yaku_count_han(state, player, rules, ctx, flags)
        : 0;
    fu = cj4_yaku_calculate_fu(state, player, ctx, decomp, flags);

    cj4_yaku_fill_basic_points(ctx, han, fu, yakuman_count, &candidate);
    cj4_yaku_finalize_points(ctx, state, player, &candidate);
    value = cj4_yaku_score_value(ctx, player, state, &candidate);

    if (!best->valid ||
        value > best->value ||
        (value == best->value && candidate.han > best->score.han) ||
        (value == best->value && candidate.han == best->score.han &&
         candidate.fu > best->score.fu))
    {
        best->valid = 1;
        best->value = value;
        best->score = candidate;
    }
}

static void
cj4_yaku_search_best_standard(
    const cj4_mahjong *state,
    cj4_player player,
    const cj4_rules *rules,
    const cj4_yaku_context *ctx,
    int counts[CJ4_TILE_TYPE_COUNT],
    uint8_t win_available,
    cj4_yaku_decomposition *decomp,
    cj4_yaku_flags base_flags,
    cj4_yaku_best_score *best)
{
    int first = -1;

    for (int i = 0; i < CJ4_TILE_TYPE_COUNT; ++i)
    {
        if (counts[i] > 0)
        {
            first = i;
            break;
        }
    }

    if (first < 0)
    {
        if (decomp->has_pair &&
            decomp->group_count == 4 &&
            (!ctx->has_winning_tile || !win_available))
        {
            cj4_yaku_flags flags = base_flags;
            cj4_yaku_evaluate_decomposition(state, player, ctx, decomp, &flags);
            cj4_yaku_consider_score(state, player, rules, ctx, decomp, flags, best);
        }

        return;
    }

    if (!decomp->has_pair && counts[first] >= 2)
    {
        uint8_t winning_here = ctx->has_winning_tile && win_available &&
                               ctx->winning_type_id == (cj4_tile_type)first;

        if (!winning_here || counts[first] >= 3)
        {
            counts[first] -= 2;
            decomp->has_pair = 1;
            decomp->pair_type = (cj4_tile_type)first;
            decomp->pair_uses_winning_tile = 0;
            cj4_yaku_search_best_standard(
                state, player, rules, ctx, counts, win_available, decomp, base_flags, best);
            decomp->has_pair = 0;
            decomp->pair_uses_winning_tile = 0;
            counts[first] += 2;
        }

        if (winning_here)
        {
            counts[first] -= 2;
            decomp->has_pair = 1;
            decomp->pair_type = (cj4_tile_type)first;
            decomp->pair_uses_winning_tile = 1;
            cj4_yaku_search_best_standard(
                state, player, rules, ctx, counts, 0, decomp, base_flags, best);
            decomp->has_pair = 0;
            decomp->pair_uses_winning_tile = 0;
            counts[first] += 2;
        }
    }

    if (decomp->group_count >= 4)
        return;

    if (counts[first] >= 3)
    {
        cj4_yaku_group *group = &decomp->groups[decomp->group_count];
        uint8_t winning_here = ctx->has_winning_tile && win_available &&
                               ctx->winning_type_id == (cj4_tile_type)first;

        if (!winning_here || counts[first] >= 4)
        {
            counts[first] -= 3;
            *group = (cj4_yaku_group){
                CJ4_GROUP_TRIPLET,
                (cj4_tile_type)first,
                0,
                0,
                0
            };
            decomp->group_count++;
            cj4_yaku_search_best_standard(
                state, player, rules, ctx, counts, win_available, decomp, base_flags, best);
            decomp->group_count--;
            counts[first] += 3;
        }

        if (winning_here)
        {
            counts[first] -= 3;
            *group = (cj4_yaku_group){
                CJ4_GROUP_TRIPLET,
                (cj4_tile_type)first,
                0,
                1,
                0
            };
            decomp->group_count++;
            cj4_yaku_search_best_standard(
                state, player, rules, ctx, counts, 0, decomp, base_flags, best);
            decomp->group_count--;
            counts[first] += 3;
        }
    }

    if (first <= CJ4_TILE_TYPE_7S &&
        first % 9 <= 6 &&
        counts[first + 1] > 0 &&
        counts[first + 2] > 0)
    {
        cj4_yaku_group *group = &decomp->groups[decomp->group_count];
        uint8_t winning_here = 0;
        uint8_t winning_position = 0;

        if (ctx->has_winning_tile && win_available &&
            ctx->winning_type_id >= (cj4_tile_type)first &&
            ctx->winning_type_id <= (cj4_tile_type)(first + 2))
        {
            winning_here = 1;
            winning_position = (uint8_t)(ctx->winning_type_id - first);
        }

        if (!winning_here || counts[ctx->winning_type_id] >= 2)
        {
            counts[first]--;
            counts[first + 1]--;
            counts[first + 2]--;
            *group = (cj4_yaku_group){
                CJ4_GROUP_SEQUENCE,
                (cj4_tile_type)first,
                0,
                0,
                0
            };
            decomp->group_count++;
            cj4_yaku_search_best_standard(
                state, player, rules, ctx, counts, win_available, decomp, base_flags, best);
            decomp->group_count--;
            counts[first]++;
            counts[first + 1]++;
            counts[first + 2]++;
        }

        if (winning_here)
        {
            counts[first]--;
            counts[first + 1]--;
            counts[first + 2]--;
            *group = (cj4_yaku_group){
                CJ4_GROUP_SEQUENCE,
                (cj4_tile_type)first,
                0,
                1,
                winning_position
            };
            decomp->group_count++;
            cj4_yaku_search_best_standard(
                state, player, rules, ctx, counts, 0, decomp, base_flags, best);
            decomp->group_count--;
            counts[first]++;
            counts[first + 1]++;
            counts[first + 2]++;
        }
    }
}

static uint8_t
cj4_yaku_collect_best_score(
    const cj4_mahjong *state,
    cj4_player player,
    const cj4_rules *rules,
    cj4_hand_score *out)
{
    cj4_yaku_context ctx;
    cj4_yaku_decomposition decomp;
    cj4_yaku_flags flags = 0;
    cj4_yaku_best_score best = {0};
    int counts[CJ4_TILE_TYPE_COUNT];

    if (!cj4_is_complete_hand(state, player))
        return 0;

    cj4_yaku_collect_context(state, player, &ctx);
    cj4_yaku_apply_count_based_yaku(state, player, rules, &ctx, &flags);

    if (flags & (CJ4_YAKU_CHIITOI | CJ4_YAKU_KOKUSHI | CJ4_YAKU_KOKUSHI_13))
        cj4_yaku_consider_score(state, player, rules, &ctx, NULL, flags, &best);

    if (!(flags & CJ4_YAKU_CHIITOI) &&
        !(flags & CJ4_YAKU_KOKUSHI) &&
        !(flags & CJ4_YAKU_KOKUSHI_13))
    {
        memcpy(counts, ctx.concealed_counts, sizeof(counts));
        cj4_yaku_init_decomposition(state, player, &decomp);
        cj4_yaku_search_best_standard(
            state,
            player,
            rules,
            &ctx,
            counts,
            (uint8_t)ctx.has_winning_tile,
            &decomp,
            flags,
            &best);
    }

    if (!best.valid)
        return 0;

    *out = best.score;
    return 1;
}

static uint8_t
cj4_yaku_prepare_round_end_state(
    const cj4_mahjong *state,
    cj4_player player,
    cj4_mahjong *prepared)
{
    if (state->phase != CJ4_PHASE_ROUND_END)
    {
        *prepared = *state;
        return 1;
    }

    if (!cj4_yaku_is_winner(state, player) ||
        state->winning_tile == CJ4_TILE_ID_INVALID)
        return 0;

    *prepared = *state;
    prepared->locations[state->winning_tile].zone = CJ4_ZONE_HAND;
    prepared->locations[state->winning_tile].owner = player;

    if (state->winner_count == 1 &&
        state->winner == player &&
        state->current_player == player &&
        state->draw_tile == state->winning_tile)
    {
        prepared->phase = CJ4_PHASE_DRAW;
        prepared->current_player = player;
        prepared->draw_tile = state->winning_tile;
        return 1;
    }

    if (state->pending_kakan_tile == state->winning_tile)
    {
        prepared->phase = CJ4_PHASE_KAKAN_RESOLVE;
        prepared->current_player = state->loser;
        prepared->winning_from_chankan = 1;
        prepared->pending_kakan_tile = state->winning_tile;
        prepared->draw_tile = CJ4_TILE_ID_INVALID;
        return 1;
    }

    prepared->phase = CJ4_PHASE_DISCARD;
    prepared->current_player = state->loser;
    prepared->draw_tile = CJ4_TILE_ID_INVALID;
    prepared->winning_from_chankan = 0;
    prepared->pending_kakan_tile = CJ4_TILE_ID_INVALID;

    return 1;
}

bool
cj4_calculate_hand_score(
    const cj4_mahjong *state,
    cj4_player player,
    const cj4_rules *rules,
    cj4_hand_score *out)
{
    cj4_mahjong prepared;

    if (!out)
        return false;

    memset(out, 0, sizeof(*out));

    if (!cj4_yaku_prepare_round_end_state(state, player, &prepared))
        return false;

    return cj4_yaku_collect_best_score(&prepared, player, rules, out) != 0;
}
