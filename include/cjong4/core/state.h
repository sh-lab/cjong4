#ifndef CJ4_STATE_H
#define CJ4_STATE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "location.h"
#include "phase.h"
#include "player.h"
#include "tile.h"
#include "wind.h"

#define CJ4_MAX_DRAWS 70
#define CJ4_MAX_DISCARDS 86
#define CJ4_MAX_HAND_TILES 14
#define CJ4_MAX_MELDS 4
#define CJ4_CALLED_INDEX_NONE 255

enum
{
    CJ4_MAX_DORA_INDICATORS = 5
};

typedef struct
{
    cj4_tile_id tile;
    cj4_player player;
    uint8_t is_active;
    uint8_t is_tsumogiri;
    uint8_t is_riichi;
} cj4_discard;

typedef struct
{
    cj4_tile_id tiles[4];
    uint8_t size;
    cj4_meld_type type;
    cj4_player from_player;
    uint8_t called_index;
} cj4_meld;

typedef struct
{
    cj4_tile_id items[CJ4_MAX_HAND_TILES];
    uint8_t count;
} cj4_hand;

typedef struct
{
    cj4_discard items[CJ4_MAX_DISCARDS];
    uint8_t count;
} cj4_discard_list;

typedef struct
{
    cj4_meld items[CJ4_MAX_MELDS];
    uint8_t count;
} cj4_meld_list;

typedef struct
{
    cj4_tile_id items[CJ4_MAX_DORA_INDICATORS];
    uint8_t count;
} cj4_dora_indicator_list;

typedef enum
{
    CJ4_ROUND_END_NONE,
    CJ4_ROUND_END_TSUMO,
    CJ4_ROUND_END_RON,
    CJ4_ROUND_END_EXHAUSTIVE_DRAW,
    CJ4_ROUND_END_ABORTIVE_DRAW
} cj4_round_end_type;

typedef enum
{
    CJ4_ABORTIVE_DRAW_NONE,
    CJ4_ABORTIVE_DRAW_KYUUSHU_KYUUHAI,
    CJ4_ABORTIVE_DRAW_SUUFON_RENDA,
    CJ4_ABORTIVE_DRAW_FOUR_RIICHI,
    CJ4_ABORTIVE_DRAW_FOUR_KANS,
    CJ4_ABORTIVE_DRAW_TRIPLE_RON
} cj4_abortive_draw_reason;

typedef enum
{
    CJ4_PAO_NONE,
    CJ4_PAO_DAISANGEN,
    CJ4_PAO_DAISUUSHII,
    CJ4_PAO_SUUKANTSU
} cj4_pao_type;

typedef struct
{
    cj4_location locations[CJ4_TILE_ID_COUNT];

    int32_t scores[CJ4_PLAYER_COUNT];
    cj4_wind round_wind;
    cj4_player dealer;
    uint8_t honba;
    uint8_t riichi_sticks;

    /* cfppssss */
    uint8_t progress;
    /* iiiirrrr, RRRRTTTT, dpppDDDD, two bits per player */
    uint8_t riichi_ippatsu;
    uint8_t furiten;
    uint8_t double_riichi_pending;
    uint8_t draw_turns;
    /* player 1|0, player 3|2; each nibble is ttpp. */
    uint8_t pao[2];
    /* rraaattt, xxxxwwww, winning tile */
    uint8_t round_result;
    uint8_t winner_mask;
    cj4_tile_id winning_tile;

    cj4_tile_id draw_tile;
    cj4_tile_id last_discard_tile;
    cj4_tile_id pending_kakan_tile;
    cj4_tile_id pending_ankan_tile;
    cj4_tile_id pending_ankan_tiles[4];

    uint8_t wall_pos;
    uint8_t dead_wall_draw_count;
    union
    {
        uint8_t dora_count;
        uint8_t dora_indicators_count;
    };
    uint8_t pending_kan_dora_count;
    uint8_t discard_count;

    cj4_wind next_round_wind;
    cj4_player next_dealer;
    uint8_t settlement_should_end;
} cj4_mahjong;

#if defined(__cplusplus)
static_assert(offsetof(cj4_mahjong, winning_tile) + sizeof(cj4_tile_id) -
                      offsetof(cj4_mahjong, progress) ==
                  10,
              "packed progress and round result must be 10 bytes");
#else
_Static_assert(offsetof(cj4_mahjong, winning_tile) + sizeof(cj4_tile_id) -
                       offsetof(cj4_mahjong, progress) ==
                   10,
               "packed progress and round result must be 10 bytes");
#endif

/* Read-only packed-state accessors. */
static inline cj4_phase
cj4_state_phase(
    const cj4_mahjong *s)
{
    return (cj4_phase)(s->progress & 0x0fu);
}
static inline cj4_player
cj4_state_current_player(
    const cj4_mahjong *s)
{
    return (cj4_player)((s->progress >> 4) & 0x03u);
}
static inline bool
cj4_state_first_turn(
    const cj4_mahjong *s)
{
    return (s->progress & 0x40u) != 0;
}
static inline bool
cj4_state_is_chankan(
    const cj4_mahjong *s)
{
    return (s->progress & 0x80u) != 0;
}

static inline bool
cj4_state_is_riichi(
    const cj4_mahjong *s,
    cj4_player p)
{
    return (s->riichi_ippatsu & (1u << p)) != 0;
}
static inline bool
cj4_state_is_ippatsu(
    const cj4_mahjong *s,
    cj4_player p)
{
    return (s->riichi_ippatsu & (0x10u << p)) != 0;
}
static inline bool
cj4_state_temporary_furiten(
    const cj4_mahjong *s,
    cj4_player p)
{
    return (s->furiten & (1u << p)) != 0;
}
static inline bool
cj4_state_riichi_furiten(
    const cj4_mahjong *s,
    cj4_player p)
{
    return (s->furiten & (0x10u << p)) != 0;
}
static inline bool
cj4_state_double_riichi(
    const cj4_mahjong *s,
    cj4_player p)
{
    return (s->double_riichi_pending & (1u << p)) != 0;
}
static inline bool
cj4_state_has_pending_riichi(
    const cj4_mahjong *s)
{
    return ((s->double_riichi_pending >> 4) & 7u) != 7u;
}
static inline cj4_player
cj4_state_pending_riichi_player(
    const cj4_mahjong *s)
{
    return (cj4_player)((s->double_riichi_pending >> 4) & 7u);
}
static inline bool
cj4_state_pending_riichi_is_double(
    const cj4_mahjong *s)
{
    return (s->double_riichi_pending & 0x80u) != 0;
}
static inline uint8_t
cj4_state_draw_turn(
    const cj4_mahjong *s,
    cj4_player p)
{
    return (uint8_t)((s->draw_turns >> (2u * p)) & 3u);
}
static inline cj4_pao_type
cj4_state_pao_type(
    const cj4_mahjong *s,
    cj4_player p)
{
    uint8_t nibble = (uint8_t)((s->pao[p / 2] >> (4u * (p & 1u))) & 0x0fu);
    return (cj4_pao_type)((nibble >> 2) & 3u);
}
static inline cj4_player
cj4_state_pao_player(
    const cj4_mahjong *s,
    cj4_player p)
{
    return (cj4_player)((s->pao[p / 2] >> (4u * (p & 1u))) & 3u);
}
static inline cj4_round_end_type
cj4_state_round_end_type(
    const cj4_mahjong *s)
{
    return (cj4_round_end_type)(s->round_result & 7u);
}
static inline cj4_abortive_draw_reason
cj4_state_abortive_reason(
    const cj4_mahjong *s)
{
    return (cj4_abortive_draw_reason)((s->round_result >> 3) & 7u);
}
static inline uint8_t
cj4_state_winner_count(
    const cj4_mahjong *s)
{
    uint8_t value = (uint8_t)(s->winner_mask & 0x0fu);
    value = (uint8_t)(value - ((value >> 1) & 0x55u));
    value = (uint8_t)((value & 0x33u) + ((value >> 2) & 0x33u));
    return (uint8_t)((value + (value >> 4)) & 0x0fu);
}
static inline bool
cj4_state_is_winner(
    const cj4_mahjong *s,
    cj4_player p)
{
    return (s->winner_mask & (1u << p)) != 0;
}

#endif /* CJ4_STATE_H */
