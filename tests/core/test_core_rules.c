#include <assert.h>
#include <string.h>

#include "../../src/core/hand_check.h"
#include "../../src/core/state_ops.h"
#include "../../src/core/state_score.h"
#include "cjong4/core/rules.h"
#include "cjong4/core/state.h"
#include "cjong4/core/state_abortive.h"
#include "cjong4/core/state_chi.h"
#include "cjong4/core/state_discard.h"
#include "cjong4/core/state_init.h"
#include "cjong4/core/state_kan.h"
#include "cjong4/core/state_pass.h"
#include "cjong4/core/state_pon.h"
#include "cjong4/core/state_query.h"
#include "cjong4/core/state_riichi.h"
#include "cjong4/core/state_ron.h"
#include "cjong4/core/state_round.h"
#include "cjong4/core/state_settle.h"
#include "cjong4/core/state_tsumo.h"

int
manager_tests_main(void);

static cj4_tile_id
tile(
    cj4_tile_type type,
    uint8_t index)
{
    return cj4_tile_make(type, index);
}

static cj4_mahjong
make_empty_state(
    void)
{
    cj4_mahjong state;

    memset(&state, 0, sizeof(state));
    memset(state.locations, CJ4_LOCATION_NONE, sizeof(state.locations));
    for (uint16_t i = 0; i < CJ4_TILE_ID_COUNT; ++i)
        state.locations[i].wall = (uint8_t)i;
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    state.dealer = CJ4_PLAYER_0;
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.round_wind = CJ4_WIND_EAST;
    state.draw_tile = CJ4_TILE_ID_INVALID;
    state.pending_kakan_tile = CJ4_TILE_ID_INVALID;
    state.pending_ankan_tile = CJ4_TILE_ID_INVALID;
    for (uint8_t i = 0; i < 4; ++i)
        state.pending_ankan_tiles[i] = CJ4_TILE_ID_INVALID;
    cj4_state_clear_pending_riichi_bits(&state);
    state.last_discard_tile = CJ4_TILE_ID_INVALID;
    state.winning_tile = CJ4_TILE_ID_INVALID;
    cj4_state_set_round_result(&state, CJ4_ROUND_END_NONE, CJ4_ABORTIVE_DRAW_NONE);

    for (uint8_t i = 0; i < CJ4_PLAYER_COUNT; ++i)
        state.scores[i] = 25000;

    return state;
}

static void
set_hand(
    cj4_mahjong *state,
    cj4_player player,
    const cj4_tile_id *tiles,
    uint8_t count)
{
    for (uint8_t i = 0; i < count; ++i)
    {
        state->locations[tiles[i]].placement = cj4_location_make_hand(player);
    }
}

static void
set_test_wall(
    cj4_mahjong *state,
    uint8_t position,
    cj4_tile_id tile)
{
    for (uint16_t i = 0; i < CJ4_TILE_ID_COUNT; ++i)
        if (state->locations[i].wall == position)
            state->locations[i].wall = CJ4_LOCATION_NONE;
    state->locations[tile].wall = position;
}

static void
add_discard(
    cj4_mahjong *state,
    cj4_player player,
    cj4_tile_id discarded)
{
    cj4_player current = cj4_state_current_player(state);
    cj4_state_set_current_player(state, player);
    cj4_state_record_discard(state, discarded, 0, 0);
    cj4_state_set_current_player(state, current);
}

static void
set_test_meld(
    cj4_mahjong *state,
    cj4_player player,
    uint8_t group,
    const cj4_meld *meld)
{
    for (uint8_t i = 0; i < meld->size; ++i)
    {
        cj4_tile_id id = meld->tiles[i];
        state->locations[id].placement =
            cj4_location_make_meld(player, group, meld->type);
        if (i == meld->called_index && meld->from_player != player &&
            state->locations[id].discard == CJ4_LOCATION_NONE)
        {
            state->locations[id].discard =
                cj4_location_make_discard(meld->from_player, 0, false);
            state->locations[id].discard_history =
                cj4_location_make_discard_history(state->discard_count++, false);
        }
    }
}

static cj4_meld
get_test_meld(
    const cj4_mahjong *state,
    cj4_player player,
    uint8_t group)
{
    cj4_meld meld;
    assert(cj4_get_meld(state, player, group, &meld));
    return meld;
}

static void
set_test_kan(
    cj4_mahjong *state,
    cj4_player player,
    uint8_t group,
    cj4_meld_type type,
    cj4_tile_id first)
{
    set_test_meld(state, player, group, &(cj4_meld){.tiles = {first, (cj4_tile_id)(first + 1), (cj4_tile_id)(first + 2), (cj4_tile_id)(first + 3)}, .size = 4, .type = type, .from_player = player, .called_index = CJ4_CALLED_INDEX_NONE});
}

static uint8_t
contains_win_yaku(
    const cj4_win_result *result,
    cj4_win_yaku yaku)
{
    for (uint8_t i = 0; i < result->yaku_count; ++i)
    {
        if (result->yaku[i] == yaku)
            return 1;
    }

    return 0;
}

static void
test_v2_packed_location_and_state_layout(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_discard discards[CJ4_MAX_DISCARDS];
    cj4_location unknown;

    assert(sizeof(cj4_location) == 4);
    assert(sizeof(cj4_mahjong) >= sizeof(cj4_location) * CJ4_TILE_ID_COUNT);
    assert(sizeof(cj4_mahjong) <= 700);
    assert(cj4_tile_id_is_valid(CJ4_TILE_ID_MIN));
    assert(cj4_tile_id_is_valid(CJ4_TILE_ID_MAX));
    assert(!cj4_tile_id_is_valid(136));
    assert(!cj4_tile_id_is_valid(CJ4_TILE_ID_INVALID));

    memset(&unknown, CJ4_LOCATION_NONE, sizeof(unknown));
    assert(cj4_location_is_unknown(&unknown));
    assert(!cj4_location_is_unknown(NULL));

    assert(cj4_location_make_discard(CJ4_PLAYER_3, 30, true) == 0xfe);
    assert(cj4_location_is_discard(0x00));
    assert(cj4_location_is_discard(0x1e));
    assert(cj4_location_is_discard(0x80));
    assert(cj4_location_is_discard(0xfe));
    assert(!cj4_location_is_discard(0x1f));
    assert(!cj4_location_is_discard(0x7f));
    assert(!cj4_location_is_discard(CJ4_LOCATION_NONE));
    assert(cj4_location_discard_player(CJ4_LOCATION_NONE) == CJ4_PLAYER_COUNT);
    assert(cj4_location_discard_index(CJ4_LOCATION_NONE) == 31);
    assert(!cj4_location_discard_is_tsumogiri(CJ4_LOCATION_NONE));
    assert(cj4_location_make_hand(CJ4_PLAYER_3) == 0x60);
    assert(cj4_location_make_meld(
               CJ4_PLAYER_3,
               3,
               CJ4_MELD_KAKAN) == 0xfc);
    assert(cj4_location_make_discard_history(85, true) == 0xd5);
    assert(cj4_location_is_hand(0x00));
    assert(cj4_location_is_hand(0x60));
    assert(!cj4_location_is_hand(0x01));
    assert(!cj4_location_is_hand(0x7f));
    assert(!cj4_location_is_hand(CJ4_LOCATION_NONE));
    assert(cj4_location_placement_player(0x01) == CJ4_PLAYER_COUNT);
    assert(cj4_location_is_meld(0x80));
    assert(cj4_location_is_meld(0xfc));
    assert(!cj4_location_is_meld(0x85));
    assert(!cj4_location_is_meld(0x86));
    assert(!cj4_location_is_meld(0x87));
    assert(!cj4_location_is_meld(CJ4_LOCATION_NONE));
    assert(cj4_location_meld_group(CJ4_LOCATION_NONE) == 4);
    assert(cj4_location_meld_type(CJ4_LOCATION_NONE) == CJ4_MELD_INVALID);
    assert(cj4_location_is_discard_history(0x00));
    assert(cj4_location_is_discard_history(0xd5));
    assert(!cj4_location_is_discard_history(0x56));
    assert(!cj4_location_is_discard_history(CJ4_LOCATION_NONE));
    assert(cj4_location_discard_history_index(CJ4_LOCATION_NONE) == 86);
    assert(!cj4_location_discard_is_riichi(CJ4_LOCATION_NONE));
    assert(cj4_get_wall_tile(&state, CJ4_LOCATION_NONE) ==
           CJ4_TILE_ID_INVALID);

    state.locations[0].discard = 0x1f;
    state.locations[0].discard_history =
        cj4_location_make_discard_history(0, false);
    state.discard_count = 1;
    assert(cj4_location_collect_discards(&state, discards) == 0);
    assert(cj4_location_collect_discards(&state, discards) == 0);

    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_2);
    cj4_state_set_first_turn(&state, true);
    cj4_state_set_chankan(&state, true);
    assert(state.progress == 0xe4);

    cj4_state_set_riichi(&state, CJ4_PLAYER_1, true);
    cj4_state_set_ippatsu(&state, CJ4_PLAYER_3, true);
    assert(state.riichi_ippatsu == 0x82);
    cj4_state_set_temporary_furiten(&state, CJ4_PLAYER_0, true);
    cj4_state_set_riichi_furiten(&state, CJ4_PLAYER_2, true);
    assert(state.furiten == 0x41);
    cj4_state_set_draw_turn(&state, CJ4_PLAYER_0, 1);
    cj4_state_set_draw_turn(&state, CJ4_PLAYER_1, 2);
    assert(state.draw_turns == 0x09);

    cj4_state_set_pao(
        &state,
        CJ4_PLAYER_2,
        CJ4_PLAYER_1,
        CJ4_PAO_DAISUUSHII);
    assert(state.pao[1] == 0x09);
    cj4_state_set_round_result(
        &state,
        CJ4_ROUND_END_ABORTIVE_DRAW,
        CJ4_ABORTIVE_DRAW_FOUR_KANS);
    assert(state.round_result == 0x24);
}

static void
test_v2_location_reconstruction_and_player_mask(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_tile_id own = tile(0, 0);
    cj4_tile_id hidden = tile(1, 0);
    cj4_tile_id called = tile(2, 0);
    cj4_tile_id hand1 = tile(2, 1);
    cj4_tile_id hand2 = tile(2, 2);
    cj4_tile_id dora = tile(3, 0);
    cj4_mahjong masked;
    cj4_meld meld;
    cj4_discard discards[CJ4_MAX_DISCARDS];
    cj4_tile_id hand[CJ4_MAX_HAND_TILES];
    cj4_meld melds[CJ4_MAX_MELDS];

    set_hand(&state, CJ4_PLAYER_0, &own, 1);
    set_hand(&state, CJ4_PLAYER_1, &hidden, 1);
    add_discard(&state, CJ4_PLAYER_3, called);
    set_hand(&state, CJ4_PLAYER_2, &hand1, 1);
    set_hand(&state, CJ4_PLAYER_2, &hand2, 1);
    set_test_wall(&state, 130, dora);
    state.dora_count = 1;
    set_test_meld(&state, CJ4_PLAYER_2, 0, &(cj4_meld){.tiles = {called, hand1, hand2}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_3, .called_index = 0});

    assert(cj4_get_meld(&state, CJ4_PLAYER_2, 0, &meld));
    assert(meld.type == CJ4_MELD_PON);
    assert(meld.size == 3);
    assert(meld.from_player == CJ4_PLAYER_3);
    assert(meld.called_index < meld.size);
    assert(meld.tiles[meld.called_index] == called);
    assert(cj4_location_collect_hand(&state, CJ4_PLAYER_0, hand) == 1);
    assert(hand[0] == own);
    assert(cj4_location_collect_discards(&state, discards) == 1);
    assert(discards[0].tile == called);
    assert(!discards[0].is_active);
    assert(cj4_location_collect_melds(&state, CJ4_PLAYER_2, melds) == 1);
    assert(melds[0].type == CJ4_MELD_PON);

    masked = cj4_make_player_state(&state, CJ4_PLAYER_0);
    assert(masked.locations[own].wall != CJ4_LOCATION_NONE);
    assert(masked.locations[hidden].wall == CJ4_LOCATION_NONE);
    assert(masked.locations[hidden].discard == CJ4_LOCATION_NONE);
    assert(masked.locations[called].wall != CJ4_LOCATION_NONE);
    assert(masked.locations[called].discard != CJ4_LOCATION_NONE);
    assert(cj4_location_is_meld(masked.locations[called].placement));
    assert(masked.locations[dora].wall == 130);

    state.draw_tile = 200;
    masked = cj4_make_player_state(&state, CJ4_PLAYER_0);
    assert(masked.draw_tile == CJ4_TILE_ID_INVALID);
}

static void
test_v2_initial_state_uses_locations_as_canonical_wall(
    void)
{
    cj4_tile_id wall[CJ4_TILE_ID_COUNT];
    cj4_rules rules = cj4_rules_default();
    cj4_mahjong unchanged;
    uint8_t hand_count = 0;

    for (uint16_t i = 0; i < CJ4_TILE_ID_COUNT; ++i)
        wall[i] = (cj4_tile_id)i;

    assert(cj4_wall_is_valid(wall));
    cj4_mahjong state = cj4_create_initial_state(wall, &rules);
    for (uint16_t i = 0; i < CJ4_TILE_ID_COUNT; ++i)
    {
        assert(state.locations[i].wall == i);
        if (cj4_location_is_hand(state.locations[i].placement))
            hand_count++;
    }

    assert(hand_count == 53);
    assert(state.wall_pos == 53);
    assert(state.draw_tile != CJ4_TILE_ID_INVALID);
    assert(state.last_discard_tile == CJ4_TILE_ID_INVALID);
    assert(state.discard_count == 0);
    assert(state.dora_count == 1);

    wall[0] = wall[1];
    assert(!cj4_wall_is_valid(wall));
    state = cj4_create_initial_state(wall, &rules);
    assert(cj4_state_phase(&state) == CJ4_PHASE_GAME_END);
    assert(state.draw_tile == CJ4_TILE_ID_INVALID);

    wall[0] = CJ4_TILE_ID_INVALID;
    assert(!cj4_wall_is_valid(wall));
    assert(!cj4_wall_is_valid(NULL));

    state = make_empty_state();
    cj4_state_set_phase(&state, CJ4_PHASE_SETTLE);
    state.settlement_should_end = 0;
    unchanged = cj4_do_next_round(state, wall, &rules);
    assert(memcmp(&state, &unchanged, sizeof(state)) == 0);
}

static void
test_v2_rejects_out_of_range_winning_tile(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_win_result result;
    uint8_t count = 1;

    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    cj4_state_set_round_result(&state, CJ4_ROUND_END_RON, CJ4_ABORTIVE_DRAW_NONE);
    state.winner_mask = 1;
    state.winning_tile = 200;

    assert(!cj4_collect_winning_results(&state, NULL, &result, 1, &count));
    assert(count == 0);
}

static void
test_riichi_uses_shape_tenpai(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_tile_id draw = tile(26, 0);
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(3, 0),
        tile(4, 0),
        tile(15, 0),
        tile(15, 1),
        draw};

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    state.draw_tile = draw;

    assert(cj4_can_riichi(&state, draw));
}

static void
test_riichi_requires_four_live_wall_tiles(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_tile_id draw = tile(26, 0);
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(3, 0),
        tile(4, 0),
        tile(15, 0),
        tile(15, 1),
        draw};

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    state.draw_tile = draw;

    state.wall_pos = 118;
    assert(cj4_can_riichi(&state, draw));

    state.wall_pos = 119;
    assert(!cj4_can_riichi(&state, draw));
}

static void
test_rules_default_and_validate(
    void)
{
    cj4_rules rules = cj4_rules_default();

    assert(cj4_rules_validate(&rules));
    assert(rules.initial_score == 25000);
    assert(rules.target_score == 30000);
    assert(rules.max_ron_players == 3);
    assert(rules.kuikae_forbidden == 1);
    assert(rules.kokushi_ron_on_ankan == 1);
    assert(rules.kazoe_yakuman == 1);
    assert(rules.kiriage_mangan == 1);
    assert(rules.multi_ron_honba_first_only == 0);
    assert(rules.nagashi_dealer_tenpai_renchan == 0);
    assert(rules.target_score_excludes_riichi_sticks == 0);

    rules.target_score_excludes_riichi_sticks = 2;
    assert(!cj4_rules_validate(&rules));
    rules.target_score_excludes_riichi_sticks = 0;

    rules.max_ron_players = 0;
    assert(!cj4_rules_validate(&rules));
}

static void
test_tenhou_preset_fields(
    void)
{
    cj4_rules rules = cj4_rules_tenhou();

    assert(cj4_rules_validate(&rules));
    assert(rules.version == CJ4_RULES_VERSION);
    assert(rules.triple_ron_abortive_draw == 1);
    assert(rules.pao_liability_only == 0);
    assert(rules.kiriage_mangan == 0);
    assert(rules.kokushi_ron_on_ankan == 0);
    assert(rules.kokushi_13_wait_double == 0);
    assert(rules.suuankou_tanki_double == 0);
    assert(rules.junsei_chuuren_double == 0);
    assert(rules.daisuushii_double == 0);
    assert(rules.pao == 1);
    assert(rules.pao_daisangen == 1);
    assert(rules.pao_daisuushii == 1);
    assert(rules.pao_suukantsu == 0);
    assert(rules.multi_ron_honba_first_only == 1);
    assert(rules.nagashi_dealer_tenpai_renchan == 1);
    assert(rules.target_score_excludes_riichi_sticks == 1);
}

static void
test_riichi_establishes_after_pass(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_mahjong declared;
    cj4_mahjong established;
    cj4_tile_id draw = tile(26, 0);
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(3, 0),
        tile(4, 0),
        tile(15, 0),
        tile(15, 1),
        draw};

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    state.draw_tile = draw;
    cj4_state_set_first_turn(&state, 1);
    cj4_state_set_draw_turn(&state, CJ4_PLAYER_0, 1);
    set_test_wall(&state, (uint8_t)(0), tile(30, 0));

    declared = cj4_do_riichi(state, draw);

    assert(cj4_state_has_pending_riichi(&declared));
    assert(cj4_state_is_riichi(&declared, CJ4_PLAYER_0) == 0);
    assert(declared.scores[CJ4_PLAYER_0] == 25000);
    assert(declared.riichi_sticks == 0);

    established = cj4_do_pass(declared, NULL);

    assert(!cj4_state_has_pending_riichi(&established));
    assert(cj4_state_is_riichi(&established, CJ4_PLAYER_0) == 1);
    assert(cj4_state_is_ippatsu(&established, CJ4_PLAYER_0) == 1);
    assert(cj4_state_double_riichi(&established, CJ4_PLAYER_0) == 1);
    assert(established.scores[CJ4_PLAYER_0] == 24000);
    assert(established.riichi_sticks == 1);
    assert(!cj4_can_discard(established, tile(0, 0)));
}

static void
test_riichi_ron_clears_pending_without_payment(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_mahjong declared;
    cj4_mahjong ron;
    cj4_player winner = CJ4_PLAYER_1;
    cj4_tile_id draw = tile(26, 0);
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(3, 0),
        tile(4, 0),
        tile(15, 0),
        tile(15, 1),
        draw};

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    state.draw_tile = draw;

    declared = cj4_do_riichi(state, draw);
    ron = cj4_do_ron_multi(declared, &winner, 1, NULL);

    assert(cj4_state_phase(&ron) == CJ4_PHASE_ROUND_END);
    assert(!cj4_state_has_pending_riichi(&ron));
    assert(cj4_state_is_riichi(&ron, CJ4_PLAYER_0) == 0);
    assert(ron.scores[CJ4_PLAYER_0] == 25000);
    assert(ron.riichi_sticks == 0);
}

static void
test_riichi_called_discard_still_establishes(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_mahjong declared;
    cj4_mahjong called;
    cj4_tile_id draw = tile(26, 0);
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(3, 0),
        tile(4, 0),
        tile(15, 0),
        tile(15, 1),
        draw};
    const cj4_tile_id pon_tiles[] = {tile(26, 1), tile(26, 2)};

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    set_hand(&state, CJ4_PLAYER_2, pon_tiles, 2);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    state.draw_tile = draw;

    declared = cj4_do_riichi(state, draw);
    called = cj4_do_pon(declared, CJ4_PLAYER_2, pon_tiles[0], pon_tiles[1]);

    assert(cj4_state_is_riichi(&called, CJ4_PLAYER_0) == 1);
    assert(!cj4_state_has_pending_riichi(&called));
    assert(called.scores[CJ4_PLAYER_0] == 24000);
    assert(called.riichi_sticks == 1);
    assert(cj4_state_is_ippatsu(&called, CJ4_PLAYER_0) == 0);
}

static void
test_temporary_furiten_blocks_ron(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong after_pass;
    cj4_mahjong later;
    const cj4_tile_id hand[] = {
        tile(1, 0),
        tile(2, 0),
        tile(3, 0),
        tile(10, 0),
        tile(11, 0),
        tile(12, 0),
        tile(19, 0),
        tile(20, 0),
        tile(21, 0),
        tile(4, 0),
        tile(5, 0),
        tile(14, 0),
        tile(14, 1)};

    rules.kuitan = 1;

    set_hand(&state, CJ4_PLAYER_2, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    add_discard(&state, CJ4_PLAYER_0, tile(6, 0));

    assert(cj4_can_ron(&state, CJ4_PLAYER_2, &rules));

    after_pass = cj4_do_pass(state, &rules);
    assert(cj4_state_temporary_furiten(&after_pass, CJ4_PLAYER_2) == 1);
    assert(cj4_state_riichi_furiten(&after_pass, CJ4_PLAYER_2) == 0);

    later = after_pass;
    cj4_state_set_phase(&later, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&later, CJ4_PLAYER_1);
    later.draw_tile = CJ4_TILE_ID_INVALID;
    add_discard(&later, CJ4_PLAYER_1, tile(3, 1));

    assert(!cj4_can_ron(&later, CJ4_PLAYER_2, &rules));
}

static void
test_riichi_furiten_is_recorded(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong after_pass;
    cj4_mahjong later;
    const cj4_tile_id hand[] = {
        tile(1, 0),
        tile(2, 0),
        tile(3, 0),
        tile(10, 0),
        tile(11, 0),
        tile(12, 0),
        tile(19, 0),
        tile(20, 0),
        tile(21, 0),
        tile(4, 0),
        tile(5, 0),
        tile(14, 0),
        tile(14, 1)};

    rules.kuitan = 1;

    set_hand(&state, CJ4_PLAYER_2, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    cj4_state_set_riichi(&state, CJ4_PLAYER_2, 1);
    add_discard(&state, CJ4_PLAYER_0, tile(6, 0));

    assert(cj4_can_ron(&state, CJ4_PLAYER_2, &rules));

    after_pass = cj4_do_pass(state, &rules);
    assert(cj4_state_temporary_furiten(&after_pass, CJ4_PLAYER_2) == 0);
    assert(cj4_state_riichi_furiten(&after_pass, CJ4_PLAYER_2) == 1);

    later = after_pass;
    cj4_state_set_phase(&later, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&later, CJ4_PLAYER_1);
    later.draw_tile = CJ4_TILE_ID_INVALID;
    add_discard(&later, CJ4_PLAYER_1, tile(3, 1));

    assert(!cj4_can_ron(&later, CJ4_PLAYER_2, &rules));
}

static void
test_permanent_furiten_blocks_other_wait(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    const cj4_tile_id hand[] = {
        tile(1, 0),
        tile(2, 0),
        tile(3, 0),
        tile(10, 0),
        tile(11, 0),
        tile(12, 0),
        tile(19, 0),
        tile(20, 0),
        tile(21, 0),
        tile(4, 0),
        tile(5, 0),
        tile(14, 0),
        tile(14, 1)};

    rules.kuitan = 1;

    set_hand(&state, CJ4_PLAYER_2, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    add_discard(&state, CJ4_PLAYER_2, tile(6, 0));
    add_discard(&state, CJ4_PLAYER_0, tile(3, 1));

    assert(!cj4_can_ron(&state, CJ4_PLAYER_2, &rules));
}

static void
test_kan_flow_aborts_on_fourth_kakan_without_noten_penalty(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong round_end;
    cj4_mahjong settled;

    rules.noten_penalty = 1;
    rules.noten_penalty_points = 3000;

    cj4_state_set_phase(&state, CJ4_PHASE_ANKAN_RESOLVE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.dead_wall_draw_count = 3;
    state.honba = 0;

    set_test_kan(&state, CJ4_PLAYER_0, 0, CJ4_MELD_ANKAN, tile(0, 0));
    set_test_kan(&state, CJ4_PLAYER_0, 1, CJ4_MELD_KAKAN, tile(1, 0));
    set_test_kan(&state, CJ4_PLAYER_1, 0, CJ4_MELD_MINKAN, tile(2, 0));
    set_test_kan(&state, CJ4_PLAYER_1, 1, CJ4_MELD_ANKAN, tile(3, 0));

    round_end = cj4_do_rinshan_draw(state, &rules);
    assert(cj4_state_phase(&round_end) == CJ4_PHASE_ROUND_END);
    assert(cj4_state_round_end_type(&round_end) == CJ4_ROUND_END_ABORTIVE_DRAW);
    assert(round_end.dead_wall_draw_count == 3);

    settled = cj4_do_settle(round_end, &rules);
    assert(cj4_state_phase(&settled) == CJ4_PHASE_SETTLE);
    assert(settled.scores[0] == 25000);
    assert(settled.scores[1] == 25000);
    assert(settled.scores[2] == 25000);
    assert(settled.scores[3] == 25000);
    assert(settled.next_dealer == CJ4_PLAYER_0);
    assert(settled.honba == 1);
}

static void
test_riichi_restricts_actions(
    void)
{
    cj4_mahjong discard_state = make_empty_state();
    cj4_mahjong chi_state = make_empty_state();
    cj4_mahjong pon_state = make_empty_state();
    cj4_mahjong minkan_state = make_empty_state();
    cj4_mahjong kakan_state = make_empty_state();
    cj4_tile_id draw = tile(8, 0);
    const cj4_tile_id discard_hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(3, 0),
        tile(4, 0),
        tile(15, 0),
        tile(15, 1),
        draw};
    const cj4_tile_id chi_hand[] = {
        tile(1, 0),
        tile(2, 0)};
    const cj4_tile_id pon_hand[] = {
        tile(3, 1),
        tile(3, 2)};
    const cj4_tile_id minkan_hand[] = {
        tile(3, 1),
        tile(3, 2),
        tile(3, 3)};
    const cj4_tile_id kakan_hand[] = {
        tile(4, 3),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(12, 0),
        tile(13, 0),
        tile(14, 0),
        tile(21, 0),
        tile(22, 0),
        tile(23, 0),
        tile(24, 0)};

    set_hand(&discard_state, CJ4_PLAYER_0, discard_hand, (uint8_t)(sizeof(discard_hand) / sizeof(discard_hand[0])));
    cj4_state_set_phase(&discard_state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&discard_state, CJ4_PLAYER_0);
    discard_state.draw_tile = draw;
    cj4_state_set_riichi(&discard_state, CJ4_PLAYER_0, 1);

    assert(cj4_can_discard(discard_state, draw));
    assert(!cj4_can_discard(discard_state, tile(0, 0)));

    set_hand(&chi_state, CJ4_PLAYER_1, chi_hand, (uint8_t)(sizeof(chi_hand) / sizeof(chi_hand[0])));
    cj4_state_set_phase(&chi_state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&chi_state, CJ4_PLAYER_0);
    cj4_state_set_riichi(&chi_state, CJ4_PLAYER_1, 1);
    add_discard(&chi_state, CJ4_PLAYER_0, tile(3, 0));

    assert(!cj4_can_chi(&chi_state));
    assert(!cj4_can_chi_with_tile(&chi_state, chi_hand[0], chi_hand[1]));

    set_hand(&pon_state, CJ4_PLAYER_2, pon_hand, (uint8_t)(sizeof(pon_hand) / sizeof(pon_hand[0])));
    cj4_state_set_phase(&pon_state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&pon_state, CJ4_PLAYER_0);
    cj4_state_set_riichi(&pon_state, CJ4_PLAYER_2, 1);
    add_discard(&pon_state, CJ4_PLAYER_0, tile(3, 0));

    assert(!cj4_can_pon(&pon_state, CJ4_PLAYER_2));
    assert(!cj4_can_pon_with_tile(&pon_state, CJ4_PLAYER_2, pon_hand[0], pon_hand[1]));

    set_hand(&minkan_state, CJ4_PLAYER_2, minkan_hand, (uint8_t)(sizeof(minkan_hand) / sizeof(minkan_hand[0])));
    cj4_state_set_phase(&minkan_state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&minkan_state, CJ4_PLAYER_0);
    cj4_state_set_riichi(&minkan_state, CJ4_PLAYER_2, 1);
    add_discard(&minkan_state, CJ4_PLAYER_0, tile(3, 0));

    assert(!cj4_can_minkan(&minkan_state, CJ4_PLAYER_2));
    assert(!cj4_can_minkan_with_tile(&minkan_state, CJ4_PLAYER_2, minkan_hand[0], minkan_hand[1], minkan_hand[2]));

    set_hand(&kakan_state, CJ4_PLAYER_0, kakan_hand, (uint8_t)(sizeof(kakan_hand) / sizeof(kakan_hand[0])));
    cj4_state_set_phase(&kakan_state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&kakan_state, CJ4_PLAYER_0);
    cj4_state_set_riichi(&kakan_state, CJ4_PLAYER_0, 1);
    kakan_state.draw_tile = kakan_hand[0];
    set_test_meld(&kakan_state, CJ4_PLAYER_0, 0, &(cj4_meld){.tiles = {tile(4, 0), tile(4, 1), tile(4, 2)}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_1, .called_index = 0});

    assert(!cj4_can_kakan(&kakan_state));
    assert(!cj4_can_kakan_with_tile(&kakan_state, kakan_hand[0]));
}

static void
test_seven_pairs_rejects_quad(
    void)
{
    cj4_mahjong state = make_empty_state();
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(0, 1),
        tile(0, 2),
        tile(0, 3),
        tile(9, 0),
        tile(9, 1),
        tile(10, 0),
        tile(10, 1),
        tile(18, 0),
        tile(18, 1),
        tile(19, 0),
        tile(19, 1),
        tile(27, 0),
        tile(27, 1)};

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));

    assert(!cj4_is_complete_hand(&state, CJ4_PLAYER_0));
}

static void
test_chankan_pass_records_furiten(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong next;
    const cj4_tile_id meld_tiles[] = {
        tile(1, 0),
        tile(2, 0),
        tile(3, 0)};
    const cj4_tile_id hand[] = {
        tile(11, 0),
        tile(12, 0),
        tile(13, 0),
        tile(21, 0),
        tile(22, 0),
        tile(23, 0),
        tile(5, 0),
        tile(6, 0),
        tile(16, 0),
        tile(16, 1)};

    rules.kuitan = 1;

    set_hand(&state, CJ4_PLAYER_2, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    for (uint8_t i = 0; i < 3; ++i)
    {
        state.locations[meld_tiles[i]].placement =
            cj4_location_make_meld(CJ4_PLAYER_2, 0, CJ4_MELD_CHI);
    }

    cj4_state_set_phase(&state, CJ4_PHASE_KAKAN_RESOLVE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.pending_kakan_tile = tile(4, 0);
    set_test_meld(&state, CJ4_PLAYER_2, 0, &(cj4_meld){.tiles = {meld_tiles[0], meld_tiles[1], meld_tiles[2]}, .size = 3, .type = CJ4_MELD_CHI, .from_player = CJ4_PLAYER_1, .called_index = 0});

    assert(cj4_can_ron(&state, CJ4_PLAYER_2, &rules));

    next = cj4_do_rinshan_draw(state, &rules);

    assert(cj4_state_temporary_furiten(&next, CJ4_PLAYER_2) == 1);
    assert(cj4_state_riichi_furiten(&next, CJ4_PLAYER_2) == 0);
}

static void
test_claim_tile_arguments_must_be_distinct(
    void)
{
    cj4_mahjong pon_state = make_empty_state();
    cj4_mahjong minkan_state = make_empty_state();
    const cj4_tile_id pon_hand[] = {
        tile(3, 1),
        tile(3, 2)};
    const cj4_tile_id minkan_hand[] = {
        tile(3, 1),
        tile(3, 2),
        tile(3, 3)};

    set_hand(&pon_state, CJ4_PLAYER_2, pon_hand, (uint8_t)(sizeof(pon_hand) / sizeof(pon_hand[0])));
    cj4_state_set_phase(&pon_state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&pon_state, CJ4_PLAYER_0);
    add_discard(&pon_state, CJ4_PLAYER_0, tile(3, 0));

    assert(cj4_can_pon(&pon_state, CJ4_PLAYER_2));
    assert(!cj4_can_pon_with_tile(&pon_state, CJ4_PLAYER_2, pon_hand[0], pon_hand[0]));

    set_hand(&minkan_state, CJ4_PLAYER_2, minkan_hand, (uint8_t)(sizeof(minkan_hand) / sizeof(minkan_hand[0])));
    cj4_state_set_phase(&minkan_state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&minkan_state, CJ4_PLAYER_0);
    add_discard(&minkan_state, CJ4_PLAYER_0, tile(3, 0));

    assert(cj4_can_minkan(&minkan_state, CJ4_PLAYER_2));
    assert(!cj4_can_minkan_with_tile(&minkan_state, CJ4_PLAYER_2, minkan_hand[0], minkan_hand[0], minkan_hand[1]));
}

static void
test_kuikae_forbidden_after_pon_and_chi(
    void)
{
    cj4_rules rules = cj4_rules_default();
    cj4_mahjong pon_state = make_empty_state();
    cj4_mahjong chi_state = make_empty_state();

    cj4_state_set_phase(&pon_state, CJ4_PHASE_AFTER_CALL);
    cj4_state_set_current_player(&pon_state, CJ4_PLAYER_1);
    set_test_meld(&pon_state, CJ4_PLAYER_1, 0, &(cj4_meld){.tiles = {tile(3, 0), tile(3, 1), tile(3, 2)}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_0, .called_index = 0});
    set_hand(&pon_state, CJ4_PLAYER_1, (const cj4_tile_id[]){tile(3, 3), tile(4, 0)}, 2);

    assert(!cj4_can_discard_with_rules(pon_state, &rules, tile(3, 3)));
    assert(cj4_can_discard_with_rules(pon_state, &rules, tile(4, 0)));
    rules.kuikae_forbidden = 0;
    assert(cj4_can_discard_with_rules(pon_state, &rules, tile(3, 3)));

    rules.kuikae_forbidden = 1;
    cj4_state_set_phase(&chi_state, CJ4_PHASE_AFTER_CALL);
    cj4_state_set_current_player(&chi_state, CJ4_PLAYER_1);
    set_test_meld(&chi_state, CJ4_PLAYER_1, 0, &(cj4_meld){.tiles = {tile(2, 0), tile(3, 0), tile(4, 0)}, .size = 3, .type = CJ4_MELD_CHI, .from_player = CJ4_PLAYER_0, .called_index = 0});
    set_hand(&chi_state, CJ4_PLAYER_1, (const cj4_tile_id[]){tile(2, 1), tile(5, 0), tile(6, 0)}, 3);

    assert(!cj4_can_discard_with_rules(chi_state, &rules, tile(2, 1)));
    assert(!cj4_can_discard_with_rules(chi_state, &rules, tile(5, 0)));
    assert(cj4_can_discard_with_rules(chi_state, &rules, tile(6, 0)));
}

static void
test_kan_flow_aborts_on_fourth_ankan(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_mahjong round_end;
    const cj4_tile_id ankan_tiles[] = {
        tile(0, 0),
        tile(0, 1),
        tile(0, 2),
        tile(0, 3)};
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(0, 1),
        tile(0, 2),
        tile(0, 3),
        tile(10, 0),
        tile(11, 0),
        tile(12, 0),
        tile(19, 0),
        tile(20, 0),
        tile(21, 0),
        tile(4, 0),
        tile(5, 0),
        tile(14, 0),
        tile(14, 1)};

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.draw_tile = tile(14, 1);
    set_test_kan(&state, CJ4_PLAYER_0, 0, CJ4_MELD_ANKAN, tile(5, 0));
    set_test_kan(&state, CJ4_PLAYER_1, 0, CJ4_MELD_MINKAN, tile(6, 0));
    set_test_kan(&state, CJ4_PLAYER_2, 0, CJ4_MELD_KAKAN, tile(7, 0));

    round_end = cj4_do_ankan(
        state,
        ankan_tiles[0],
        ankan_tiles[1],
        ankan_tiles[2],
        ankan_tiles[3]);

    assert(cj4_state_phase(&round_end) == CJ4_PHASE_ANKAN_RESOLVE);

    round_end = cj4_do_rinshan_draw(round_end, NULL);

    assert(cj4_state_phase(&round_end) == CJ4_PHASE_ROUND_END);
    assert(cj4_state_round_end_type(&round_end) == CJ4_ROUND_END_ABORTIVE_DRAW);
    assert(round_end.draw_tile == CJ4_TILE_ID_INVALID);
    assert(round_end.dead_wall_draw_count == 0);
}

static void
test_kan_dora_timing_for_ankan_and_kakan(
    void)
{
    cj4_mahjong ankan_state = make_empty_state();
    cj4_mahjong after_ankan;
    cj4_mahjong after_ankan_draw;
    cj4_mahjong kakan_state = make_empty_state();
    cj4_mahjong after_kakan_draw;
    cj4_mahjong after_discard;
    const cj4_tile_id ankan_tiles[] = {
        tile(0, 0),
        tile(0, 1),
        tile(0, 2),
        tile(0, 3)};

    set_hand(&ankan_state, CJ4_PLAYER_0, ankan_tiles, 4);
    cj4_state_set_phase(&ankan_state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&ankan_state, CJ4_PLAYER_0);
    ankan_state.draw_tile = ankan_tiles[3];
    ankan_state.dora_indicators_count = 1;
    set_test_wall(&ankan_state, (uint8_t)(130), tile(9, 0));

    after_ankan = cj4_do_ankan(
        ankan_state,
        ankan_tiles[0],
        ankan_tiles[1],
        ankan_tiles[2],
        ankan_tiles[3]);

    assert(cj4_state_phase(&after_ankan) == CJ4_PHASE_ANKAN_RESOLVE);
    assert(after_ankan.dora_indicators_count == 1);
    assert(cj4_count_melds(&after_ankan, CJ4_PLAYER_0) == 0);

    after_ankan_draw = cj4_do_rinshan_draw(after_ankan, NULL);
    assert(cj4_state_phase(&after_ankan_draw) == CJ4_PHASE_DRAW);
    assert(after_ankan_draw.dora_indicators_count == 2);
    assert(cj4_count_melds(&after_ankan_draw, CJ4_PLAYER_0) == 1);

    cj4_state_set_phase(&kakan_state, CJ4_PHASE_KAKAN_RESOLVE);
    cj4_state_set_current_player(&kakan_state, CJ4_PLAYER_0);
    kakan_state.pending_kakan_tile = tile(3, 0);
    kakan_state.dora_indicators_count = 1;
    set_test_wall(&kakan_state, (uint8_t)(130), tile(9, 1));

    after_kakan_draw = cj4_do_rinshan_draw(kakan_state, NULL);
    assert(cj4_state_phase(&after_kakan_draw) == CJ4_PHASE_DRAW);
    assert(after_kakan_draw.dora_indicators_count == 1);
    assert(after_kakan_draw.pending_kan_dora == 1);

    after_discard = cj4_do_discard(after_kakan_draw, after_kakan_draw.draw_tile);
    assert(after_discard.dora_indicators_count == 2);
    assert(after_discard.pending_kan_dora == 0);
}

static void
test_kan_requires_a_live_wall_tile(
    void)
{
    cj4_mahjong state = make_empty_state();
    const cj4_tile_id kan_tiles[] = {
        tile(0, 0),
        tile(0, 1),
        tile(0, 2),
        tile(0, 3)};

    set_hand(&state, CJ4_PLAYER_0, kan_tiles, 4);
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.draw_tile = kan_tiles[3];

    state.wall_pos = 121;
    assert(cj4_can_ankan(&state));

    state.wall_pos = 122;
    assert(!cj4_can_ankan(&state));
}

static void
test_consecutive_kakan_reveals_previous_dora_before_rinshan(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_mahjong after_first;
    cj4_mahjong after_second;
    cj4_mahjong after_discard;

    cj4_state_set_phase(&state, CJ4_PHASE_KAKAN_RESOLVE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.pending_kakan_tile = tile(3, 0);
    state.dora_indicators_count = 1;
    set_test_wall(&state, (uint8_t)(134), tile(4, 0));
    set_test_wall(&state, (uint8_t)(135), tile(5, 0));

    after_first = cj4_do_rinshan_draw(state, NULL);
    assert(after_first.pending_kan_dora == 1);
    assert(after_first.dora_indicators_count == 1);

    cj4_state_set_phase(&after_first, CJ4_PHASE_KAKAN_RESOLVE);
    after_first.pending_kakan_tile = tile(6, 0);
    after_second = cj4_do_rinshan_draw(after_first, NULL);
    assert(after_second.pending_kan_dora == 1);
    assert(after_second.dora_indicators_count == 2);

    after_discard = cj4_do_discard(after_second, after_second.draw_tile);
    assert(after_discard.pending_kan_dora == 0);
    assert(after_discard.dora_indicators_count == 3);
}

static void
test_minkan_then_ankan_preserves_pending_dora(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_mahjong after_minkan;
    cj4_mahjong after_ankan_declared;
    cj4_mahjong after_ankan_draw;
    cj4_mahjong after_discard;
    const cj4_tile_id ankan_tiles[] = {
        tile(0, 0),
        tile(0, 1),
        tile(0, 2),
        tile(0, 3)};

    cj4_state_set_phase(&state, CJ4_PHASE_ANKAN_RESOLVE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.pending_ankan_tile = CJ4_TILE_ID_INVALID;
    state.dora_indicators_count = 1;
    set_test_wall(&state, (uint8_t)(134), tile(0, 3));
    set_test_wall(&state, (uint8_t)(135), tile(9, 0));

    after_minkan = cj4_do_rinshan_draw(state, NULL);
    assert(after_minkan.pending_kan_dora == 1);

    set_hand(&after_minkan, CJ4_PLAYER_0, ankan_tiles, 4);
    after_minkan.draw_tile = ankan_tiles[3];
    after_ankan_declared = cj4_do_ankan(
        after_minkan,
        ankan_tiles[0],
        ankan_tiles[1],
        ankan_tiles[2],
        ankan_tiles[3]);
    assert(after_ankan_declared.pending_kan_dora == 1);
    assert(after_ankan_declared.dora_indicators_count == 1);

    after_ankan_draw = cj4_do_rinshan_draw(after_ankan_declared, NULL);
    assert(after_ankan_draw.pending_kan_dora == 0);
    assert(after_ankan_draw.dora_indicators_count == 3);

    after_discard = cj4_do_discard(after_ankan_draw, after_ankan_draw.draw_tile);
    assert(after_discard.pending_kan_dora == 0);
    assert(after_discard.dora_indicators_count == 3);
}

static void
test_previous_kan_dora_counts_on_following_rinshan_tsumo(
    void)
{
    cj4_rules rules = cj4_rules_tenhou();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong after_rinshan;
    cj4_mahjong without_previous_dora;
    cj4_mahjong won;
    cj4_hand_score score;
    cj4_hand_score score_without_previous_dora;
    cj4_hand_score won_score;
    cj4_tile_id draw = tile(5, 0);
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(3, 0),
        tile(4, 0),
        tile(15, 0),
        tile(15, 1)};

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_KAKAN_RESOLVE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.dealer = CJ4_PLAYER_1;
    state.pending_kakan_tile = tile(6, 0);
    state.pending_kan_dora = 1;
    state.dora_indicators_count = 1;
    set_test_wall(&state, (uint8_t)(130), tile(30, 0));
    set_test_wall(&state, (uint8_t)(128), tile(4, 1));
    set_test_wall(&state, (uint8_t)(134), draw);

    after_rinshan = cj4_do_rinshan_draw(state, &rules);

    assert(cj4_state_phase(&after_rinshan) == CJ4_PHASE_DRAW);
    assert(after_rinshan.draw_tile == draw);
    assert(after_rinshan.dora_indicators_count == 2);
    assert(after_rinshan.pending_kan_dora == 1);
    assert(cj4_calculate_hand_score(
        &after_rinshan,
        CJ4_PLAYER_0,
        &rules,
        &score));

    without_previous_dora = after_rinshan;
    without_previous_dora.dora_indicators_count = 1;
    assert(cj4_calculate_hand_score(
        &without_previous_dora,
        CJ4_PLAYER_0,
        &rules,
        &score_without_previous_dora));
    assert(score.han == score_without_previous_dora.han + 1);

    won = cj4_do_tsumo(after_rinshan);
    assert(won.dora_indicators_count == 2);
    assert(won.pending_kan_dora == 0);
    assert(cj4_calculate_hand_score(
        &won,
        CJ4_PLAYER_0,
        &rules,
        &won_score));
    assert(won_score.han == score.han);
}

static void
test_pending_kan_dora_is_discarded_on_win_and_capped(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_mahjong won;
    cj4_mahjong discarded;
    cj4_player winner = CJ4_PLAYER_1;

    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.pending_kan_dora = 2;
    state.dora_indicators_count = 1;
    add_discard(&state, CJ4_PLAYER_0, tile(1, 0));

    won = cj4_do_ron_multi(state, &winner, 1, NULL);
    assert(won.pending_kan_dora == 0);
    assert(won.dora_indicators_count == 1);

    state = make_empty_state();
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.draw_tile = tile(2, 0);
    state.locations[state.draw_tile].placement =
        cj4_location_make_hand(CJ4_PLAYER_0);
    state.pending_kan_dora = 4;
    state.dora_indicators_count = 4;

    discarded = cj4_do_discard(state, state.draw_tile);
    assert(discarded.pending_kan_dora == 0);
    assert(discarded.dora_indicators_count == 5);
}

static void
test_kokushi_ron_on_ankan_rule(
    void)
{
    cj4_rules rules = cj4_rules_default();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong ankan;
    const cj4_tile_id ankan_tiles[] = {
        tile(0, 0),
        tile(0, 1),
        tile(0, 2),
        tile(0, 3)};
    const cj4_tile_id kokushi[] = {
        tile(8, 0),
        tile(8, 1),
        tile(9, 0),
        tile(17, 0),
        tile(18, 0),
        tile(26, 0),
        tile(27, 0),
        tile(28, 0),
        tile(29, 0),
        tile(30, 0),
        tile(31, 0),
        tile(32, 0),
        tile(33, 0)};

    set_hand(&state, CJ4_PLAYER_0, ankan_tiles, 4);
    set_hand(&state, CJ4_PLAYER_1, kokushi, (uint8_t)(sizeof(kokushi) / sizeof(kokushi[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.draw_tile = ankan_tiles[3];

    ankan = cj4_do_ankan(
        state,
        ankan_tiles[0],
        ankan_tiles[1],
        ankan_tiles[2],
        ankan_tiles[3]);

    assert(cj4_can_ron(&ankan, CJ4_PLAYER_1, &rules));
    assert(ankan.dora_indicators_count == 0);
    assert(cj4_count_melds(&ankan, CJ4_PLAYER_0) == 0);
    rules.kokushi_ron_on_ankan = 0;
    assert(!cj4_can_ron(&ankan, CJ4_PLAYER_1, &rules));
}

static void
test_ankan_kokushi_ron_leaves_no_committed_kan(
    void)
{
    cj4_rules rules = cj4_rules_default();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong ankan;
    cj4_mahjong ron;
    cj4_win_result results[CJ4_PLAYER_COUNT];
    uint8_t result_count = 0;
    cj4_player winner = CJ4_PLAYER_1;
    const cj4_tile_id ankan_tiles[] = {
        tile(0, 0),
        tile(0, 1),
        tile(0, 2),
        tile(0, 3)};
    const cj4_tile_id kokushi[] = {
        tile(8, 0),
        tile(8, 1),
        tile(9, 0),
        tile(17, 0),
        tile(18, 0),
        tile(26, 0),
        tile(27, 0),
        tile(28, 0),
        tile(29, 0),
        tile(30, 0),
        tile(31, 0),
        tile(32, 0),
        tile(33, 0)};

    set_hand(&state, CJ4_PLAYER_0, ankan_tiles, 4);
    set_hand(&state, CJ4_PLAYER_1, kokushi, (uint8_t)(sizeof(kokushi) / sizeof(kokushi[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.draw_tile = ankan_tiles[3];
    state.dora_indicators_count = 1;

    ankan = cj4_do_ankan(
        state,
        ankan_tiles[0],
        ankan_tiles[1],
        ankan_tiles[2],
        ankan_tiles[3]);
    ron = cj4_do_ron_multi(ankan, &winner, 1, &rules);

    assert(cj4_state_phase(&ron) == CJ4_PHASE_ROUND_END);
    assert(ron.dora_indicators_count == 1);
    assert(cj4_count_melds(&ron, CJ4_PLAYER_0) == 0);
    assert(cj4_location_is_hand(ron.locations[ankan_tiles[1]].placement));
    assert(cj4_location_placement_player(
               ron.locations[ankan_tiles[1]].placement) == CJ4_PLAYER_0);
    assert(cj4_collect_winning_results(
        &ron,
        &rules,
        results,
        CJ4_PLAYER_COUNT,
        &result_count));
    assert(result_count == 1);
    assert(results[0].player == CJ4_PLAYER_1);
    assert(contains_win_yaku(&results[0], CJ4_WIN_YAKU_KOKUSHI) ||
           contains_win_yaku(&results[0], CJ4_WIN_YAKU_KOKUSHI_13_WAIT));
}

static void
test_triple_ron_abortive_draw_rule(
    void)
{
    cj4_rules rules = cj4_rules_default();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong round_end;
    cj4_mahjong settled;
    cj4_player claimers[] = {CJ4_PLAYER_1, CJ4_PLAYER_2, CJ4_PLAYER_3};

    rules.triple_ron_abortive_draw = 1;
    rules.max_ron_players = 3;

    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.dealer = CJ4_PLAYER_0;
    state.honba = 2;
    state.riichi_sticks = 1;
    add_discard(&state, CJ4_PLAYER_0, tile(0, 0));

    round_end = cj4_do_ron_multi(state, claimers, 3, &rules);

    assert(cj4_state_phase(&round_end) == CJ4_PHASE_ROUND_END);
    assert(cj4_state_round_end_type(&round_end) == CJ4_ROUND_END_ABORTIVE_DRAW);
    assert(cj4_state_abortive_reason(&round_end) == CJ4_ABORTIVE_DRAW_TRIPLE_RON);
    assert(cj4_state_winner_count(&round_end) == 0);

    settled = cj4_do_settle(round_end, &rules);
    assert(settled.honba == 3);
    assert(settled.riichi_sticks == 1);
    assert(settled.scores[CJ4_PLAYER_0] == 25000);
}

static void
test_exhaustive_draw_uses_shape_tenpai(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(3, 0),
        tile(4, 0),
        tile(15, 0),
        tile(15, 1)};

    rules.noten_penalty = 1;
    rules.noten_penalty_points = 3000;

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    cj4_state_set_round_result(&state, CJ4_ROUND_END_EXHAUSTIVE_DRAW, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.dealer = CJ4_PLAYER_0;
    state.winner_mask = 0;

    settled = cj4_do_settle(state, &rules);

    assert(settled.scores[CJ4_PLAYER_0] == 28000);
    assert(settled.scores[CJ4_PLAYER_1] == 24000);
    assert(settled.scores[CJ4_PLAYER_2] == 24000);
    assert(settled.scores[CJ4_PLAYER_3] == 24000);
    assert(settled.next_dealer == CJ4_PLAYER_0);
}

static void
test_exhaustive_draw_uses_live_wall_not_discard_count(
    void)
{
    cj4_mahjong state = make_empty_state();
    cj4_mahjong drawn;
    cj4_mahjong discarded;
    cj4_mahjong round_end;
    cj4_tile_id last_draw = tile(33, 3);

    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    cj4_state_set_first_turn(&state, 0);
    state.discard_count = CJ4_MAX_DRAWS;
    state.wall_pos = 120;
    state.dead_wall_draw_count = 1;
    set_test_wall(&state, (uint8_t)(120), last_draw);

    drawn = cj4_do_pass(state, NULL);

    assert(cj4_state_phase(&drawn) == CJ4_PHASE_DRAW);
    assert(cj4_state_current_player(&drawn) == CJ4_PLAYER_1);
    assert(drawn.draw_tile == last_draw);
    assert(drawn.wall_pos == 121);

    discarded = cj4_do_discard(drawn, last_draw);
    round_end = cj4_do_pass(discarded, NULL);

    assert(cj4_state_phase(&round_end) == CJ4_PHASE_ROUND_END);
    assert(cj4_state_round_end_type(&round_end) == CJ4_ROUND_END_EXHAUSTIVE_DRAW);
}

static void
test_last_discard_cannot_be_called(
    void)
{
    cj4_mahjong state = make_empty_state();
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 1),
        tile(2, 2),
        tile(2, 3)};

    set_hand(&state, CJ4_PLAYER_1, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.wall_pos = 121;
    add_discard(&state, CJ4_PLAYER_0, tile(2, 0));

    assert(cj4_can_chi(&state));
    assert(cj4_can_pon(&state, CJ4_PLAYER_1));
    assert(cj4_can_minkan(&state, CJ4_PLAYER_1));

    state.wall_pos = 122;
    assert(!cj4_can_chi(&state));
    assert(!cj4_can_pon(&state, CJ4_PLAYER_1));
    assert(!cj4_can_minkan(&state, CJ4_PLAYER_1));
}

static void
test_haitei_uses_kan_adjusted_live_wall_end(
    void)
{
    cj4_rules rules = cj4_rules_default();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong won;
    cj4_win_result results[CJ4_PLAYER_COUNT];
    uint8_t result_count = 0;
    cj4_tile_id draw = tile(5, 0);
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(3, 0),
        tile(4, 0),
        tile(15, 0),
        tile(15, 1),
        draw};

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.dealer = CJ4_PLAYER_1;
    state.draw_tile = draw;
    state.wall_pos = 121;
    state.dead_wall_draw_count = 1;
    set_test_wall(&state, (uint8_t)(120), draw);

    won = cj4_do_tsumo(state);

    assert(cj4_collect_winning_results(
        &won,
        &rules,
        results,
        CJ4_PLAYER_COUNT,
        &result_count));
    assert(result_count == 1);
    assert(contains_win_yaku(&results[0], CJ4_WIN_YAKU_HAITEI));
}

static void
test_exhaustive_draw_resets_honba_when_dealer_is_noten(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(3, 0),
        tile(4, 0),
        tile(15, 0),
        tile(15, 1)};

    rules.noten_penalty = 1;
    rules.noten_penalty_points = 3000;

    set_hand(&state, CJ4_PLAYER_1, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    cj4_state_set_round_result(&state, CJ4_ROUND_END_EXHAUSTIVE_DRAW, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.dealer = CJ4_PLAYER_0;
    state.honba = 2;
    state.winner_mask = 0;

    settled = cj4_do_settle(state, &rules);

    assert(settled.scores[CJ4_PLAYER_0] == 24000);
    assert(settled.scores[CJ4_PLAYER_1] == 28000);
    assert(settled.scores[CJ4_PLAYER_2] == 24000);
    assert(settled.scores[CJ4_PLAYER_3] == 24000);
    assert(settled.next_dealer == CJ4_PLAYER_1);
    assert(settled.honba == 0);
}

static void
test_tonpuu_enters_south_when_target_is_not_reached(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(3, 0),
        tile(4, 0),
        tile(15, 0),
        tile(15, 1)};

    rules.game_type = CJ4_GAME_TONPUU;
    rules.target_score = 30000;

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    state.round_wind = CJ4_WIND_EAST;
    cj4_state_set_round_result(&state, CJ4_ROUND_END_EXHAUSTIVE_DRAW, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_3);
    state.dealer = CJ4_PLAYER_3;
    state.winner_mask = 0;
    state.scores[CJ4_PLAYER_0] = 29000;
    state.scores[CJ4_PLAYER_1] = 28000;
    state.scores[CJ4_PLAYER_2] = 22000;
    state.scores[CJ4_PLAYER_3] = 21000;

    settled = cj4_do_settle(state, &rules);

    assert(settled.settlement_should_end == 0);
    assert(settled.next_dealer == CJ4_PLAYER_0);
    assert(settled.next_round_wind == CJ4_WIND_SOUTH);
}

static void
test_hanchan_enters_west_when_target_is_not_reached(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(3, 0),
        tile(4, 0),
        tile(15, 0),
        tile(15, 1)};

    rules.game_type = CJ4_GAME_HANCHAN;
    rules.target_score = 30000;

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    state.round_wind = CJ4_WIND_SOUTH;
    cj4_state_set_round_result(&state, CJ4_ROUND_END_EXHAUSTIVE_DRAW, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_3);
    state.dealer = CJ4_PLAYER_3;
    state.winner_mask = 0;
    state.scores[CJ4_PLAYER_0] = 29000;
    state.scores[CJ4_PLAYER_1] = 28000;
    state.scores[CJ4_PLAYER_2] = 22000;
    state.scores[CJ4_PLAYER_3] = 21000;

    settled = cj4_do_settle(state, &rules);

    assert(settled.settlement_should_end == 0);
    assert(settled.next_dealer == CJ4_PLAYER_0);
    assert(settled.next_round_wind == CJ4_WIND_WEST);
}

static void
test_tonpuu_ends_when_target_is_reached(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(3, 0),
        tile(4, 0),
        tile(15, 0),
        tile(15, 1)};

    rules.game_type = CJ4_GAME_TONPUU;
    rules.target_score = 30000;

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    state.round_wind = CJ4_WIND_EAST;
    cj4_state_set_round_result(&state, CJ4_ROUND_END_EXHAUSTIVE_DRAW, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_3);
    state.dealer = CJ4_PLAYER_3;
    state.winner_mask = 0;
    state.scores[CJ4_PLAYER_0] = 32000;
    state.scores[CJ4_PLAYER_1] = 26000;
    state.scores[CJ4_PLAYER_2] = 22000;
    state.scores[CJ4_PLAYER_3] = 20000;

    settled = cj4_do_settle(state, &rules);

    assert(settled.settlement_should_end == 1);
    assert(settled.next_round_wind == CJ4_WIND_SOUTH);
}

static void
test_tonpuu_does_not_end_before_east_4_when_non_dealer_reaches_target(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;

    rules.game_type = CJ4_GAME_TONPUU;
    rules.target_score = 30000;

    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    state.round_wind = CJ4_WIND_EAST;
    cj4_state_set_round_result(&state, CJ4_ROUND_END_EXHAUSTIVE_DRAW, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_2);
    state.dealer = CJ4_PLAYER_2;
    state.winner_mask = 0;
    state.scores[CJ4_PLAYER_0] = 32000;
    state.scores[CJ4_PLAYER_1] = 25000;
    state.scores[CJ4_PLAYER_2] = 23000;
    state.scores[CJ4_PLAYER_3] = 20000;

    settled = cj4_do_settle(state, &rules);

    assert(settled.settlement_should_end == 0);
    assert(settled.next_dealer == CJ4_PLAYER_3);
    assert(settled.next_round_wind == CJ4_WIND_EAST);
}

static void
test_tonpuu_does_not_end_before_east_4_when_dealer_renchan_is_top(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;

    rules.game_type = CJ4_GAME_TONPUU;
    rules.target_score = 30000;

    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    state.round_wind = CJ4_WIND_EAST;
    cj4_state_set_round_result(&state, CJ4_ROUND_END_ABORTIVE_DRAW, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_2);
    state.dealer = CJ4_PLAYER_2;
    state.winner_mask = 0;
    state.scores[CJ4_PLAYER_0] = 25000;
    state.scores[CJ4_PLAYER_1] = 24000;
    state.scores[CJ4_PLAYER_2] = 31000;
    state.scores[CJ4_PLAYER_3] = 20000;

    settled = cj4_do_settle(state, &rules);

    assert(settled.settlement_should_end == 0);
    assert(settled.next_dealer == CJ4_PLAYER_2);
    assert(settled.next_round_wind == CJ4_WIND_EAST);
}

static void
test_tonpuu_ends_when_score_matches_target(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(3, 0),
        tile(4, 0),
        tile(15, 0),
        tile(15, 1)};

    rules.game_type = CJ4_GAME_TONPUU;
    rules.target_score = 30000;

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    state.round_wind = CJ4_WIND_EAST;
    cj4_state_set_round_result(&state, CJ4_ROUND_END_EXHAUSTIVE_DRAW, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_3);
    state.dealer = CJ4_PLAYER_3;
    state.winner_mask = 0;
    state.scores[CJ4_PLAYER_0] = 30000;
    state.scores[CJ4_PLAYER_1] = 28000;
    state.scores[CJ4_PLAYER_2] = 22000;
    state.scores[CJ4_PLAYER_3] = 20000;

    settled = cj4_do_settle(state, &rules);

    assert(settled.settlement_should_end == 1);
    assert(settled.next_dealer == CJ4_PLAYER_0);
    assert(settled.next_round_wind == CJ4_WIND_SOUTH);
}

static void
test_tonpuu_ends_when_dealer_is_top_and_matches_target(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(3, 0),
        tile(4, 0),
        tile(15, 0),
        tile(15, 1)};

    rules.game_type = CJ4_GAME_TONPUU;
    rules.target_score = 30000;

    set_hand(&state, CJ4_PLAYER_3, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    state.round_wind = CJ4_WIND_EAST;
    cj4_state_set_round_result(&state, CJ4_ROUND_END_EXHAUSTIVE_DRAW, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_3);
    state.dealer = CJ4_PLAYER_3;
    state.honba = 2;
    state.winner_mask = 0;
    state.scores[CJ4_PLAYER_0] = 28000;
    state.scores[CJ4_PLAYER_1] = 26000;
    state.scores[CJ4_PLAYER_2] = 15000;
    state.scores[CJ4_PLAYER_3] = 30000;

    settled = cj4_do_settle(state, &rules);

    assert(settled.settlement_should_end == 1);
    assert(settled.next_dealer == CJ4_PLAYER_3);
    assert(settled.next_round_wind == CJ4_WIND_EAST);
    assert(settled.honba == 2);
}

static void
test_tonpuu_continues_when_dealer_renchan_is_not_top(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(3, 0),
        tile(4, 0),
        tile(15, 0),
        tile(15, 1)};

    rules.game_type = CJ4_GAME_TONPUU;
    rules.target_score = 30000;

    set_hand(&state, CJ4_PLAYER_3, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    state.round_wind = CJ4_WIND_EAST;
    cj4_state_set_round_result(&state, CJ4_ROUND_END_EXHAUSTIVE_DRAW, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_3);
    state.dealer = CJ4_PLAYER_3;
    state.winner_mask = 0;
    state.scores[CJ4_PLAYER_0] = 32000;
    state.scores[CJ4_PLAYER_1] = 25000;
    state.scores[CJ4_PLAYER_2] = 14000;
    state.scores[CJ4_PLAYER_3] = 29000;

    settled = cj4_do_settle(state, &rules);

    assert(settled.settlement_should_end == 0);
    assert(settled.next_dealer == CJ4_PLAYER_3);
    assert(settled.next_round_wind == CJ4_WIND_EAST);
}

static void
test_tonpuu_ending_on_dealer_tenpai_draw_does_not_increase_honba(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(3, 0),
        tile(4, 0),
        tile(15, 0),
        tile(15, 1)};

    rules.game_type = CJ4_GAME_TONPUU;
    rules.target_score = 30000;
    rules.noten_penalty = 1;
    rules.noten_penalty_points = 3000;

    set_hand(&state, CJ4_PLAYER_3, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    state.round_wind = CJ4_WIND_EAST;
    cj4_state_set_round_result(&state, CJ4_ROUND_END_EXHAUSTIVE_DRAW, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_3);
    state.dealer = CJ4_PLAYER_3;
    state.honba = 2;
    state.winner_mask = 0;
    state.scores[CJ4_PLAYER_0] = 24000;
    state.scores[CJ4_PLAYER_1] = 24000;
    state.scores[CJ4_PLAYER_2] = 24000;
    state.scores[CJ4_PLAYER_3] = 29000;

    settled = cj4_do_settle(state, &rules);

    assert(settled.settlement_should_end == 1);
    assert(settled.next_dealer == CJ4_PLAYER_3);
    assert(settled.next_round_wind == CJ4_WIND_EAST);
    assert(settled.honba == 2);
}

static void
test_tenhou_target_score_excludes_awarded_riichi_sticks(
    void)
{
    cj4_rules rules = cj4_rules_tenhou();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong won;
    cj4_mahjong settled;
    cj4_player winner = CJ4_PLAYER_2;
    const cj4_tile_id hand[] = {
        tile(1, 0),
        tile(2, 0),
        tile(3, 0),
        tile(10, 0),
        tile(11, 0),
        tile(12, 0),
        tile(19, 0),
        tile(20, 0),
        tile(21, 0),
        tile(4, 1),
        tile(5, 1),
        tile(14, 0),
        tile(14, 1)};

    set_hand(&state, winner, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    state.round_wind = CJ4_WIND_SOUTH;
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.dealer = CJ4_PLAYER_3;
    state.riichi_sticks = 1;
    state.scores[CJ4_PLAYER_0] = 32700;
    state.scores[CJ4_PLAYER_1] = 25000;
    state.scores[CJ4_PLAYER_2] = 21300;
    state.scores[CJ4_PLAYER_3] = 20000;
    add_discard(&state, CJ4_PLAYER_0, tile(6, 0));

    won = cj4_do_ron_multi(state, &winner, 1, &rules);
    settled = cj4_do_settle(won, &rules);

    assert(settled.scores[winner] == 30000);
    assert(settled.riichi_sticks == 0);
    assert(settled.settlement_should_end == 0);
    assert(settled.next_round_wind == CJ4_WIND_WEST);

    rules.target_score_excludes_riichi_sticks = 0;
    settled = cj4_do_settle(won, &rules);
    assert(settled.settlement_should_end == 1);
}

static void
test_tenhou_dealer_top_check_excludes_awarded_riichi_sticks(
    void)
{
    cj4_rules rules = cj4_rules_tenhou();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong won;
    cj4_mahjong settled;
    cj4_player winner = CJ4_PLAYER_3;
    const cj4_tile_id hand[] = {
        tile(1, 0),
        tile(2, 0),
        tile(3, 0),
        tile(10, 0),
        tile(11, 0),
        tile(12, 0),
        tile(19, 0),
        tile(20, 0),
        tile(21, 0),
        tile(4, 1),
        tile(5, 1),
        tile(14, 0),
        tile(14, 1)};

    set_hand(&state, winner, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    state.round_wind = CJ4_WIND_SOUTH;
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.dealer = winner;
    state.riichi_sticks = 1;
    state.scores[CJ4_PLAYER_0] = 36600;
    state.scores[CJ4_PLAYER_1] = 30000;
    state.scores[CJ4_PLAYER_2] = 14500;
    state.scores[CJ4_PLAYER_3] = 17900;
    add_discard(&state, CJ4_PLAYER_0, tile(6, 0));

    won = cj4_do_ron_multi(state, &winner, 1, &rules);
    settled = cj4_do_settle(won, &rules);

    assert(settled.scores[winner] == 30500);
    assert(settled.riichi_sticks == 0);
    assert(settled.settlement_should_end == 0);
    assert(settled.next_dealer == winner);

    rules.target_score_excludes_riichi_sticks = 0;
    settled = cj4_do_settle(won, &rules);
    assert(settled.settlement_should_end == 1);
}

static void
test_double_riichi_is_always_enabled(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_hand_score score;
    cj4_hand_score base_score;
    cj4_tile_id draw = tile(5, 0);
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(3, 0),
        tile(4, 0),
        tile(15, 0),
        tile(15, 1),
        draw};

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.dealer = CJ4_PLAYER_1;
    state.draw_tile = draw;
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_first_turn(&state, 0);
    cj4_state_set_draw_turn(&state, CJ4_PLAYER_0, 2);
    cj4_state_set_riichi(&state, CJ4_PLAYER_0, 1);

    assert(cj4_calculate_hand_score(&state, CJ4_PLAYER_0, &rules, &base_score));

    cj4_state_set_double_riichi(&state, CJ4_PLAYER_0, 1);
    assert(cj4_calculate_hand_score(&state, CJ4_PLAYER_0, &rules, &score));
    assert(score.han > base_score.han);
}

static void
test_score_rules_control_kazoe_and_kiriage(
    void)
{
    cj4_rules rules = cj4_rules_default();
    cj4_mahjong kazoe_state = make_empty_state();
    cj4_hand_score score;
    cj4_tile_id draw = tile(5, 0);
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(3, 0),
        tile(4, 0),
        tile(15, 0),
        tile(15, 1),
        draw};
    cj4_mahjong ron_state = make_empty_state();
    cj4_mahjong won;
    cj4_player winner = CJ4_PLAYER_0;
    const cj4_tile_id ron_hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(3, 0),
        tile(4, 0),
        tile(15, 0),
        tile(15, 1)};

    set_hand(&kazoe_state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_current_player(&kazoe_state, CJ4_PLAYER_0);
    kazoe_state.dealer = CJ4_PLAYER_1;
    kazoe_state.draw_tile = draw;
    cj4_state_set_phase(&kazoe_state, CJ4_PHASE_DRAW);
    cj4_state_set_riichi(&kazoe_state, CJ4_PLAYER_0, 1);
    cj4_state_set_double_riichi(&kazoe_state, CJ4_PLAYER_0, 1);
    kazoe_state.dora_indicators_count = 5;
    set_test_wall(&kazoe_state, (uint8_t)(130), tile(14, 0));
    set_test_wall(&kazoe_state, (uint8_t)(128), tile(14, 1));
    set_test_wall(&kazoe_state, (uint8_t)(126), tile(14, 2));
    set_test_wall(&kazoe_state, (uint8_t)(124), tile(14, 3));
    set_test_wall(&kazoe_state, (uint8_t)(122), tile(4, 0));
    set_test_wall(&kazoe_state, (uint8_t)(131), tile(2, 0));
    set_test_wall(&kazoe_state, (uint8_t)(129), tile(2, 1));
    set_test_wall(&kazoe_state, (uint8_t)(127), tile(2, 2));
    set_test_wall(&kazoe_state, (uint8_t)(125), tile(2, 3));
    set_test_wall(&kazoe_state, (uint8_t)(123), tile(3, 1));

    assert(cj4_calculate_hand_score(&kazoe_state, CJ4_PLAYER_0, &rules, &score));
    assert(score.yakuman_count == 1);

    rules.kazoe_yakuman = 0;
    assert(cj4_calculate_hand_score(&kazoe_state, CJ4_PLAYER_0, &rules, &score));
    assert(score.yakuman_count == 0);
    assert(score.han >= 13);

    rules = cj4_rules_default();
    rules.kiriage_mangan = 0;
    memset(rules.aka_tiles, 0, sizeof(rules.aka_tiles));
    set_hand(&ron_state, CJ4_PLAYER_0, ron_hand, (uint8_t)(sizeof(ron_hand) / sizeof(ron_hand[0])));
    cj4_state_set_phase(&ron_state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&ron_state, CJ4_PLAYER_1);
    ron_state.dealer = CJ4_PLAYER_1;
    cj4_state_set_riichi(&ron_state, CJ4_PLAYER_0, 1);
    add_discard(&ron_state, CJ4_PLAYER_1, draw);
    won = cj4_do_ron_multi(ron_state, &winner, 1, &rules);

    assert(cj4_calculate_hand_score(&won, CJ4_PLAYER_0, &rules, &score));
    assert(score.fu == 30);
    assert(score.han == 4);
    assert(score.ron_points == 7700);

    rules.kiriage_mangan = 1;
    assert(cj4_calculate_hand_score(&won, CJ4_PLAYER_0, &rules, &score));
    assert(score.ron_points == 8000);
}

static void
test_zero_initialized_rules_keep_v1_score_compatibility(
    void)
{
    cj4_rules zero_rules = {0};
    cj4_rules current_rules = cj4_rules_default();
    cj4_mahjong kokushi_state = make_empty_state();
    cj4_mahjong kazoe_state = make_empty_state();
    cj4_hand_score score;
    cj4_tile_id draw = tile(0, 1);
    const cj4_tile_id kokushi_hand[] = {
        tile(0, 0),
        tile(8, 0),
        tile(9, 0),
        tile(17, 0),
        tile(18, 0),
        tile(26, 0),
        tile(27, 0),
        tile(28, 0),
        tile(29, 0),
        tile(30, 0),
        tile(31, 0),
        tile(32, 0),
        tile(33, 0),
        draw};
    cj4_tile_id pinfu_draw = tile(5, 0);
    const cj4_tile_id pinfu_hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(3, 0),
        tile(4, 0),
        tile(15, 0),
        tile(15, 1),
        pinfu_draw};

    set_hand(&kokushi_state, CJ4_PLAYER_0, kokushi_hand, (uint8_t)(sizeof(kokushi_hand) / sizeof(kokushi_hand[0])));
    cj4_state_set_phase(&kokushi_state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&kokushi_state, CJ4_PLAYER_0);
    kokushi_state.draw_tile = draw;

    assert(cj4_calculate_hand_score(&kokushi_state, CJ4_PLAYER_0, &zero_rules, &score));
    assert(score.yakuman_count == 2);

    current_rules.kokushi_13_wait_double = 0;
    assert(cj4_calculate_hand_score(&kokushi_state, CJ4_PLAYER_0, &current_rules, &score));
    assert(score.yakuman_count == 1);

    current_rules = cj4_rules_default();
    set_hand(&kazoe_state, CJ4_PLAYER_0, pinfu_hand, (uint8_t)(sizeof(pinfu_hand) / sizeof(pinfu_hand[0])));
    cj4_state_set_current_player(&kazoe_state, CJ4_PLAYER_0);
    kazoe_state.dealer = CJ4_PLAYER_1;
    kazoe_state.draw_tile = pinfu_draw;
    cj4_state_set_phase(&kazoe_state, CJ4_PHASE_DRAW);
    cj4_state_set_riichi(&kazoe_state, CJ4_PLAYER_0, 1);
    cj4_state_set_double_riichi(&kazoe_state, CJ4_PLAYER_0, 1);
    kazoe_state.dora_indicators_count = 5;
    set_test_wall(&kazoe_state, (uint8_t)(130), tile(14, 0));
    set_test_wall(&kazoe_state, (uint8_t)(128), tile(14, 1));
    set_test_wall(&kazoe_state, (uint8_t)(126), tile(14, 2));
    set_test_wall(&kazoe_state, (uint8_t)(124), tile(14, 3));
    set_test_wall(&kazoe_state, (uint8_t)(122), tile(4, 0));
    set_test_wall(&kazoe_state, (uint8_t)(131), tile(2, 0));
    set_test_wall(&kazoe_state, (uint8_t)(129), tile(2, 1));
    set_test_wall(&kazoe_state, (uint8_t)(127), tile(2, 2));
    set_test_wall(&kazoe_state, (uint8_t)(125), tile(2, 3));
    set_test_wall(&kazoe_state, (uint8_t)(123), tile(3, 1));

    assert(cj4_calculate_hand_score(&kazoe_state, CJ4_PLAYER_0, &zero_rules, &score));
    assert(score.yakuman_count == 1);

    current_rules.kazoe_yakuman = 0;
    assert(cj4_calculate_hand_score(&kazoe_state, CJ4_PLAYER_0, &current_rules, &score));
    assert(score.yakuman_count == 0);
    assert(score.han >= 13);
}

static void
test_collect_winning_results_returns_tsumo_details(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong won;
    cj4_win_result results[CJ4_PLAYER_COUNT];
    uint8_t result_count = 0;
    cj4_tile_id draw = tile(5, 0);
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(3, 0),
        tile(4, 0),
        tile(15, 0),
        tile(15, 1),
        draw};

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.dealer = CJ4_PLAYER_1;
    state.draw_tile = draw;
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_riichi(&state, CJ4_PLAYER_0, 1);
    cj4_state_set_double_riichi(&state, CJ4_PLAYER_0, 1);

    won = cj4_do_tsumo(state);

    assert(cj4_collect_winning_results(
        &won,
        &rules,
        results,
        CJ4_PLAYER_COUNT,
        &result_count));
    assert(result_count == 1);
    assert(results[0].player == CJ4_PLAYER_0);
    assert(results[0].han == 6);
    assert(results[0].fu == 20);
    assert(results[0].yakuman_count == 0);
    assert(results[0].ron_points == 0);
    assert(results[0].tsumo_dealer_payment == 6000);
    assert(results[0].tsumo_non_dealer_payment == 3000);
    assert(results[0].dora_count == 0);
    assert(results[0].ura_dora_count == 0);
    assert(results[0].aka_dora_count == 0);
    assert(contains_win_yaku(&results[0], CJ4_WIN_YAKU_DOUBLE_RIICHI));
    assert(contains_win_yaku(&results[0], CJ4_WIN_YAKU_MENZEN_TSUMO));
    assert(contains_win_yaku(&results[0], CJ4_WIN_YAKU_PINFU));
    assert(contains_win_yaku(&results[0], CJ4_WIN_YAKU_SANSHOKU_DOUJUN));
    assert(!contains_win_yaku(&results[0], CJ4_WIN_YAKU_RIICHI));
}

static void
test_collect_winning_results_exposes_dora_indicators(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong won;
    cj4_win_result results[CJ4_PLAYER_COUNT];
    uint8_t result_count = 0;
    cj4_tile_id draw = tile(5, 0);
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(3, 0),
        tile(4, 0),
        tile(15, 0),
        tile(15, 1),
        draw};

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.dealer = CJ4_PLAYER_1;
    state.draw_tile = draw;
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_riichi(&state, CJ4_PLAYER_0, 1);

    won = cj4_do_tsumo(state);
    won.dora_indicators_count = 2;
    set_test_wall(&won, (uint8_t)(130), tile(27, 0));
    set_test_wall(&won, (uint8_t)(128), tile(31, 0));
    set_test_wall(&won, (uint8_t)(131), tile(28, 0));
    set_test_wall(&won, (uint8_t)(129), tile(32, 0));

    assert(cj4_collect_winning_results(
        &won,
        &rules,
        results,
        CJ4_PLAYER_COUNT,
        &result_count));
    assert(result_count == 1);
    assert(results[0].dora_indicators_count == 2);
    assert(results[0].dora_indicators[0] == tile(27, 0));
    assert(results[0].dora_indicators[1] == tile(31, 0));
    assert(results[0].ura_dora_indicators_count == 2);
    assert(results[0].ura_dora_indicators[0] == tile(28, 0));
    assert(results[0].ura_dora_indicators[1] == tile(32, 0));

    won.locations[tile(27, 0)].wall = CJ4_LOCATION_NONE;
    result_count = 0;
    assert(cj4_collect_winning_results(
        &won,
        &rules,
        results,
        CJ4_PLAYER_COUNT,
        &result_count));
    assert(result_count == 1);
    assert(results[0].dora_indicators_count == 1);
    assert(results[0].dora_indicators[0] == tile(31, 0));
}

static void
test_collect_winning_results_returns_ron_details(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong won;
    cj4_win_result results[CJ4_PLAYER_COUNT];
    uint8_t result_count = 0;
    cj4_player winners[] = {CJ4_PLAYER_2};
    const cj4_tile_id hand[] = {
        tile(1, 0),
        tile(2, 0),
        tile(3, 0),
        tile(10, 0),
        tile(11, 0),
        tile(12, 0),
        tile(19, 0),
        tile(20, 0),
        tile(21, 0),
        tile(4, 1),
        tile(5, 1),
        tile(14, 0),
        tile(14, 1)};

    rules.kuitan = 1;

    set_hand(&state, CJ4_PLAYER_2, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    add_discard(&state, CJ4_PLAYER_0, tile(6, 0));

    won = cj4_do_ron_multi(state, winners, 1, &rules);

    assert(cj4_collect_winning_results(
        &won,
        &rules,
        results,
        CJ4_PLAYER_COUNT,
        &result_count));
    assert(result_count == 1);
    assert(results[0].player == CJ4_PLAYER_2);
    assert(results[0].han == 4);
    assert(results[0].fu == 30);
    assert(results[0].yakuman_count == 0);
    assert(results[0].ron_points == 7700);
    assert(results[0].tsumo_dealer_payment == 0);
    assert(results[0].tsumo_non_dealer_payment == 0);
    assert(results[0].dora_count == 0);
    assert(results[0].ura_dora_count == 0);
    assert(results[0].aka_dora_count == 0);
    assert(contains_win_yaku(&results[0], CJ4_WIN_YAKU_TANYAO));
    assert(contains_win_yaku(&results[0], CJ4_WIN_YAKU_PINFU));
    assert(contains_win_yaku(&results[0], CJ4_WIN_YAKU_SANSHOKU_DOUJUN));
}

static void
test_collect_winning_results_hides_ura_dora_without_riichi(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong won;
    cj4_win_result results[CJ4_PLAYER_COUNT];
    uint8_t result_count = 0;
    cj4_player winners[] = {CJ4_PLAYER_2};
    const cj4_tile_id hand[] = {
        tile(1, 0),
        tile(2, 0),
        tile(3, 0),
        tile(10, 0),
        tile(11, 0),
        tile(12, 0),
        tile(19, 0),
        tile(20, 0),
        tile(21, 0),
        tile(4, 1),
        tile(5, 1),
        tile(14, 0),
        tile(14, 1)};

    rules.kuitan = 1;

    set_hand(&state, CJ4_PLAYER_2, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    add_discard(&state, CJ4_PLAYER_0, tile(6, 0));

    won = cj4_do_ron_multi(state, winners, 1, &rules);
    won.dora_indicators_count = 2;
    set_test_wall(&won, (uint8_t)(130), tile(27, 0));
    set_test_wall(&won, (uint8_t)(128), tile(31, 0));
    set_test_wall(&won, (uint8_t)(131), tile(28, 0));
    set_test_wall(&won, (uint8_t)(129), tile(32, 0));

    assert(cj4_collect_winning_results(
        &won,
        &rules,
        results,
        CJ4_PLAYER_COUNT,
        &result_count));
    assert(result_count == 1);
    assert(results[0].dora_indicators_count == 2);
    assert(results[0].dora_indicators[0] == tile(27, 0));
    assert(results[0].dora_indicators[1] == tile(31, 0));
    assert(results[0].ura_dora_indicators_count == 0);
}

static void
test_collect_winning_results_returns_multi_ron_details(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong won;
    cj4_mahjong settled;
    cj4_win_result results[CJ4_PLAYER_COUNT];
    uint8_t result_count = 0;
    cj4_player winners[] = {CJ4_PLAYER_3, CJ4_PLAYER_2};
    const cj4_tile_id ron_hand[] = {
        tile(1, 0),
        tile(2, 0),
        tile(3, 0),
        tile(10, 0),
        tile(11, 0),
        tile(12, 0),
        tile(19, 0),
        tile(20, 0),
        tile(21, 0),
        tile(4, 1),
        tile(5, 1),
        tile(14, 0),
        tile(14, 1)};
    const cj4_tile_id ron_hand_alt[] = {
        tile(1, 1),
        tile(2, 1),
        tile(3, 1),
        tile(10, 1),
        tile(11, 1),
        tile(12, 1),
        tile(19, 1),
        tile(20, 1),
        tile(21, 1),
        tile(4, 2),
        tile(5, 2),
        tile(14, 2),
        tile(14, 3)};

    rules.kuitan = 1;

    set_hand(&state, CJ4_PLAYER_2, ron_hand, (uint8_t)(sizeof(ron_hand) / sizeof(ron_hand[0])));
    set_hand(&state, CJ4_PLAYER_3, ron_hand_alt, (uint8_t)(sizeof(ron_hand_alt) / sizeof(ron_hand_alt[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    add_discard(&state, CJ4_PLAYER_0, tile(6, 0));

    won = cj4_do_ron_multi(state, winners, 2, &rules);

    assert(cj4_collect_winning_results(
        &won,
        &rules,
        results,
        CJ4_PLAYER_COUNT,
        &result_count));
    assert(result_count == 2);
    assert(results[0].player == CJ4_PLAYER_2);
    assert(results[1].player == CJ4_PLAYER_3);
    assert(results[0].han == 4);
    assert(results[1].han == 4);
    assert(results[0].fu == 30);
    assert(results[1].fu == 30);
    assert(contains_win_yaku(&results[0], CJ4_WIN_YAKU_TANYAO));
    assert(contains_win_yaku(&results[1], CJ4_WIN_YAKU_TANYAO));

    rules = cj4_rules_tenhou();
    state.honba = 2;
    won = cj4_do_ron_multi(state, winners, 2, &rules);
    settled = cj4_do_settle(won, &rules);

    assert(settled.scores[CJ4_PLAYER_0] == 9000);
    assert(settled.scores[CJ4_PLAYER_1] == 25000);
    assert(settled.scores[CJ4_PLAYER_2] == 33300);
    assert(settled.scores[CJ4_PLAYER_3] == 32700);
}

static void
test_release_settle_skips_uncalculable_win_scores(
    void)
{
#ifdef NDEBUG
    cj4_rules rules = {0};
    cj4_mahjong ron_state = make_empty_state();
    cj4_mahjong tsumo_state = make_empty_state();
    cj4_mahjong settled;

    rules.target_score = 30000;

    cj4_state_set_phase(&ron_state, CJ4_PHASE_ROUND_END);
    cj4_state_set_round_result(&ron_state, CJ4_ROUND_END_RON, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&ron_state, CJ4_PLAYER_0);
    ron_state.dealer = CJ4_PLAYER_0;
    ron_state.winner_mask = (uint8_t)(1u << CJ4_PLAYER_2);
    ron_state.winning_tile = CJ4_TILE_ID_INVALID;

    settled = cj4_do_settle(ron_state, &rules);

    assert(cj4_state_phase(&settled) == CJ4_PHASE_SETTLE);
    assert(settled.scores[CJ4_PLAYER_0] == 25000);
    assert(settled.scores[CJ4_PLAYER_1] == 25000);
    assert(settled.scores[CJ4_PLAYER_2] == 25000);
    assert(settled.scores[CJ4_PLAYER_3] == 25000);

    cj4_state_set_phase(&tsumo_state, CJ4_PHASE_ROUND_END);
    cj4_state_set_round_result(&tsumo_state, CJ4_ROUND_END_TSUMO, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&tsumo_state, CJ4_PLAYER_1);
    tsumo_state.dealer = CJ4_PLAYER_0;
    tsumo_state.winner_mask = (uint8_t)(1u << CJ4_PLAYER_1);
    tsumo_state.draw_tile = CJ4_TILE_ID_INVALID;
    tsumo_state.winning_tile = CJ4_TILE_ID_INVALID;

    settled = cj4_do_settle(tsumo_state, &rules);

    assert(cj4_state_phase(&settled) == CJ4_PHASE_SETTLE);
    assert(settled.scores[CJ4_PLAYER_0] == 25000);
    assert(settled.scores[CJ4_PLAYER_1] == 25000);
    assert(settled.scores[CJ4_PLAYER_2] == 25000);
    assert(settled.scores[CJ4_PLAYER_3] == 25000);
#endif
}

static void
test_max_ron_players_uses_head_bump_order(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong next;
    cj4_player claimers[] = {CJ4_PLAYER_3, CJ4_PLAYER_1, CJ4_PLAYER_2};

    rules.max_ron_players = 2;

    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.riichi_sticks = 2;
    add_discard(&state, CJ4_PLAYER_0, tile(0, 0));

    next = cj4_do_ron_multi(state, claimers, 3, &rules);

    assert(cj4_state_winner_count(&next) == 2);
    assert(cj4_get_winner(&next, 0) == CJ4_PLAYER_1);
    assert(cj4_get_winner(&next, 1) == CJ4_PLAYER_2);
}

static void
test_riichi_ankan_keeps_waits(
    void)
{
    cj4_mahjong state = make_empty_state();
    const cj4_tile_id kan_tiles[] = {
        tile(0, 0),
        tile(0, 1),
        tile(0, 2),
        tile(0, 3)};
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(0, 1),
        tile(0, 2),
        tile(0, 3),
        tile(10, 0),
        tile(11, 0),
        tile(12, 0),
        tile(19, 0),
        tile(20, 0),
        tile(21, 0),
        tile(4, 0),
        tile(5, 0),
        tile(15, 0),
        tile(15, 1)};

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    cj4_state_set_riichi(&state, CJ4_PLAYER_0, 1);
    state.draw_tile = tile(0, 3);

    assert(cj4_can_ankan_with_tile(
        &state,
        kan_tiles[0],
        kan_tiles[1],
        kan_tiles[2],
        kan_tiles[3]));
}

static void
test_kyuushu_kyuuhai_aborts_on_first_draw(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong next;
    cj4_tile_id draw = tile(30, 0);
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(8, 0),
        tile(9, 0),
        tile(17, 0),
        tile(18, 0),
        tile(26, 0),
        tile(27, 0),
        tile(28, 0),
        tile(29, 0),
        tile(2, 0),
        tile(3, 0),
        tile(4, 0),
        tile(5, 0),
        draw};

    rules.abortive_kyuushu_kyuuhai = 1;

    set_hand(&state, CJ4_PLAYER_0, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.draw_tile = draw;
    cj4_state_set_first_turn(&state, 1);
    cj4_state_set_draw_turn(&state, CJ4_PLAYER_0, 1);

    assert(cj4_can_kyuushu_kyuuhai(&state, &rules));

    next = cj4_do_kyuushu_kyuuhai(state);

    assert(cj4_state_phase(&next) == CJ4_PHASE_ROUND_END);
    assert(cj4_state_round_end_type(&next) == CJ4_ROUND_END_ABORTIVE_DRAW);
    assert(cj4_state_abortive_reason(&next) == CJ4_ABORTIVE_DRAW_KYUUSHU_KYUUHAI);
}

static void
test_suufon_renda_aborts_after_reactions_pass(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong next;

    rules.abortive_suufon_renda = 1;

    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_3);
    cj4_state_set_first_turn(&state, 1);
    add_discard(&state, CJ4_PLAYER_0, tile(27, 0));
    add_discard(&state, CJ4_PLAYER_1, tile(27, 1));
    add_discard(&state, CJ4_PLAYER_2, tile(27, 2));
    add_discard(&state, CJ4_PLAYER_3, tile(27, 3));

    next = cj4_do_pass(state, &rules);

    assert(cj4_state_phase(&next) == CJ4_PHASE_ROUND_END);
    assert(cj4_state_round_end_type(&next) == CJ4_ROUND_END_ABORTIVE_DRAW);
    assert(cj4_state_abortive_reason(&next) == CJ4_ABORTIVE_DRAW_SUUFON_RENDA);
}

static void
test_four_riichi_aborts_after_reactions_pass(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong next;

    rules.abortive_four_riichi = 1;

    cj4_state_set_phase(&state, CJ4_PHASE_DISCARD);
    cj4_state_set_current_player(&state, CJ4_PLAYER_3);
    add_discard(&state, CJ4_PLAYER_3, tile(4, 0));
    for (uint8_t player = 0; player < CJ4_PLAYER_COUNT; ++player)
        cj4_state_set_riichi(&state, player, 1);

    next = cj4_do_pass(state, &rules);

    assert(cj4_state_phase(&next) == CJ4_PHASE_ROUND_END);
    assert(cj4_state_round_end_type(&next) == CJ4_ROUND_END_ABORTIVE_DRAW);
    assert(cj4_state_abortive_reason(&next) == CJ4_ABORTIVE_DRAW_FOUR_RIICHI);
}

static void
test_nagashi_mangan_settles_as_mangan_tsumo(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;
    cj4_win_result results[CJ4_PLAYER_COUNT];
    uint8_t result_count = 0;

    rules.target_score = 30000;
    rules.nagashi_mangan = 1;

    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    cj4_state_set_round_result(&state, CJ4_ROUND_END_EXHAUSTIVE_DRAW, CJ4_ABORTIVE_DRAW_NONE);
    state.dealer = CJ4_PLAYER_0;
    add_discard(&state, CJ4_PLAYER_1, tile(8, 0));

    assert(cj4_collect_winning_results(
        &state,
        &rules,
        results,
        CJ4_PLAYER_COUNT,
        &result_count));
    assert(result_count == 1);
    assert(results[0].player == CJ4_PLAYER_1);
    assert(contains_win_yaku(&results[0], CJ4_WIN_YAKU_NAGASHI_MANGAN));

    settled = cj4_do_settle(state, &rules);

    assert(settled.scores[CJ4_PLAYER_0] == 21000);
    assert(settled.scores[CJ4_PLAYER_1] == 33000);
    assert(settled.scores[CJ4_PLAYER_2] == 23000);
    assert(settled.scores[CJ4_PLAYER_3] == 23000);
    assert(settled.next_dealer == CJ4_PLAYER_1);
}

static void
test_tenhou_nagashi_mangan_uses_dealer_tenpai_for_renchan(
    void)
{
    cj4_rules rules = cj4_rules_tenhou();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;
    const cj4_tile_id dealer_hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(2, 0),
        tile(9, 0),
        tile(10, 0),
        tile(11, 0),
        tile(18, 0),
        tile(19, 0),
        tile(20, 0),
        tile(3, 0),
        tile(4, 0),
        tile(15, 0),
        tile(15, 1)};

    set_hand(
        &state,
        CJ4_PLAYER_0,
        dealer_hand,
        (uint8_t)(sizeof(dealer_hand) / sizeof(dealer_hand[0])));
    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    cj4_state_set_round_result(&state, CJ4_ROUND_END_EXHAUSTIVE_DRAW, CJ4_ABORTIVE_DRAW_NONE);
    state.dealer = CJ4_PLAYER_0;
    add_discard(&state, CJ4_PLAYER_1, tile(8, 0));

    settled = cj4_do_settle(state, &rules);

    assert(settled.next_dealer == CJ4_PLAYER_0);
    assert(settled.honba == 1);
}

static void
test_pao_splits_ron_payment_with_responsible_player(
    void)
{
    cj4_rules rules = {0};
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;
    const cj4_tile_id haku[] = {tile(31, 0), tile(31, 1), tile(31, 2)};
    const cj4_tile_id hatsu[] = {tile(32, 0), tile(32, 1), tile(32, 2)};
    const cj4_tile_id chun[] = {tile(33, 0), tile(33, 1), tile(33, 2)};
    const cj4_tile_id hand[] = {
        tile(0, 0),
        tile(1, 0),
        tile(13, 0),
        tile(13, 1)};

    rules.pao = 1;
    rules.target_score = 30000;

    set_hand(&state, CJ4_PLAYER_2, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    set_test_meld(&state, CJ4_PLAYER_2, 0, &(cj4_meld){.tiles = {haku[0], haku[1], haku[2]}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_0, .called_index = 0});
    set_test_meld(&state, CJ4_PLAYER_2, 1, &(cj4_meld){.tiles = {hatsu[0], hatsu[1], hatsu[2]}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_1, .called_index = 0});
    set_test_meld(&state, CJ4_PLAYER_2, 2, &(cj4_meld){.tiles = {chun[0], chun[1], chun[2]}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_1, .called_index = 0});

    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    cj4_state_set_round_result(&state, CJ4_ROUND_END_RON, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.dealer = CJ4_PLAYER_0;
    state.winner_mask = (uint8_t)(1u << CJ4_PLAYER_2);
    state.winning_tile = tile(2, 0);
    cj4_state_set_pao(
        &state,
        CJ4_PLAYER_2,
        CJ4_PLAYER_1,
        CJ4_PAO_DAISANGEN);
    add_discard(&state, CJ4_PLAYER_0, state.winning_tile);
    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);

    settled = cj4_do_settle(state, &rules);

    assert(settled.scores[CJ4_PLAYER_0] == 9000);
    assert(settled.scores[CJ4_PLAYER_1] == 9000);
    assert(settled.scores[CJ4_PLAYER_2] == 57000);
}

static void
test_pao_liability_only_splits_compound_yakuman_tsumo(
    void)
{
    cj4_rules rules = cj4_rules_default();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong won;
    cj4_mahjong settled;
    const cj4_tile_id haku[] = {tile(31, 0), tile(31, 1), tile(31, 2)};
    const cj4_tile_id hatsu[] = {tile(32, 0), tile(32, 1), tile(32, 2)};
    const cj4_tile_id chun[] = {tile(33, 0), tile(33, 1), tile(33, 2)};
    cj4_tile_id draw = tile(27, 2);
    const cj4_tile_id hand[] = {
        tile(27, 0),
        tile(27, 1),
        tile(28, 0),
        tile(28, 1),
        draw};

    rules.pao = 1;
    rules.pao_liability_only = 1;

    set_hand(&state, CJ4_PLAYER_2, hand, (uint8_t)(sizeof(hand) / sizeof(hand[0])));
    set_test_meld(&state, CJ4_PLAYER_2, 0, &(cj4_meld){.tiles = {haku[0], haku[1], haku[2]}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_0, .called_index = 0});
    set_test_meld(&state, CJ4_PLAYER_2, 1, &(cj4_meld){.tiles = {hatsu[0], hatsu[1], hatsu[2]}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_1, .called_index = 0});
    set_test_meld(&state, CJ4_PLAYER_2, 2, &(cj4_meld){.tiles = {chun[0], chun[1], chun[2]}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_1, .called_index = 0});
    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&state, CJ4_PLAYER_2);
    state.dealer = CJ4_PLAYER_0;
    state.draw_tile = draw;
    cj4_state_set_pao(
        &state,
        CJ4_PLAYER_2,
        CJ4_PLAYER_1,
        CJ4_PAO_DAISANGEN);

    won = cj4_do_tsumo(state);
    settled = cj4_do_settle(won, &rules);

    assert(settled.scores[CJ4_PLAYER_0] == 9000);
    assert(settled.scores[CJ4_PLAYER_1] == -15000);
    assert(settled.scores[CJ4_PLAYER_2] == 89000);
    assert(settled.scores[CJ4_PLAYER_3] == 17000);
}

static void
test_daisuushii_double_pao_liability_uses_two_yakuman(
    void)
{
    cj4_rules rules = cj4_rules_default();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong won;
    cj4_mahjong settled;
    cj4_player winner = CJ4_PLAYER_2;
    cj4_tile_id draw = tile(4, 1);
    const cj4_tile_id pair[] = {tile(4, 0), draw};

    rules.pao = 1;
    rules.pao_liability_only = 1;
    rules.daisuushii_double = 1;

    cj4_state_set_phase(&state, CJ4_PHASE_DRAW);
    cj4_state_set_current_player(&state, winner);
    state.dealer = CJ4_PLAYER_0;
    state.draw_tile = draw;
    set_hand(&state, winner, pair, 2);
    set_test_meld(&state, winner, 0, &(cj4_meld){.tiles = {tile(27, 0), tile(27, 1), tile(27, 2)}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_0, .called_index = 0});
    set_test_meld(&state, winner, 1, &(cj4_meld){.tiles = {tile(28, 0), tile(28, 1), tile(28, 2)}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_1, .called_index = 0});
    set_test_meld(&state, winner, 2, &(cj4_meld){.tiles = {tile(29, 0), tile(29, 1), tile(29, 2)}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_1, .called_index = 0});
    set_test_meld(&state, winner, 3, &(cj4_meld){.tiles = {tile(30, 0), tile(30, 1), tile(30, 2)}, .size = 3, .type = CJ4_MELD_PON, .from_player = CJ4_PLAYER_1, .called_index = 0});
    cj4_state_set_pao(
        &state,
        winner,
        CJ4_PLAYER_1,
        CJ4_PAO_DAISUUSHII);

    won = cj4_do_tsumo(state);
    settled = cj4_do_settle(won, &rules);

    assert(settled.scores[CJ4_PLAYER_0] == 25000);
    assert(settled.scores[CJ4_PLAYER_1] == -39000);
    assert(settled.scores[CJ4_PLAYER_2] == 89000);
    assert(settled.scores[CJ4_PLAYER_3] == 25000);

    rules.daisuushii_double = 0;
    settled = cj4_do_settle(won, &rules);
    assert(settled.scores[CJ4_PLAYER_1] == -7000);
    assert(settled.scores[CJ4_PLAYER_2] == 57000);
}

static void
test_pao_type_flags_disable_suukantsu_liability(
    void)
{
    cj4_rules rules = cj4_rules_tenhou();
    cj4_mahjong state = make_empty_state();
    cj4_mahjong settled;
    cj4_player winner = CJ4_PLAYER_2;

    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    cj4_state_set_round_result(&state, CJ4_ROUND_END_RON, CJ4_ABORTIVE_DRAW_NONE);
    cj4_state_set_current_player(&state, CJ4_PLAYER_0);
    state.dealer = CJ4_PLAYER_0;
    state.winner_mask = (uint8_t)(1u << winner);
    state.winning_tile = tile(4, 0);
    cj4_state_set_pao(
        &state,
        winner,
        CJ4_PLAYER_1,
        CJ4_PAO_SUUKANTSU);
    add_discard(&state, CJ4_PLAYER_0, state.winning_tile);
    cj4_state_set_phase(&state, CJ4_PHASE_ROUND_END);
    set_hand(&state, winner, (const cj4_tile_id[]){tile(4, 1)}, 1);
    for (uint8_t i = 0; i < 4; ++i)
    {
        set_test_meld(&state, winner, i, &(cj4_meld){.tiles = {tile((cj4_tile_type)i, 0), tile((cj4_tile_type)i, 1), tile((cj4_tile_type)i, 2), tile((cj4_tile_type)i, 3)}, .size = 4, .type = CJ4_MELD_MINKAN, .from_player = CJ4_PLAYER_1, .called_index = 0});
    }

    settled = cj4_do_settle(state, &rules);
    assert(settled.scores[CJ4_PLAYER_1] == 25000);
}

int
main(
    void)
{
    test_v2_packed_location_and_state_layout();
    test_v2_location_reconstruction_and_player_mask();
    test_v2_initial_state_uses_locations_as_canonical_wall();
    test_v2_rejects_out_of_range_winning_tile();
    test_rules_default_and_validate();
    test_tenhou_preset_fields();
    test_riichi_uses_shape_tenpai();
    test_riichi_requires_four_live_wall_tiles();
    test_riichi_establishes_after_pass();
    test_riichi_ron_clears_pending_without_payment();
    test_riichi_called_discard_still_establishes();
    test_temporary_furiten_blocks_ron();
    test_riichi_furiten_is_recorded();
    test_permanent_furiten_blocks_other_wait();
    test_riichi_restricts_actions();
    test_seven_pairs_rejects_quad();
    test_chankan_pass_records_furiten();
    test_claim_tile_arguments_must_be_distinct();
    test_kuikae_forbidden_after_pon_and_chi();
    test_kan_flow_aborts_on_fourth_kakan_without_noten_penalty();
    test_kan_flow_aborts_on_fourth_ankan();
    test_kan_dora_timing_for_ankan_and_kakan();
    test_kan_requires_a_live_wall_tile();
    test_consecutive_kakan_reveals_previous_dora_before_rinshan();
    test_minkan_then_ankan_preserves_pending_dora();
    test_previous_kan_dora_counts_on_following_rinshan_tsumo();
    test_pending_kan_dora_is_discarded_on_win_and_capped();
    test_kokushi_ron_on_ankan_rule();
    test_ankan_kokushi_ron_leaves_no_committed_kan();
    test_exhaustive_draw_uses_shape_tenpai();
    test_exhaustive_draw_uses_live_wall_not_discard_count();
    test_last_discard_cannot_be_called();
    test_haitei_uses_kan_adjusted_live_wall_end();
    test_exhaustive_draw_resets_honba_when_dealer_is_noten();
    test_tonpuu_enters_south_when_target_is_not_reached();
    test_hanchan_enters_west_when_target_is_not_reached();
    test_tonpuu_ends_when_target_is_reached();
    test_tonpuu_does_not_end_before_east_4_when_non_dealer_reaches_target();
    test_tonpuu_does_not_end_before_east_4_when_dealer_renchan_is_top();
    test_tonpuu_ends_when_score_matches_target();
    test_tonpuu_ends_when_dealer_is_top_and_matches_target();
    test_tonpuu_continues_when_dealer_renchan_is_not_top();
    test_tonpuu_ending_on_dealer_tenpai_draw_does_not_increase_honba();
    test_tenhou_target_score_excludes_awarded_riichi_sticks();
    test_tenhou_dealer_top_check_excludes_awarded_riichi_sticks();
    test_double_riichi_is_always_enabled();
    test_score_rules_control_kazoe_and_kiriage();
    test_zero_initialized_rules_keep_v1_score_compatibility();
    test_collect_winning_results_returns_tsumo_details();
    test_collect_winning_results_exposes_dora_indicators();
    test_collect_winning_results_returns_ron_details();
    test_collect_winning_results_hides_ura_dora_without_riichi();
    test_collect_winning_results_returns_multi_ron_details();
    test_release_settle_skips_uncalculable_win_scores();
    test_max_ron_players_uses_head_bump_order();
    test_triple_ron_abortive_draw_rule();
    test_riichi_ankan_keeps_waits();
    test_kyuushu_kyuuhai_aborts_on_first_draw();
    test_suufon_renda_aborts_after_reactions_pass();
    test_four_riichi_aborts_after_reactions_pass();
    test_nagashi_mangan_settles_as_mangan_tsumo();
    test_tenhou_nagashi_mangan_uses_dealer_tenpai_for_renchan();
    test_pao_splits_ron_payment_with_responsible_player();
    test_pao_liability_only_splits_compound_yakuman_tsumo();
    test_daisuushii_double_pao_liability_uses_two_yakuman();
    test_pao_type_flags_disable_suukantsu_liability();
    manager_tests_main();
    return 0;
}
