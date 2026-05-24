#ifndef CJ4_STATE_SCORE_H
#define CJ4_STATE_SCORE_H

#include <stdbool.h>
#include <stdint.h>

#include "rules.h"
#include "state.h"

typedef struct
{
    uint8_t is_valid;
    uint8_t han;
    uint16_t fu;
    uint8_t yakuman_count;
    int32_t ron_points;
    int32_t tsumo_dealer_payment;
    int32_t tsumo_non_dealer_payment;
} cj4_hand_score;

bool cj4_player_is_shape_tenpai(
    const cj4_mahjong *state,
    cj4_player player);

bool cj4_calculate_hand_score(
    const cj4_mahjong *state,
    cj4_player player,
    const cj4_rules *rules,
    cj4_hand_score *out);

#endif /* CJ4_STATE_SCORE_H */
