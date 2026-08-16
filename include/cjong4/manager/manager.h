#ifndef CJ4M_MANAGER_H
#define CJ4M_MANAGER_H

#include <stdint.h>

#include "cjong4/core/rules.h"
#include "delegate.h"

#ifdef __cplusplus
extern "C"
{
#endif

    enum
    {
        CJ4M_MAX_ACTIONS = 64
    };

    cj4_player_view
    cj4m_make_player_view(
        const cj4_mahjong *state,
        cj4_player player);

    uint8_t
    cj4m_collect_actions(
        const cj4_mahjong *state,
        const cj4_rules *rules,
        cj4_player player,
        cj4_action *actions,
        uint8_t capacity);

    /*
     * A player delegate may be called twice in one step after a minkan or
     * kakan rinshan draw.  When tsumo is available, the first call offers
     * PASS/TSUMO before the new kan dora is revealed.  After PASS, the second
     * call exposes the new indicator and offers the remaining turn actions.
     * If a delegate returns an action that was not offered, PASS is selected
     * when available; otherwise the first offered action is selected.
     */
    cj4_mahjong
    cj4m_step(
        const cj4_mahjong *state,
        const cj4_rules *rules,
        const cj4m_player_delegate delegates[CJ4_PLAYER_COUNT]);

#ifdef __cplusplus
}
#endif

#endif /* CJ4M_MANAGER_H */
