#ifndef CJ4_STATE_QUERY_H
#define CJ4_STATE_QUERY_H

#include <stdbool.h>

#include "rules.h"
#include "state.h"

#ifdef __cplusplus
extern "C"
{
#endif

    static inline const cj4_location *
    cj4_tile_location_const(const cj4_mahjong *state, cj4_tile_id tile)
    {
        return &state->locations[tile];
    }

    static inline cj4_player
    cj4_next_player(const cj4_mahjong *state)
    {
        return (cj4_player)((state->current_player + 1) % CJ4_PLAYER_COUNT);
    }

    enum
    {
        CJ4_MAX_WIN_RESULT_YAKU = 32,
        CJ4_MAX_WIN_RESULT_DORA_INDICATORS = 5
    };

    typedef enum
    {
        CJ4_WIN_YAKU_RIICHI,
        CJ4_WIN_YAKU_DOUBLE_RIICHI,
        CJ4_WIN_YAKU_IPPATSU,
        CJ4_WIN_YAKU_MENZEN_TSUMO,
        CJ4_WIN_YAKU_TANYAO,
        CJ4_WIN_YAKU_YAKUHAI_HAKU,
        CJ4_WIN_YAKU_YAKUHAI_HATSU,
        CJ4_WIN_YAKU_YAKUHAI_CHUN,
        CJ4_WIN_YAKU_YAKUHAI_SEAT_WIND,
        CJ4_WIN_YAKU_YAKUHAI_ROUND_WIND,
        CJ4_WIN_YAKU_CHIITOI,
        CJ4_WIN_YAKU_KOKUSHI,
        CJ4_WIN_YAKU_KOKUSHI_13_WAIT,
        CJ4_WIN_YAKU_TOITOI,
        CJ4_WIN_YAKU_HONROUTOU,
        CJ4_WIN_YAKU_HONITSU,
        CJ4_WIN_YAKU_CHINITSU,
        CJ4_WIN_YAKU_PINFU,
        CJ4_WIN_YAKU_IIPEIKOU,
        CJ4_WIN_YAKU_RYANPEIKOU,
        CJ4_WIN_YAKU_SANSHOKU_DOUJUN,
        CJ4_WIN_YAKU_ITTSUU,
        CJ4_WIN_YAKU_CHANTA,
        CJ4_WIN_YAKU_JUNCHAN,
        CJ4_WIN_YAKU_SANANKOU,
        CJ4_WIN_YAKU_SHOUSANGEN,
        CJ4_WIN_YAKU_DAISANGEN,
        CJ4_WIN_YAKU_SHOUSUUSHII,
        CJ4_WIN_YAKU_DAISUUSHII,
        CJ4_WIN_YAKU_TSUUIISOU,
        CJ4_WIN_YAKU_RYUUIISOU,
        CJ4_WIN_YAKU_CHINROUTOU,
        CJ4_WIN_YAKU_SANKANTSU,
        CJ4_WIN_YAKU_SUUKANTSU,
        CJ4_WIN_YAKU_SANSHOKU_DOUKOU,
        CJ4_WIN_YAKU_SUUANKOU,
        CJ4_WIN_YAKU_SUUANKOU_TANKI,
        CJ4_WIN_YAKU_CHUUREN,
        CJ4_WIN_YAKU_JUNSEI_CHUUREN,
        CJ4_WIN_YAKU_RINSHAN,
        CJ4_WIN_YAKU_HAITEI,
        CJ4_WIN_YAKU_HOUTEI,
        CJ4_WIN_YAKU_CHANKAN,
        CJ4_WIN_YAKU_TENHOU,
        CJ4_WIN_YAKU_CHIIHOU
    } cj4_win_yaku;

    typedef struct
    {
        cj4_player player;
        uint8_t han;
        uint16_t fu;
        uint8_t yakuman_count;
        int32_t ron_points;
        int32_t tsumo_dealer_payment;
        int32_t tsumo_non_dealer_payment;
        uint8_t dora_count;
        uint8_t ura_dora_count;
        uint8_t aka_dora_count;
        cj4_tile_id dora_indicators[CJ4_MAX_WIN_RESULT_DORA_INDICATORS];
        uint8_t dora_indicators_count;
        cj4_tile_id ura_dora_indicators[CJ4_MAX_WIN_RESULT_DORA_INDICATORS];
        uint8_t ura_dora_indicators_count;
        cj4_win_yaku yaku[CJ4_MAX_WIN_RESULT_YAKU];
        uint8_t yaku_count;
    } cj4_win_result;

    uint8_t
    cj4_count_hand(
        const cj4_mahjong *state,
        cj4_player player,
        cj4_tile_type type);

    cj4_tile_id
    cj4_get_last_discard_tile(const cj4_mahjong *state);

    bool
    cj4_collect_winning_results(
        const cj4_mahjong *state,
        const cj4_rules *rules,
        cj4_win_result *out_results,
        uint8_t capacity,
        uint8_t *out_count);

#ifdef __cplusplus
}
#endif

#endif /* CJ4_STATE_QUERY_H */