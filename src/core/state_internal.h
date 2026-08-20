#ifndef CJ4_STATE_INTERNAL_H
#define CJ4_STATE_INTERNAL_H

#include "location_internal.h"
#include "state.h"

#if defined(__cplusplus)
extern "C"
{
#endif

#define CJ4_MAX_DORA 5
#define CJ4_LIVE_WALL_END 122

    static const uint8_t CJ4_RINSHAN_INDICES[4] = {
        134,
        135,
        132,
        133};

    static const uint8_t CJ4_DORA_INDICES[5] = {
        130,
        128,
        126,
        124,
        122};

    static const uint8_t CJ4_URA_DORA_INDICES[5] = {
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
        s->progress = (uint8_t)((s->progress & 0xf0u) | ((uint8_t)value & 0x0fu));
    }

    static inline void
    cj4_state_set_current_player(
        cj4_mahjong *s,
        cj4_player value)
    {
        s->progress = (uint8_t)((s->progress & 0xcfu) | (((uint8_t)value & 3u) << 4));
    }

    static inline void
    cj4_state_set_first_turn(
        cj4_mahjong *s,
        bool value)
    {
        s->progress = value ? (uint8_t)(s->progress | 0x40u)
                            : (uint8_t)(s->progress & 0xbfu);
    }

    static inline void
    cj4_state_set_chankan(
        cj4_mahjong *s,
        bool value)
    {
        s->progress = value ? (uint8_t)(s->progress | 0x80u)
                            : (uint8_t)(s->progress & 0x7fu);
    }

    static inline void
    cj4_state_set_riichi(
        cj4_mahjong *s,
        cj4_player p,
        bool value)
    {
        uint8_t bit = (uint8_t)(1u << p);
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
        uint8_t bit = (uint8_t)(0x10u << p);
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
        uint8_t bit = (uint8_t)(1u << p);
        s->furiten = value ? (uint8_t)(s->furiten | bit)
                           : (uint8_t)(s->furiten & (uint8_t)~bit);
    }

    static inline void
    cj4_state_set_riichi_furiten(
        cj4_mahjong *s,
        cj4_player p,
        bool value)
    {
        uint8_t bit = (uint8_t)(0x10u << p);
        s->furiten = value ? (uint8_t)(s->furiten | bit)
                           : (uint8_t)(s->furiten & (uint8_t)~bit);
    }

    static inline void
    cj4_state_set_double_riichi(
        cj4_mahjong *s,
        cj4_player p,
        bool value)
    {
        uint8_t bit = (uint8_t)(1u << p);
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
        s->double_riichi_pending = (uint8_t)((s->double_riichi_pending & 0x0fu) |
                                             ((uint8_t)p << 4) |
                                             (is_double ? 0x80u : 0u));
    }

    static inline void
    cj4_state_clear_pending_riichi_bits(
        cj4_mahjong *s)
    {
        s->double_riichi_pending =
            (uint8_t)((s->double_riichi_pending & 0x0fu) | 0x70u);
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

    static inline void
    cj4_state_set_pao(
        cj4_mahjong *s,
        cj4_player owner,
        cj4_player liable,
        cj4_pao_type type)
    {
        uint8_t shift = (uint8_t)(4u * (owner & 1u));
        uint8_t mask = (uint8_t)(0x0fu << shift);
        uint8_t value = type == CJ4_PAO_NONE
                            ? 0u
                            : (uint8_t)(((uint8_t)type << 2) | liable);
        s->pao[owner / 2] =
            (uint8_t)((s->pao[owner / 2] & (uint8_t)~mask) | (value << shift));
    }

    static inline void
    cj4_state_set_round_result(
        cj4_mahjong *s,
        cj4_round_end_type type,
        cj4_abortive_draw_reason reason)
    {
        s->round_result =
            (uint8_t)(((uint8_t)type & 7u) | (((uint8_t)reason & 7u) << 3));
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
