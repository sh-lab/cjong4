#ifndef CJ4_STATE_INTERNAL_H
#define CJ4_STATE_INTERNAL_H

#include "location_internal.h"
#include "state.h"

#if defined(__cplusplus)
extern "C"
{
#endif

#define CJ4_LIVE_WALL_END 122

    enum
    {
        CJ4_RINSHAN_TILE_COUNT = CJ4_TILE_PER_TYPE,
        CJ4_MAX_DORA = CJ4_MAX_DORA_INDICATORS
    };

    static const uint8_t CJ4_RINSHAN_INDICES[CJ4_RINSHAN_TILE_COUNT] = {
        134,
        135,
        132,
        133};

    static const uint8_t CJ4_DORA_INDICES[CJ4_MAX_DORA] = {
        130,
        128,
        126,
        124,
        122};

    static const uint8_t CJ4_URA_DORA_INDICES[CJ4_MAX_DORA] = {
        131,
        129,
        127,
        125,
        123};

    static inline void
    cj4_state_set_phase(
        cj4_mahjong *s,
        cj4_phase value)
    {
        s->progress =
            (uint8_t)((s->progress & (uint8_t)~CJ4_STATE_PHASE_MASK) |
                      ((uint8_t)value & CJ4_STATE_PHASE_MASK));
    }

    static inline void
    cj4_state_set_current_player(
        cj4_mahjong *s,
        cj4_player value)
    {
        s->progress =
            (uint8_t)((s->progress &
                       (uint8_t)~CJ4_STATE_CURRENT_PLAYER_MASK) |
                      (((uint8_t)value & CJ4_STATE_PAO_PLAYER_MASK)
                       << CJ4_STATE_CURRENT_PLAYER_SHIFT));
    }

    static inline void
    cj4_state_set_first_turn(
        cj4_mahjong *s,
        bool value)
    {
        s->progress = value
                          ? (uint8_t)(s->progress |
                                      CJ4_STATE_FIRST_TURN_FLAG)
                          : (uint8_t)(s->progress &
                                      (uint8_t)~CJ4_STATE_FIRST_TURN_FLAG);
    }

    static inline void
    cj4_state_set_chankan(
        cj4_mahjong *s,
        bool value)
    {
        s->progress = value
                          ? (uint8_t)(s->progress | CJ4_STATE_CHANKAN_FLAG)
                          : (uint8_t)(s->progress &
                                      (uint8_t)~CJ4_STATE_CHANKAN_FLAG);
    }

    static inline void
    cj4_state_set_riichi(
        cj4_mahjong *s,
        cj4_player p,
        bool value)
    {
        uint8_t bit = (uint8_t)(CJ4_STATE_RIICHI_FLAG << p);
        s->riichi_ippatsu = value
                                ? (uint8_t)(s->riichi_ippatsu | bit)
                                : (uint8_t)(s->riichi_ippatsu & (uint8_t)~bit);
    }

    static inline void
    cj4_state_set_ippatsu(
        cj4_mahjong *s,
        cj4_player p,
        bool value)
    {
        uint8_t bit = (uint8_t)(CJ4_STATE_IPPATSU_FLAG << p);
        s->riichi_ippatsu = value
                                ? (uint8_t)(s->riichi_ippatsu | bit)
                                : (uint8_t)(s->riichi_ippatsu & (uint8_t)~bit);
    }

    static inline void
    cj4_state_set_temporary_furiten(
        cj4_mahjong *s,
        cj4_player p,
        bool value)
    {
        uint8_t bit = (uint8_t)(CJ4_STATE_TEMPORARY_FURITEN_FLAG << p);
        s->furiten = value ? (uint8_t)(s->furiten | bit)
                           : (uint8_t)(s->furiten & (uint8_t)~bit);
    }

    static inline void
    cj4_state_set_riichi_furiten(
        cj4_mahjong *s,
        cj4_player p,
        bool value)
    {
        uint8_t bit = (uint8_t)(CJ4_STATE_RIICHI_FURITEN_FLAG << p);
        s->furiten = value ? (uint8_t)(s->furiten | bit)
                           : (uint8_t)(s->furiten & (uint8_t)~bit);
    }

    static inline void
    cj4_state_set_double_riichi(
        cj4_mahjong *s,
        cj4_player p,
        bool value)
    {
        uint8_t bit = (uint8_t)(CJ4_STATE_RIICHI_FLAG << p);
        s->double_riichi_pending = value
                                       ? (uint8_t)(s->double_riichi_pending | bit)
                                       : (uint8_t)(s->double_riichi_pending & (uint8_t)~bit);
    }

    static inline void
    cj4_state_set_pending_riichi(
        cj4_mahjong *s,
        cj4_player p,
        bool is_double)
    {
        s->double_riichi_pending =
            (uint8_t)((s->double_riichi_pending &
                       CJ4_STATE_PLAYER_FLAGS_MASK) |
                      ((uint8_t)p << CJ4_STATE_PENDING_RIICHI_PLAYER_SHIFT) |
                      (is_double
                           ? CJ4_STATE_PENDING_DOUBLE_RIICHI_FLAG
                           : 0u));
    }

    static inline void
    cj4_state_clear_pending_riichi_bits(
        cj4_mahjong *s)
    {
        s->double_riichi_pending =
            (uint8_t)((s->double_riichi_pending &
                       CJ4_STATE_PLAYER_FLAGS_MASK) |
                      (CJ4_STATE_PENDING_RIICHI_PLAYER_NONE
                       << CJ4_STATE_PENDING_RIICHI_PLAYER_SHIFT));
    }

    static inline void
    cj4_state_set_draw_turn(
        cj4_mahjong *s,
        cj4_player p,
        uint8_t value)
    {
        uint8_t shift = (uint8_t)(CJ4_STATE_DRAW_TURN_BITS * p);
        uint8_t mask = (uint8_t)(CJ4_STATE_DRAW_TURN_MASK << shift);
        s->draw_turns = (uint8_t)((s->draw_turns & (uint8_t)~mask) |
                                  ((value & CJ4_STATE_DRAW_TURN_MASK) << shift));
    }

    static inline void
    cj4_state_set_pao(
        cj4_mahjong *s,
        cj4_player owner,
        cj4_player liable,
        cj4_pao_type type)
    {
        uint8_t shift = (uint8_t)(CJ4_STATE_PAO_BITS_PER_PLAYER *
                                  (owner % CJ4_STATE_PAO_PLAYERS_PER_BYTE));
        uint8_t mask = (uint8_t)(CJ4_STATE_PAO_ENTRY_MASK << shift);
        uint8_t value = type == CJ4_PAO_NONE
                            ? 0u
                            : (uint8_t)(((uint8_t)type
                                         << CJ4_STATE_PAO_TYPE_SHIFT) |
                                        liable);
        s->pao[owner / CJ4_STATE_PAO_PLAYERS_PER_BYTE] =
            (uint8_t)((s->pao[owner /
                              CJ4_STATE_PAO_PLAYERS_PER_BYTE] &
                       (uint8_t)~mask) |
                      (value << shift));
    }

    static inline void
    cj4_state_set_round_result(
        cj4_mahjong *s,
        cj4_round_end_type type,
        cj4_abortive_draw_reason reason)
    {
        s->round_result =
            (uint8_t)(((uint8_t)type & CJ4_STATE_ROUND_END_TYPE_MASK) |
                      (((uint8_t)reason &
                        CJ4_STATE_ABORTIVE_REASON_MASK)
                       << CJ4_STATE_ABORTIVE_REASON_SHIFT));
    }

    static inline const cj4_location *
    cj4_state_tile_location_const(
        const cj4_mahjong *state,
        cj4_tile_id tile)
    {
        return &state->locations[tile];
    }

#if defined(__cplusplus)
}
#endif

#endif /* CJ4_STATE_INTERNAL_H */
