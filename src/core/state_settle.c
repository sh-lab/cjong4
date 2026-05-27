#include "state_settle.h"

#include "state_score.h"

#include <assert.h>

static uint8_t
cj4_settle_player_has_top_score(const cj4_mahjong *state, cj4_player player)
{
    for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
    {
        if (state->scores[i] > state->scores[player])
            return 0;
    }

    return 1;
}

static uint8_t
cj4_settle_dealer_wins(const cj4_mahjong *state)
{
    for (uint8_t i = 0; i < state->winner_count; ++i)
    {
        if (state->winners[i] == state->dealer)
            return 1;
    }

    return 0;
}

static uint8_t
cj4_settle_any_negative_score(const cj4_mahjong *state)
{
    for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
    {
        if (state->scores[i] < 0)
            return 1;
    }

    return 0;
}

static cj4_wind
cj4_settle_last_round_wind(const cj4_rules *rules)
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
cj4_settle_extension_last_round_wind(const cj4_rules *rules)
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
cj4_settle_is_endgame_round(const cj4_mahjong *state, cj4_wind last_round_wind)
{
    return state->round_wind > last_round_wind ||
           (state->round_wind == last_round_wind &&
            state->dealer == CJ4_PLAYER_3);
}

static void
cj4_settle_apply_tsumo(
    cj4_mahjong *next,
    const cj4_mahjong *state,
    const cj4_hand_score *score)
{
    int32_t honba_payment = state->honba * 100;
    cj4_player winner = state->winner;

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
        cj4_hand_score score;
        cj4_player winner = state->winners[i];
        int32_t total;

        assert(cj4_calculate_hand_score(state, winner, rules, &score));
        total = score.ron_points + honba_bonus;

        next->scores[state->loser] -= total;
        next->scores[winner] += total;
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
cj4_can_settle(const cj4_mahjong state)
{
    return state.phase == CJ4_PHASE_ROUND_END;
}

cj4_mahjong
cj4_do_settle(const cj4_mahjong state, const cj4_rules *rules)
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
            cj4_hand_score score;
            assert(cj4_calculate_hand_score(&state, state.winner, rules, &score));
            cj4_settle_apply_tsumo(&next, &state, &score);
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
        if (state.round_end_type != CJ4_ROUND_END_ABORTIVE_DRAW)
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
