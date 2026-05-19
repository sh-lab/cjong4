#include "state_chi.h"
#include "state_query.h"
#include "state_ops.h"
#include <assert.h>

bool cj4_can_chi(const cj4_mahjong *state)
{
    if (state->phase != CJ4_PHASE_DISCARD)
    {
        return false;
    }

    /* Chi is only allowed for the left/next player */
    cj4_player next_player = cj4_next_player(state);

    cj4_tile_id last = cj4_get_last_discard_tile(state);
    cj4_tile_type last_type = cj4_tile_get_type(last);

    /* Cannot chi honor tiles */
    if (cj4_tile_get_suit(last) == CJ4_TILE_SUIT_HONOR)
    {
        return false;
    }

    /* Check for any sequence (x, x+1, x+2) that includes last_type
       and whether the player has the other two tiles in hand. */
    for (int offset = -2; offset <= 0; ++offset)
    {
        int t0 = (int)last_type + offset;
        int t1 = t0 + 1;
        int t2 = t0 + 2;

        /* ensure types are within number tile range (0..26) */
        if (t0 < 0 || t2 > 26)
            continue;

        /* ensure suit matches (safety) */
        if (cj4_tile_type_get_suit((cj4_tile_type)t0) != cj4_tile_get_suit(last))
            continue;

        /* last_type must be within t0..t2 (always true by construction) */

        /* compute both other types explicitly */
        cj4_tile_type other_a, other_b;
        if (last_type == (cj4_tile_type)t0)
        {
            other_a = (cj4_tile_type)(t0 + 1);
            other_b = (cj4_tile_type)(t0 + 2);
        }
        else if (last_type == (cj4_tile_type)t1)
        {
            other_a = (cj4_tile_type)(t0 + 0);
            other_b = (cj4_tile_type)(t0 + 2);
        }
        else
        {
            other_a = (cj4_tile_type)(t0 + 0);
            other_b = (cj4_tile_type)(t0 + 1);
        }

        if (cj4_count_hand(state, next_player, other_a) >= 1 &&
            cj4_count_hand(state, next_player, other_b) >= 1)
        {
            return true;
        }
    }

    return false;
}

bool cj4_can_chi_with_tile(const cj4_mahjong *state, cj4_tile_id tile1, cj4_tile_id tile2)
{
    if (!cj4_can_chi(state))
    {
        return false;
    }

    cj4_player next_player = cj4_next_player(state);

    cj4_tile_id last = cj4_get_last_discard_tile(state);

    /* Cannot chi honors */
    if (cj4_tile_get_suit(last) == CJ4_TILE_SUIT_HONOR)
        return false;

    cj4_tile_type t_last = cj4_tile_get_type(last);
    cj4_tile_type t1 = cj4_tile_get_type(tile1);
    cj4_tile_type t2 = cj4_tile_get_type(tile2);

    /* All tiles must be same suit */
    if (cj4_tile_type_get_suit(t1) != cj4_tile_type_get_suit(t_last) ||
        cj4_tile_type_get_suit(t2) != cj4_tile_type_get_suit(t_last))
    {
        return false;
    }

    /* Numbers must form consecutive sequence */
    uint8_t n_last = cj4_tile_type_get_number(t_last);
    uint8_t n1 = cj4_tile_type_get_number(t1);
    uint8_t n2 = cj4_tile_type_get_number(t2);

    /* Collect numbers into array and sort */
    uint8_t nums[3] = { n_last, n1, n2 };
    cj4_tile_id tiles[3] = { last, tile1, tile2 };

    /* Simple sort (ascending) for three elements */
    for (int i = 0; i < 3; ++i)
    {
        for (int j = i + 1; j < 3; ++j)
        {
            if (nums[i] > nums[j])
            {
                uint8_t tmpn = nums[i]; nums[i] = nums[j]; nums[j] = tmpn;
                cj4_tile_id tmpt = tiles[i]; tiles[i] = tiles[j]; tiles[j] = tmpt;
            }
        }
    }

    if (!(nums[0] + 1 == nums[1] && nums[1] + 1 == nums[2]))
    {
        return false;
    }

    /* Ensure tile1 and tile2 are indeed in the player's hand */
    const cj4_location *l1 = cj4_tile_location_const(state, tile1);
    const cj4_location *l2 = cj4_tile_location_const(state, tile2);

    if (l1->zone != CJ4_ZONE_HAND || l1->owner != next_player)
    {
        return false;
    }

    if (l2->zone != CJ4_ZONE_HAND || l2->owner != next_player)
    {
        return false;
    }

    return true;
}

cj4_mahjong
cj4_do_chi(const cj4_mahjong state, cj4_tile_id tile1, cj4_tile_id tile2)
{
    assert(cj4_can_chi_with_tile(&state, tile1, tile2));

    cj4_mahjong next = state;

    cj4_player next_player = cj4_next_player(&state);

    cj4_tile_id last = cj4_get_last_discard_tile(&state);

    cj4_meld *m = &next.melds[next_player][next.meld_count[next_player]++];
    m->type = CJ4_MELD_CHI;
    m->tiles[0] = last;
    m->tiles[1] = tile1;
    m->tiles[2] = tile2;
    m->size = 3;
    m->from_player = state.current_player;
    m->called_index = 0;


    next.locations[last].zone = CJ4_ZONE_MELD;
    next.locations[last].owner = next_player;
    next.locations[tile1].zone = CJ4_ZONE_MELD;
    next.locations[tile1].owner = next_player;
    next.locations[tile2].zone = CJ4_ZONE_MELD;
    next.locations[tile2].owner = next_player;

    cj4_state_call_post_process(&next);

    next.current_player = next_player;

    next.phase = CJ4_PHASE_AFTER_CALL;

    return next;
}
