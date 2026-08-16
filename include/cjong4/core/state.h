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
    uint8_t pending_kan_dora;
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

/* Packed-state accessors. Setters normalize all reserved bits. */
static inline cj4_phase
cj4_state_phase(
    const cj4_mahjong *s)
{
    return (cj4_phase)(s->progress & 0x0fu);
}
static inline void
cj4_state_set_phase(
    cj4_mahjong *s,
    cj4_phase value)
{
    s->progress = (uint8_t)((s->progress & 0xf0u) | ((uint8_t)value & 0x0fu));
}
static inline cj4_player
cj4_state_current_player(
    const cj4_mahjong *s)
{
    return (cj4_player)((s->progress >> 4) & 0x03u);
}
static inline void
cj4_state_set_current_player(
    cj4_mahjong *s,
    cj4_player value)
{
    s->progress = (uint8_t)((s->progress & 0xcfu) | (((uint8_t)value & 3u) << 4));
}
static inline bool
cj4_state_first_turn(
    const cj4_mahjong *s)
{
    return (s->progress & 0x40u) != 0;
}
static inline void
cj4_state_set_first_turn(
    cj4_mahjong *s,
    bool value)
{
    s->progress = value ? (uint8_t)(s->progress | 0x40u)
                        : (uint8_t)(s->progress & 0xbfu);
}
static inline bool
cj4_state_is_chankan(
    const cj4_mahjong *s)
{
    return (s->progress & 0x80u) != 0;
}
static inline void
cj4_state_set_chankan(
    cj4_mahjong *s,
    bool value)
{
    s->progress = value ? (uint8_t)(s->progress | 0x80u)
                        : (uint8_t)(s->progress & 0x7fu);
}

static inline bool
cj4_state_is_riichi(
    const cj4_mahjong *s,
    cj4_player p)
{
    return (s->riichi_ippatsu & (1u << p)) != 0;
}
static inline void
cj4_state_set_riichi(
    cj4_mahjong *s,
    cj4_player p,
    bool v)
{
    uint8_t bit = (uint8_t)(1u << p);
    s->riichi_ippatsu = v ? (uint8_t)(s->riichi_ippatsu | bit)
                          : (uint8_t)(s->riichi_ippatsu & (uint8_t)~bit);
}
static inline bool
cj4_state_is_ippatsu(
    const cj4_mahjong *s,
    cj4_player p)
{
    return (s->riichi_ippatsu & (0x10u << p)) != 0;
}
static inline void
cj4_state_set_ippatsu(
    cj4_mahjong *s,
    cj4_player p,
    bool v)
{
    uint8_t bit = (uint8_t)(0x10u << p);
    s->riichi_ippatsu = v ? (uint8_t)(s->riichi_ippatsu | bit)
                          : (uint8_t)(s->riichi_ippatsu & (uint8_t)~bit);
}
static inline bool
cj4_state_temporary_furiten(
    const cj4_mahjong *s,
    cj4_player p)
{
    return (s->furiten & (1u << p)) != 0;
}
static inline void
cj4_state_set_temporary_furiten(
    cj4_mahjong *s,
    cj4_player p,
    bool v)
{
    uint8_t bit = (uint8_t)(1u << p);
    s->furiten = v ? (uint8_t)(s->furiten | bit)
                   : (uint8_t)(s->furiten & (uint8_t)~bit);
}
static inline bool
cj4_state_riichi_furiten(
    const cj4_mahjong *s,
    cj4_player p)
{
    return (s->furiten & (0x10u << p)) != 0;
}
static inline void
cj4_state_set_riichi_furiten(
    cj4_mahjong *s,
    cj4_player p,
    bool v)
{
    uint8_t bit = (uint8_t)(0x10u << p);
    s->furiten = v ? (uint8_t)(s->furiten | bit)
                   : (uint8_t)(s->furiten & (uint8_t)~bit);
}
static inline bool
cj4_state_double_riichi(
    const cj4_mahjong *s,
    cj4_player p)
{
    return (s->double_riichi_pending & (1u << p)) != 0;
}
static inline void
cj4_state_set_double_riichi(
    cj4_mahjong *s,
    cj4_player p,
    bool v)
{
    uint8_t bit = (uint8_t)(1u << p);
    s->double_riichi_pending = v ? (uint8_t)(s->double_riichi_pending | bit)
                                 : (uint8_t)(s->double_riichi_pending & (uint8_t)~bit);
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
static inline void
cj4_state_set_pending_riichi(
    cj4_mahjong *s,
    cj4_player p,
    bool is_double)
{
    s->double_riichi_pending = (uint8_t)((s->double_riichi_pending & 0x0fu) |
                                         ((uint8_t)p << 4) |
                                         (is_double ? 0x80u : 0u));
}
static inline void
cj4_state_clear_pending_riichi_bits(
    cj4_mahjong *s)
{
    s->double_riichi_pending = (uint8_t)((s->double_riichi_pending & 0x0fu) | 0x70u);
}
static inline uint8_t
cj4_state_draw_turn(
    const cj4_mahjong *s,
    cj4_player p)
{
    return (uint8_t)((s->draw_turns >> (2u * p)) & 3u);
}
static inline void
cj4_state_set_draw_turn(
    cj4_mahjong *s,
    cj4_player p,
    uint8_t value)
{
    uint8_t shift = (uint8_t)(2u * p);
    uint8_t mask = (uint8_t)(3u << shift);
    s->draw_turns = (uint8_t)((s->draw_turns & (uint8_t)~mask) |
                              ((value & 3u) << shift));
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
static inline void
cj4_state_set_pao(
    cj4_mahjong *s,
    cj4_player owner,
    cj4_player liable,
    cj4_pao_type type)
{
    uint8_t shift = (uint8_t)(4u * (owner & 1u));
    uint8_t mask = (uint8_t)(0x0fu << shift);
    uint8_t value = type == CJ4_PAO_NONE ? 0u : (uint8_t)(((uint8_t)type << 2) | liable);
    s->pao[owner / 2] = (uint8_t)((s->pao[owner / 2] & (uint8_t)~mask) | (value << shift));
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
static inline void
cj4_state_set_round_result(
    cj4_mahjong *s,
    cj4_round_end_type type,
    cj4_abortive_draw_reason reason)
{
    s->round_result = (uint8_t)(((uint8_t)type & 7u) | (((uint8_t)reason & 7u) << 3));
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
