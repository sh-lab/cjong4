#include "state_settle.h"

#include "state_score.h"

#include <assert.h>

static uint8_t
cj4_settle_player_has_top_score(
    const cj4_mahjong *state,
    cj4_player player)
{
    for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
    {
        if (state->scores[i] > state->scores[player])
            return 0;
    }

    return 1;
}

static uint8_t
cj4_settle_dealer_wins(
    const cj4_mahjong *state)
{
    for (uint8_t i = 0; i < state->winner_count; ++i)
    {
        if (state->winners[i] == state->dealer)
            return 1;
    }

    return 0;
}

static uint8_t
cj4_settle_has_nagashi_mangan(
    const cj4_mahjong *state)
{
    for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
    {
        if (state->nagashi_mangan[i])
            return 1;
    }

    return 0;
}

static uint8_t
cj4_settle_first_nagashi_mangan_player(
    const cj4_mahjong *state)
{
    for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
    {
        if (state->nagashi_mangan[i])
            return i;
    }

    return CJ4_PLAYER_COUNT;
}

static uint8_t
cj4_settle_rule_bool(
    const cj4_rules *rules,
    uint8_t value,
    uint8_t default_value)
{
    if (!rules || rules->version == 0)
        return default_value;

    return value != 0;
}

static uint8_t
cj4_settle_pao_type_enabled(
    const cj4_rules *rules,
    cj4_pao_type type)
{
    switch (type)
    {
    case CJ4_PAO_DAISANGEN:
        return cj4_settle_rule_bool(rules, rules ? rules->pao_daisangen : 0, 1);
    case CJ4_PAO_DAISUUSHII:
        return cj4_settle_rule_bool(rules, rules ? rules->pao_daisuushii : 0, 1);
    case CJ4_PAO_SUUKANTSU:
        return cj4_settle_rule_bool(rules, rules ? rules->pao_suukantsu : 0, 1);
    case CJ4_PAO_NONE:
    default:
        return 0;
    }
}

static uint8_t
cj4_settle_get_pao_player(
    const cj4_mahjong *state,
    const cj4_rules *rules,
    cj4_player winner,
    cj4_player *out_player)
{
    if (!rules || !rules->pao || !state->pao_owner[winner])
        return 0;

    if (!cj4_settle_pao_type_enabled(rules, state->pao_type[winner]))
        return 0;

    if (state->pao_player[winner] >= CJ4_PLAYER_COUNT)
        return 0;

    *out_player = state->pao_player[winner];
    return 1;
}

static uint8_t
cj4_settle_pao_responsible_yakuman_count(
    const cj4_mahjong *state,
    const cj4_rules *rules,
    cj4_player winner,
    const cj4_hand_score *score)
{
    uint8_t responsible = 1;

    if (!rules || !rules->pao_liability_only ||
        score->yakuman_count <= 1)
    {
        return score->yakuman_count;
    }

    if (state->pao_type[winner] == CJ4_PAO_DAISUUSHII &&
        cj4_settle_rule_bool(rules, rules->daisuushii_double, 1))
    {
        responsible = 2;
    }

    return responsible > score->yakuman_count ? score->yakuman_count : responsible;
}

static uint8_t
cj4_settle_any_negative_score(
    const cj4_mahjong *state)
{
    for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
    {
        if (state->scores[i] < 0)
            return 1;
    }

    return 0;
}

static cj4_wind
cj4_settle_last_round_wind(
    const cj4_rules *rules)
{
    if (!rules)
        return CJ4_WIND_SOUTH;

    switch (rules->game_type)
    {
    case CJ4_GAME_TONPUU:
        return CJ4_WIND_EAST;
    case CJ4_GAME_FULL:
        return CJ4_WIND_NORTH;
    case CJ4_GAME_HANCHAN:
    default:
        return CJ4_WIND_SOUTH;
    }
}

static cj4_wind
cj4_settle_extension_last_round_wind(
    const cj4_rules *rules)
{
    if (!rules)
        return CJ4_WIND_SOUTH;

    switch (rules->game_type)
    {
    case CJ4_GAME_TONPUU:
        return CJ4_WIND_SOUTH;
    case CJ4_GAME_HANCHAN:
        return CJ4_WIND_WEST;
    case CJ4_GAME_FULL:
    default:
        return CJ4_WIND_NORTH;
    }
}

static uint8_t
cj4_settle_is_endgame_round(
    const cj4_mahjong *state,
    cj4_wind last_round_wind)
{
    return state->round_wind > last_round_wind ||
           (state->round_wind == last_round_wind &&
            state->dealer == CJ4_PLAYER_3);
}

static void
cj4_settle_apply_tsumo(
    cj4_mahjong *next,
    const cj4_mahjong *state,
    const cj4_rules *rules,
    const cj4_hand_score *score)
{
    int32_t honba_payment = state->honba * 100;
    cj4_player winner = state->winner;
    cj4_player pao_player = CJ4_PLAYER_COUNT;

    if (cj4_settle_get_pao_player(state, rules, winner, &pao_player))
    {
        uint8_t responsible_yakuman =
            cj4_settle_pao_responsible_yakuman_count(state, rules, winner, score);
        int32_t total;

        if (rules && rules->pao_liability_only &&
            score->yakuman_count > responsible_yakuman &&
            responsible_yakuman > 0)
        {
            for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
            {
                int32_t payment;
                int32_t pao_part;

                if (i == winner)
                    continue;

                if (winner == state->dealer)
                    payment = score->tsumo_non_dealer_payment + honba_payment;
                else if (i == state->dealer)
                    payment = score->tsumo_dealer_payment + honba_payment;
                else
                    payment = score->tsumo_non_dealer_payment + honba_payment;

                next->scores[i] -= payment;
                next->scores[winner] += payment;

                pao_part = (payment - honba_payment) *
                           responsible_yakuman /
                           score->yakuman_count;

                if (i != pao_player)
                {
                    next->scores[i] += pao_part;
                    next->scores[pao_player] -= pao_part;
                }
            }

            return;
        }

        if (winner == state->dealer)
        {
            total = (score->tsumo_non_dealer_payment + honba_payment) *
                    (CJ4_PLAYER_COUNT - 1);
        }
        else
        {
            total = score->tsumo_dealer_payment + honba_payment +
                    (score->tsumo_non_dealer_payment + honba_payment) *
                        (CJ4_PLAYER_COUNT - 2);
        }

        next->scores[pao_player] -= total;
        next->scores[winner] += total;
        return;
    }

    if (winner == state->dealer)
    {
        int32_t payment = score->tsumo_non_dealer_payment + honba_payment;

        for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
        {
            if (i == winner)
                continue;

            next->scores[i] -= payment;
            next->scores[winner] += payment;
        }
    }
    else
    {
        int32_t dealer_payment = score->tsumo_dealer_payment + honba_payment;
        int32_t child_payment = score->tsumo_non_dealer_payment + honba_payment;

        for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
        {
            if (i == winner)
                continue;

            if (i == state->dealer)
            {
                next->scores[i] -= dealer_payment;
                next->scores[winner] += dealer_payment;
            }
            else
            {
                next->scores[i] -= child_payment;
                next->scores[winner] += child_payment;
            }
        }
    }
}

static void
cj4_settle_apply_ron(
    cj4_mahjong *next,
    const cj4_mahjong *state,
    const cj4_rules *rules)
{
    int32_t honba_bonus = state->honba * 300;

    for (uint8_t i = 0; i < state->winner_count; ++i)
    {
        cj4_hand_score score = {0};
        cj4_player winner = state->winners[i];
        int32_t winner_honba_bonus = honba_bonus;
        int32_t total;
        bool calculated =
            cj4_calculate_hand_score(state, winner, rules, &score);

        assert(calculated);
        if (!calculated)
            continue;

        if (i > 0 &&
            cj4_settle_rule_bool(
                rules,
                rules ? rules->multi_ron_honba_first_only : 0,
                0))
        {
            winner_honba_bonus = 0;
        }

        total = score.ron_points + winner_honba_bonus;

        cj4_player pao_player = CJ4_PLAYER_COUNT;

        if (cj4_settle_get_pao_player(state, rules, winner, &pao_player) &&
            pao_player != state->loser)
        {
            int32_t loser_payment;
            int32_t pao_payment;

            if (rules && rules->pao_liability_only &&
                score.yakuman_count > 0)
            {
                uint8_t responsible_yakuman =
                    cj4_settle_pao_responsible_yakuman_count(state, rules, winner, &score);
                pao_payment = score.ron_points *
                              responsible_yakuman /
                              score.yakuman_count;
                loser_payment = total - pao_payment;
            }
            else
            {
                loser_payment = score.ron_points / 2;
                pao_payment = score.ron_points - loser_payment + winner_honba_bonus;
            }

            next->scores[state->loser] -= loser_payment;
            next->scores[pao_player] -= pao_payment;
            next->scores[winner] += total;
        }
        else
        {
            next->scores[state->loser] -= total;
            next->scores[winner] += total;
        }
    }
}

static void
cj4_settle_apply_nagashi_mangan(
    cj4_mahjong *next,
    const cj4_mahjong *state)
{
    const int32_t dealer_payment = 4000;
    const int32_t child_payment = 2000;
    int32_t honba_payment = state->honba * 100;

    for (uint8_t winner = 0; winner < CJ4_PLAYER_COUNT; ++winner)
    {
        if (!state->nagashi_mangan[winner])
            continue;

        for (uint8_t payer = 0; payer < CJ4_PLAYER_COUNT; ++payer)
        {
            int32_t payment;

            if (payer == winner || state->nagashi_mangan[payer])
                continue;

            if (winner == state->dealer)
                payment = dealer_payment + honba_payment;
            else if (payer == state->dealer)
                payment = dealer_payment + honba_payment;
            else
                payment = child_payment + honba_payment;

            next->scores[payer] -= payment;
            next->scores[winner] += payment;
        }
    }
}

static void
cj4_settle_apply_draw(
    cj4_mahjong *next,
    const cj4_mahjong *state,
    const cj4_rules *rules,
    uint8_t tenpai[CJ4_PLAYER_COUNT])
{
    uint8_t tenpai_count = 0;

    for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
    {
        tenpai[i] = (uint8_t)cj4_player_is_shape_tenpai(state, (cj4_player)i);
        tenpai_count += tenpai[i];
    }

    if (rules &&
        rules->noten_penalty &&
        tenpai_count > 0 &&
        tenpai_count < CJ4_PLAYER_COUNT)
    {
        int32_t total_points = rules->noten_penalty_points;
        int32_t tenpai_gain = total_points / tenpai_count;
        int32_t noten_loss = total_points / (CJ4_PLAYER_COUNT - tenpai_count);

        for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
        {
            if (tenpai[i])
                next->scores[i] += tenpai_gain;
            else
                next->scores[i] -= noten_loss;
        }
    }
}

static void
cj4_settle_determine_progress(
    cj4_mahjong *next,
    const cj4_mahjong *state,
    const cj4_rules *rules,
    uint8_t dealer_continues)
{
    cj4_wind last_round_wind = cj4_settle_last_round_wind(rules);
    cj4_wind extension_last_round_wind =
        cj4_settle_extension_last_round_wind(rules);
    uint8_t reached_target = 0;
    int32_t target_score = rules ? rules->target_score : 30000;

    next->next_dealer = state->dealer;
    next->next_round_wind = state->round_wind;

    if (!dealer_continues)
    {
        next->next_dealer = (cj4_player)((state->dealer + 1) % CJ4_PLAYER_COUNT);

        if (next->next_dealer == CJ4_PLAYER_0 &&
            next->next_round_wind < CJ4_WIND_NORTH)
        {
            next->next_round_wind = (cj4_wind)(next->next_round_wind + 1);
        }
    }

    for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
    {
        if (next->scores[i] >= target_score)
        {
            reached_target = 1;
            break;
        }
    }

    next->settlement_should_end = 0;

    if (rules &&
        rules->tobi_end &&
        cj4_settle_any_negative_score(next))
    {
        next->settlement_should_end = 1;
        return;
    }

    if (cj4_settle_is_endgame_round(state, last_round_wind))
    {
        if (!dealer_continues && state->dealer == CJ4_PLAYER_3)
        {
            if (reached_target ||
                state->round_wind >= extension_last_round_wind)
            {
                next->settlement_should_end = 1;
                return;
            }
        }

        if (reached_target)
        {
            if (!dealer_continues ||
                cj4_settle_player_has_top_score(next, state->dealer))
            {
                next->settlement_should_end = 1;
                return;
            }
        }
    }
}

bool
cj4_can_settle(
    const cj4_mahjong state)
{
    return state.phase == CJ4_PHASE_ROUND_END;
}

cj4_mahjong
cj4_do_settle(
    const cj4_mahjong state,
    const cj4_rules *rules)
{
    cj4_mahjong next = state;
    uint8_t tenpai[CJ4_PLAYER_COUNT] = {0};
    uint8_t dealer_continues = 0;

    assert(cj4_can_settle(state));

    if (state.winner_count > 0)
    {
        if (state.winner_count == 1 &&
            state.winner == state.current_player &&
            state.draw_tile == state.winning_tile)
        {
            cj4_hand_score score = {0};
            bool calculated = cj4_calculate_hand_score(
                &state,
                state.winner,
                rules,
                &score);

            assert(calculated);
            if (calculated)
                cj4_settle_apply_tsumo(&next, &state, rules, &score);
        }
        else
        {
            cj4_settle_apply_ron(&next, &state, rules);
        }

        if (state.riichi_sticks > 0)
        {
            next.scores[state.winners[0]] += state.riichi_sticks * 1000;
            next.riichi_sticks = 0;
        }

        dealer_continues = cj4_settle_dealer_wins(&state);
        next.honba = dealer_continues ? (uint8_t)(state.honba + 1) : 0;
    }
    else
    {
        if (cj4_settle_has_nagashi_mangan(&state))
        {
            uint8_t stick_winner;

            cj4_settle_apply_nagashi_mangan(&next, &state);
            if (cj4_settle_rule_bool(
                    rules,
                    rules ? rules->nagashi_dealer_tenpai_renchan : 0,
                    0))
            {
                dealer_continues = (uint8_t)cj4_player_is_shape_tenpai(
                    &state,
                    state.dealer);
            }
            else
            {
                dealer_continues = state.nagashi_mangan[state.dealer];
            }

            stick_winner = cj4_settle_first_nagashi_mangan_player(&state);
            if (stick_winner < CJ4_PLAYER_COUNT && state.riichi_sticks > 0)
            {
                next.scores[stick_winner] += state.riichi_sticks * 1000;
                next.riichi_sticks = 0;
            }
        }
        else if (state.round_end_type != CJ4_ROUND_END_ABORTIVE_DRAW)
        {
            cj4_settle_apply_draw(&next, &state, rules, tenpai);
            dealer_continues = tenpai[state.dealer];
        }
        else
        {
            dealer_continues = 1;
        }

        next.honba = dealer_continues ? (uint8_t)(state.honba + 1) : 0;
    }

    cj4_settle_determine_progress(&next, &state, rules, dealer_continues);

    if (next.settlement_should_end && dealer_continues && next.honba > state.honba)
    {
        next.honba = state.honba;
    }

    next.phase = CJ4_PHASE_SETTLE;

    return next;
}
