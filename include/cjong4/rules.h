#ifndef CJ4_RULES_H
#define CJ4_RULES_H

#include "tile.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        CJ4_GAME_TONPUU,
        CJ4_GAME_HANCHAN,
        CJ4_GAME_FULL
    } cj4_game_type;

    typedef struct
    {
        /* score / structure */
        int32_t initial_score;
        int32_t target_score;
        cj4_game_type game_type;
        uint8_t tobi_end;

        /* general */
        uint8_t kuitan;

        /* riichi */
        uint8_t ippatsu;

        /* ron */
        uint8_t max_ron_players; /* 1=head bump, 2=double ron, 3=triple ron */

        /* draw / abort */
        uint8_t noten_penalty;
        int32_t noten_penalty_points;

        uint8_t abort_kyushukyuhai;
        uint8_t abort_sufurenta;

        /* red tiles */
        uint8_t aka_tiles[136];
    } cj4_rules;

#ifdef __cplusplus
}
#endif

#endif /* CJ4_RULES_H */