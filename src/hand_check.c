#include "hand_check.h"
#include "state_query.h"
#include "tile.h"

/* Recursive meld partitioning using type counts. */
static bool remove_melds(int counts[CJ4_TILE_TYPE_COUNT], int melds_needed)
{
    if (melds_needed == 0)
        return true;

    int i;
    for (i = 0; i < CJ4_TILE_TYPE_COUNT; ++i)
    {
        if (counts[i] > 0)
            break;
    }

    if (i == CJ4_TILE_TYPE_COUNT)
        return false;

    /* Triplet */
    if (counts[i] >= 3)
    {
        counts[i] -= 3;
        if (remove_melds(counts, melds_needed - 1))
        {
            counts[i] += 3;
            return true;
        }
        counts[i] += 3;
    }

    /* Sequence */
    if (i <= 26)
    {
        int number = i % 9;
        if (number <= 6)
        {
            if (counts[i+1] > 0 && counts[i+2] > 0)
            {
                counts[i]--; counts[i+1]--; counts[i+2]--;
                if (remove_melds(counts, melds_needed - 1))
                {
                    counts[i]++; counts[i+1]++; counts[i+2]++;
                    return true;
                }
                counts[i]++; counts[i+1]++; counts[i+2]++;
            }
        }
    }

    return false;
}


static bool check_standard(int counts_orig[CJ4_TILE_TYPE_COUNT], int melds_needed)
{
    int counts[CJ4_TILE_TYPE_COUNT];
    for (int i = 0; i < CJ4_TILE_TYPE_COUNT; ++i)
        counts[i] = counts_orig[i];

    int tiles = 0;
    for (int i = 0; i < CJ4_TILE_TYPE_COUNT; ++i)
        tiles += counts[i];

    if (tiles != melds_needed * 3 + 2)
        return false;

    for (int i = 0; i < CJ4_TILE_TYPE_COUNT; ++i)
    {
        if (counts[i] >= 2)
        {
            counts[i] -= 2;
            if (remove_melds(counts, melds_needed))
            {
                counts[i] += 2;
                return true;
            }
            counts[i] += 2;
        }
    }

    return false;
}

static bool check_seven_pairs(int counts[CJ4_TILE_TYPE_COUNT])
{
    int pair_count = 0;
    int tiles = 0;
    for (int i = 0; i < CJ4_TILE_TYPE_COUNT; ++i)
    {
        tiles += counts[i];
        pair_count += counts[i] / 2;
    }
    /* seven pairs requires 14 tiles and exactly 7 pairs (quads may count as two pairs) */
    return (tiles == 14 && pair_count == 7);
}

static bool check_kokushi(int counts[CJ4_TILE_TYPE_COUNT])
{
    /* Kokushi (thirteen orphans) set of types */
    const int terminals[13] = {
        0, 8,    /* manzu 1,9 */
        9, 17,   /* pinzu 1,9 */
        18, 26,  /* souzu 1,9 */
        27,28,29,30,31,32,33 /* honors */
    };

    int found = 0;
    int duplicates = 0;
    for (int i = 0; i < 13; ++i)
    {
        int t = terminals[i];
        if (counts[t] >= 1) 
        {
            found++;
        }
        if (counts[t] >= 2) 
        {
            duplicates++;
        }
    }
    return (found == 13 && duplicates == 1);
}

bool cj4_is_complete_hand(const cj4_mahjong* state, cj4_player player)
{
    int counts[34] = {0};
    int tiles = 0;

    for (int tid = 0; tid < CJ4_TILE_ID_COUNT; ++tid)
    {
        const cj4_location *loc = cj4_tile_location_const(state, (cj4_tile_id)tid);
        if (loc->zone == CJ4_ZONE_HAND && loc->owner == player)
        {
            cj4_tile_type type = cj4_tile_get_type((cj4_tile_id)tid);
            counts[type]++;
            tiles++;
        }
    }

    /* Compute expected number of concealed tiles based on existing melds. */
    if (state->meld_count[player] > CJ4_MAX_MELDS) return false; /* sanity */
    int melds = state->meld_count[player];
    int expected_tiles = (4 - melds) * 3 + 2;

    /* For the shape check, only consider concealed tiles (tiles in CJ4_ZONE_HAND).
     * Open/closed melds are assumed to be valid melds already and should not be
     * re-counted into the concealed tile grouping. Therefore, the number of
     * concealed tiles must equal expected_tiles.
     */
    if (tiles != expected_tiles) return false;

    /* If there are no melds, also allow kokushi and seven pairs shapes. If there
     * are melds, kokushi and seven pairs are impossible as shape-only checks.
     */
    if (melds == 0)
    {
        /* Check kokushi */
        if (check_kokushi(counts)) return true;

        /* Check seven pairs */
        if (check_seven_pairs(counts)) return true;
    }

    /* Check standard meld partitioning for the remaining concealed tiles. This
     * will attempt to partition concealed tiles into the needed number of melds
     * (which is 4 - melds) plus a pair.
     */
    int melds_needed = 4 - melds;
    if (check_standard(counts, melds_needed)) return true;

    return false;
}
