#ifndef CJ4_RULES_H
#define CJ4_RULES_H

#include "tile.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define CJ4_RULES_VERSION 1u

    typedef enum
    {
        CJ4_GAME_TONPUU,
        CJ4_GAME_HANCHAN,
        CJ4_GAME_FULL
    } cj4_game_type;

    typedef struct
    {
        uint32_t version;

        /* score / structure */
        int32_t initial_score;
        int32_t target_score;
        cj4_game_type game_type;
        uint8_t tobi_end;

        /* general */
        uint8_t kuitan;
        uint8_t kuikae_forbidden;

        /* riichi */
        uint8_t ippatsu;

        /* ron */
        uint8_t max_ron_players; /* 1=head bump, 2=double ron, 3=triple ron */
        uint8_t kokushi_ron_on_ankan;
        uint8_t triple_ron_abortive_draw;

        /* draw */
        uint8_t noten_penalty;
        int32_t noten_penalty_points;
        uint8_t abortive_kyuushu_kyuuhai;
        uint8_t abortive_suufon_renda;
        uint8_t abortive_four_riichi;
        uint8_t nagashi_mangan;

        /* scoring */
        uint8_t kokushi_13_wait_double;
        uint8_t suuankou_tanki_double;
        uint8_t junsei_chuuren_double;
        uint8_t daisuushii_double;
        uint8_t kazoe_yakuman;
        uint8_t kiriage_mangan;

        /* settlement */
        uint8_t pao;
        uint8_t pao_liability_only;
        uint8_t pao_daisangen;
        uint8_t pao_daisuushii;
        uint8_t pao_suukantsu;
        uint8_t multi_ron_honba_first_only;
        uint8_t nagashi_dealer_tenpai_renchan;
        uint8_t target_score_excludes_riichi_sticks;

        /* red tiles */
        uint8_t aka_tiles[136];
    } cj4_rules;

    cj4_rules
    cj4_rules_default(void);

    cj4_rules
    cj4_rules_tenhou(void);

    cj4_rules
    cj4_rules_mjsoul(void);

    bool
    cj4_rules_validate(const cj4_rules *rules);

#ifdef __cplusplus
}
#endif

#endif /* CJ4_RULES_H */
