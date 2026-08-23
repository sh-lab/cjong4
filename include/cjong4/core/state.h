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
    CJ4_MAX_DORA_INDICATORS = 5,

    CJ4_STATE_PHASE_MASK = 0x0f,
    CJ4_STATE_CURRENT_PLAYER_SHIFT = 4,
    CJ4_STATE_CURRENT_PLAYER_MASK = 0x30,
    CJ4_STATE_FIRST_TURN_FLAG = 0x40,
    CJ4_STATE_CHANKAN_FLAG = 0x80,

    CJ4_STATE_PLAYER_FLAGS_MASK = 0x0f,
    CJ4_STATE_PLAYER_FLAG = 0x01,
    CJ4_STATE_RIICHI_FLAG = CJ4_STATE_PLAYER_FLAG,
    CJ4_STATE_IPPATSU_FLAG = 0x10,
    CJ4_STATE_TEMPORARY_FURITEN_FLAG = 0x01,
    CJ4_STATE_RIICHI_FURITEN_FLAG = 0x10,

    CJ4_STATE_PENDING_RIICHI_PLAYER_SHIFT = 4,
    CJ4_STATE_PENDING_RIICHI_PLAYER_MASK = 0x70,
    CJ4_STATE_PENDING_RIICHI_PLAYER_NONE = 7,
    CJ4_STATE_PENDING_DOUBLE_RIICHI_FLAG = 0x80,

    CJ4_STATE_DRAW_TURN_BITS = 2,
    CJ4_STATE_DRAW_TURN_MASK = 0x03,

    CJ4_STATE_PAO_PLAYERS_PER_BYTE = 2,
    CJ4_STATE_PAO_BYTE_COUNT =
        CJ4_PLAYER_COUNT / CJ4_STATE_PAO_PLAYERS_PER_BYTE,
    CJ4_STATE_PAO_BITS_PER_PLAYER = 4,
    CJ4_STATE_PAO_ENTRY_MASK = 0x0f,
    CJ4_STATE_PAO_TYPE_SHIFT = 2,
    CJ4_STATE_PAO_TYPE_MASK = 0x03,
    CJ4_STATE_PAO_PLAYER_MASK = 0x03,

    CJ4_STATE_ROUND_END_TYPE_MASK = 0x07,
    CJ4_STATE_ABORTIVE_REASON_SHIFT = 3,
    CJ4_STATE_ABORTIVE_REASON_MASK = 0x07,

    CJ4_STATE_PACKED_BYTE_COUNT = 10
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
    uint8_t pao[CJ4_STATE_PAO_BYTE_COUNT];
    /* rraaattt, xxxxwwww, winning tile */
    uint8_t round_result;
    uint8_t winner_mask;
    cj4_tile_id winning_tile;

    cj4_tile_id draw_tile;
    cj4_tile_id last_discard_tile;
    cj4_tile_id pending_kakan_tile;
    cj4_tile_id pending_ankan_tile;
    cj4_tile_id pending_ankan_tiles[CJ4_TILE_PER_TYPE];

    uint8_t wall_pos;
    uint8_t dead_wall_draw_count;
    uint8_t dora_count;
    uint8_t pending_kan_dora_count;
    uint8_t discard_count;

    cj4_wind next_round_wind;
    cj4_player next_dealer;
    uint8_t settlement_should_end;
} cj4_mahjong;

#if defined(__cplusplus)
static_assert(offsetof(cj4_mahjong, winning_tile) + sizeof(cj4_tile_id) -
                      offsetof(cj4_mahjong, progress) ==
                  CJ4_STATE_PACKED_BYTE_COUNT,
              "packed progress and round result must be 10 bytes");
#else
_Static_assert(offsetof(cj4_mahjong, winning_tile) + sizeof(cj4_tile_id) -
                       offsetof(cj4_mahjong, progress) ==
                   CJ4_STATE_PACKED_BYTE_COUNT,
               "packed progress and round result must be 10 bytes");
#endif

/* Read-only packed-state accessors. */
static inline cj4_phase
cj4_state_phase(
    const cj4_mahjong *s)
{
    return (cj4_phase)(s->progress & CJ4_STATE_PHASE_MASK);
}
static inline cj4_player
cj4_state_current_player(
    const cj4_mahjong *s)
{
    return (cj4_player)((s->progress & CJ4_STATE_CURRENT_PLAYER_MASK) >>
                        CJ4_STATE_CURRENT_PLAYER_SHIFT);
}
static inline bool
cj4_state_first_turn(
    const cj4_mahjong *s)
{
    return (s->progress & CJ4_STATE_FIRST_TURN_FLAG) != 0;
}
static inline bool
cj4_state_is_chankan(
    const cj4_mahjong *s)
{
    return (s->progress & CJ4_STATE_CHANKAN_FLAG) != 0;
}

static inline bool
cj4_state_is_riichi(
    const cj4_mahjong *s,
    cj4_player p)
{
    return (s->riichi_ippatsu & (CJ4_STATE_RIICHI_FLAG << p)) != 0;
}
static inline bool
cj4_state_is_ippatsu(
    const cj4_mahjong *s,
    cj4_player p)
{
    return (s->riichi_ippatsu & (CJ4_STATE_IPPATSU_FLAG << p)) != 0;
}
static inline bool
cj4_state_temporary_furiten(
    const cj4_mahjong *s,
    cj4_player p)
{
    return (s->furiten & (CJ4_STATE_TEMPORARY_FURITEN_FLAG << p)) != 0;
}
static inline bool
cj4_state_riichi_furiten(
    const cj4_mahjong *s,
    cj4_player p)
{
    return (s->furiten & (CJ4_STATE_RIICHI_FURITEN_FLAG << p)) != 0;
}
static inline bool
cj4_state_double_riichi(
    const cj4_mahjong *s,
    cj4_player p)
{
    return (s->double_riichi_pending & (CJ4_STATE_RIICHI_FLAG << p)) != 0;
}
static inline bool
cj4_state_has_pending_riichi(
    const cj4_mahjong *s)
{
    return ((s->double_riichi_pending &
             CJ4_STATE_PENDING_RIICHI_PLAYER_MASK) >>
            CJ4_STATE_PENDING_RIICHI_PLAYER_SHIFT) !=
           CJ4_STATE_PENDING_RIICHI_PLAYER_NONE;
}
static inline cj4_player
cj4_state_pending_riichi_player(
    const cj4_mahjong *s)
{
    return (cj4_player)((s->double_riichi_pending &
                         CJ4_STATE_PENDING_RIICHI_PLAYER_MASK) >>
                        CJ4_STATE_PENDING_RIICHI_PLAYER_SHIFT);
}
static inline bool
cj4_state_pending_riichi_is_double(
    const cj4_mahjong *s)
{
    return (s->double_riichi_pending &
            CJ4_STATE_PENDING_DOUBLE_RIICHI_FLAG) != 0;
}
static inline uint8_t
cj4_state_draw_turn(
    const cj4_mahjong *s,
    cj4_player p)
{
    return (uint8_t)((s->draw_turns >> (CJ4_STATE_DRAW_TURN_BITS * p)) &
                     CJ4_STATE_DRAW_TURN_MASK);
}
static inline cj4_pao_type
cj4_state_pao_type(
    const cj4_mahjong *s,
    cj4_player p)
{
    uint8_t shift = (uint8_t)(CJ4_STATE_PAO_BITS_PER_PLAYER *
                              (p % CJ4_STATE_PAO_PLAYERS_PER_BYTE));
    uint8_t entry = (uint8_t)((s->pao[p / CJ4_STATE_PAO_PLAYERS_PER_BYTE] >>
                               shift) &
                              CJ4_STATE_PAO_ENTRY_MASK);
    return (cj4_pao_type)((entry >> CJ4_STATE_PAO_TYPE_SHIFT) &
                          CJ4_STATE_PAO_TYPE_MASK);
}
static inline cj4_player
cj4_state_pao_player(
    const cj4_mahjong *s,
    cj4_player p)
{
    uint8_t shift = (uint8_t)(CJ4_STATE_PAO_BITS_PER_PLAYER *
                              (p % CJ4_STATE_PAO_PLAYERS_PER_BYTE));
    return (cj4_player)((s->pao[p / CJ4_STATE_PAO_PLAYERS_PER_BYTE] >> shift) &
                        CJ4_STATE_PAO_PLAYER_MASK);
}
static inline cj4_round_end_type
cj4_state_round_end_type(
    const cj4_mahjong *s)
{
    return (cj4_round_end_type)(s->round_result &
                                CJ4_STATE_ROUND_END_TYPE_MASK);
}
static inline cj4_abortive_draw_reason
cj4_state_abortive_reason(
    const cj4_mahjong *s)
{
    return (cj4_abortive_draw_reason)((s->round_result >> CJ4_STATE_ABORTIVE_REASON_SHIFT) &
                                      CJ4_STATE_ABORTIVE_REASON_MASK);
}
static inline uint8_t
cj4_state_winner_count(
    const cj4_mahjong *s)
{
    uint8_t count = 0;
    for (cj4_player p = 0; p < CJ4_PLAYER_COUNT; ++p)
        if ((s->winner_mask & (CJ4_STATE_PLAYER_FLAG << p)) != 0)
            ++count;
    return count;
}
static inline bool
cj4_state_is_winner(
    const cj4_mahjong *s,
    cj4_player p)
{
    return (s->winner_mask & (CJ4_STATE_PLAYER_FLAG << p)) != 0;
}

#endif /* CJ4_STATE_H */
