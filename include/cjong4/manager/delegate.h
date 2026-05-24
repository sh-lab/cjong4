#ifndef CJ4M_DELEGATE_H
#define CJ4M_DELEGATE_H

#include <stdint.h>

#include "cjong4/core/action.h"
#include "player_view.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef cj4_action (*cj4m_decide_action_fn)(
        void *ctx,
        const cj4_player_view *view,
        const cj4_action *actions,
        uint8_t action_count);

    typedef struct
    {
        void *ctx;
        cj4m_decide_action_fn decide;
    } cj4m_player_delegate;

#ifdef __cplusplus
}
#endif

#endif /* CJ4M_DELEGATE_H */
