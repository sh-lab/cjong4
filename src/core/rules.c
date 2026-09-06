#include "rules.h"

#include "tile_const.h"

#include <string.h>

static uint8_t
cj4_rules_is_bool(
    uint8_t value)
{
    return value == 0 || value == 1;
}

static uint8_t
cj4_rules_booleans_are_valid(
    const cj4_rules *rules)
{
    const uint8_t values[] = {
        rules->tobi_end,
        rules->kuitan,
        rules->kuikae_forbidden,
        rules->ippatsu,
        rules->kokushi_ron_on_ankan,
        rules->triple_ron_abortive_draw,
        rules->noten_penalty,
        rules->abortive_kyuushu_kyuuhai,
        rules->abortive_suufon_renda,
        rules->abortive_four_riichi,
        rules->nagashi_mangan,
        rules->kokushi_13_wait_double,
        rules->suuankou_tanki_double,
        rules->junsei_chuuren_double,
        rules->daisuushii_double,
        rules->kazoe_yakuman,
        rules->kiriage_mangan,
        rules->pao,
        rules->pao_liability_only,
        rules->pao_daisangen,
        rules->pao_daisuushii,
        rules->pao_suukantsu,
        rules->multi_ron_honba_first_only,
        rules->nagashi_dealer_tenpai_renchan,
        rules->target_score_excludes_riichi_sticks};

    for (uint8_t i = 0; i < (uint8_t)(sizeof(values) / sizeof(values[0])); ++i)
    {
        if (!cj4_rules_is_bool(values[i]))
            return 0;
    }

    return 1;
}

static void
cj4_rules_set_common_red_fives(
    cj4_rules *rules)
{
    rules->aka_tiles[CJ4_TILE_ID_5M_0] = 1;
    rules->aka_tiles[CJ4_TILE_ID_5P_0] = 1;
    rules->aka_tiles[CJ4_TILE_ID_5S_0] = 1;
}

cj4_rules
cj4_rules_default(
    void)
{
    cj4_rules rules;

    memset(&rules, 0, sizeof(rules));

    rules.version = CJ4_RULES_VERSION;

    rules.initial_score = 25000;
    rules.target_score = 30000;
    rules.game_type = CJ4_GAME_HANCHAN;
    rules.tobi_end = 1;

    rules.kuitan = 1;
    rules.kuikae_forbidden = 1;
    rules.kan_dora_timing = CJ4_KAN_DORA_EARLY;
    rules.four_kans_abort_timing = CJ4_FOUR_KANS_ABORT_AFTER_DISCARD;
    rules.ippatsu = 1;

    rules.max_ron_players = 3;
    rules.kokushi_ron_on_ankan = 1;
    rules.triple_ron_abortive_draw = 0;

    rules.noten_penalty = 1;
    rules.noten_penalty_points = 3000;
    rules.abortive_kyuushu_kyuuhai = 1;
    rules.abortive_suufon_renda = 1;
    rules.abortive_four_riichi = 1;
    rules.nagashi_mangan = 1;

    rules.kokushi_13_wait_double = 1;
    rules.suuankou_tanki_double = 1;
    rules.junsei_chuuren_double = 1;
    rules.daisuushii_double = 1;
    rules.kazoe_yakuman = 1;
    rules.kiriage_mangan = 1;

    rules.pao = 1;
    rules.pao_liability_only = 0;
    rules.pao_daisangen = 1;
    rules.pao_daisuushii = 1;
    rules.pao_suukantsu = 1;
    rules.multi_ron_honba_first_only = 0;
    rules.nagashi_dealer_tenpai_renchan = 0;
    rules.target_score_excludes_riichi_sticks = 0;

    cj4_rules_set_common_red_fives(&rules);

    return rules;
}

cj4_rules
cj4_rules_tenhou(
    void)
{
    cj4_rules rules = cj4_rules_default();

    rules.triple_ron_abortive_draw = 1;
    rules.pao_liability_only = 0;
    rules.kiriage_mangan = 0;
    rules.kokushi_ron_on_ankan = 0;
    rules.kokushi_13_wait_double = 0;
    rules.suuankou_tanki_double = 0;
    rules.junsei_chuuren_double = 0;
    rules.daisuushii_double = 0;
    rules.pao_daisangen = 1;
    rules.pao_daisuushii = 1;
    rules.pao_suukantsu = 0;
    rules.multi_ron_honba_first_only = 1;
    rules.nagashi_dealer_tenpai_renchan = 1;
    rules.target_score_excludes_riichi_sticks = 1;
    rules.kan_dora_timing = CJ4_KAN_DORA_LATE;

    return rules;
}

cj4_rules
cj4_rules_mjsoul(
    void)
{
    cj4_rules rules = cj4_rules_default();

    rules.triple_ron_abortive_draw = 0;
    rules.pao_liability_only = 0;
    rules.kan_dora_timing = CJ4_KAN_DORA_LATE;

    return rules;
}

bool
cj4_rules_validate(
    const cj4_rules *rules)
{
    if (!rules)
        return false;

    if (rules->version != 0 && rules->version != CJ4_RULES_VERSION)
        return false;

    if (rules->initial_score <= 0 || rules->target_score <= 0)
        return false;

    if (rules->game_type != CJ4_GAME_TONPUU &&
        rules->game_type != CJ4_GAME_HANCHAN &&
        rules->game_type != CJ4_GAME_FULL)
    {
        return false;
    }

    if (rules->kan_dora_timing != CJ4_KAN_DORA_EARLY &&
        rules->kan_dora_timing != CJ4_KAN_DORA_LATE)
    {
        return false;
    }

    if (rules->four_kans_abort_timing != CJ4_FOUR_KANS_ABORT_IMMEDIATE &&
        rules->four_kans_abort_timing != CJ4_FOUR_KANS_ABORT_AFTER_DISCARD)
    {
        return false;
    }

    if (rules->max_ron_players < 1 || rules->max_ron_players > 3)
        return false;

    if (!cj4_rules_booleans_are_valid(rules))
        return false;

    if (rules->noten_penalty)
    {
        if (rules->noten_penalty_points <= 0)
            return false;

        if (rules->noten_penalty_points % 100 != 0)
            return false;
    }
    else if (rules->noten_penalty_points < 0)
    {
        return false;
    }

    for (uint16_t tile = 0; tile < CJ4_TILE_ID_COUNT; ++tile)
    {
        if (!cj4_rules_is_bool(rules->aka_tiles[tile]))
            return false;
    }

    return true;
}
