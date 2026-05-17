#ifndef CJ4_TILE_H
#define CJ4_TILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <assert.h>

/*
 * Tile identifiers.
 *
 * cj4_tile_id:
 *   Unique identifier for each tile instance.
 *   Range: 0-135
 *
 * cj4_tile_type:
 *   Tile type identifier.
 *   Range: 0-33
 *
 * Notes:
 *   - A tile_id represents a unique physical tile (not just its type).
 *   - Four tile_ids exist for each tile_type.
 *   - tile_id = tile_type * 4 + index
 */
typedef uint8_t cj4_tile_id;
typedef uint8_t cj4_tile_type;

/*
 * Tile system constants.
 */
enum {
    CJ4_TILE_ID_MIN = 0,
    CJ4_TILE_ID_MAX = 135,

    CJ4_TILE_TYPE_MIN = 0,
    CJ4_TILE_TYPE_MAX = 33,

    CJ4_TILE_TYPE_COUNT = 34,
    CJ4_TILE_PER_TYPE  = 4,

    CJ4_TILE_ID_COUNT = CJ4_TILE_TYPE_COUNT * CJ4_TILE_PER_TYPE,

    CJ4_TILE_ID_INVALID = 255

};

/*
 * Compile-time validation.
 */
#if defined(__cplusplus)
static_assert(CJ4_TILE_TYPE_COUNT * CJ4_TILE_PER_TYPE == 136,
              "Tile count must be 136");
#else
_Static_assert(CJ4_TILE_TYPE_COUNT * CJ4_TILE_PER_TYPE == 136,
               "Tile count must be 136");
#endif

/*
 * API contract
 *
 * All functions in this header are pure functions.
 *
 * Preconditions:
 *   - Arguments must be within the valid ranges described above.
 *
 * Behavior on invalid input:
 *   - Undefined behavior.
 *   - In debug builds, assertions will fail.
 *
 * Note:
 *   These functions are defined as static inline for header-only usage.
 */

/*
 * Get tile type from tile id.
 *
 * Preconditions:
 *   - id in [0, 135]
 */
static inline cj4_tile_type
cj4_tile_get_type(cj4_tile_id id)
{
    assert(id >= CJ4_TILE_ID_MIN && id <= CJ4_TILE_ID_MAX);
    return (cj4_tile_type)(id / CJ4_TILE_PER_TYPE);
}

typedef enum {
    CJ4_TILE_SUIT_MANZU = 0,
    CJ4_TILE_SUIT_PINZU = 1,
    CJ4_TILE_SUIT_SOUZU = 2,
    CJ4_TILE_SUIT_HONOR = 3
} cj4_tile_suit;

static inline cj4_tile_suit
cj4_tile_type_get_suit(cj4_tile_type type)
{
    assert(type >= CJ4_TILE_TYPE_MIN && type <= CJ4_TILE_TYPE_MAX);

    if (type <= 8)   return CJ4_TILE_SUIT_MANZU;
    if (type <= 17)  return CJ4_TILE_SUIT_PINZU;
    if (type <= 26)  return CJ4_TILE_SUIT_SOUZU;
    return CJ4_TILE_SUIT_HONOR;
}

static inline cj4_tile_suit
cj4_tile_get_suit(cj4_tile_id id)
{
    return cj4_tile_type_get_suit(cj4_tile_get_type(id));
}

static inline uint8_t
cj4_tile_type_is_yaochu(cj4_tile_type type)
{
    assert(type >= CJ4_TILE_TYPE_MIN && type <= CJ4_TILE_TYPE_MAX);

    if (type >= 27) return 1;

    uint8_t n = type % 9;
    return (n == 0 || n == 8);
}

static inline uint8_t
cj4_tile_is_yaochu(cj4_tile_id id)
{
    return cj4_tile_type_is_yaochu(cj4_tile_get_type(id));
}

static inline uint8_t
cj4_tile_type_get_number(cj4_tile_type type)
{
    assert(type >= CJ4_TILE_TYPE_MIN && type <= CJ4_TILE_TYPE_MAX);

    if (type >= 27) return 0;
    return (uint8_t)(type % 9 + 1);
}

static inline uint8_t
cj4_tile_get_number(cj4_tile_id id)
{
    return cj4_tile_type_get_number(cj4_tile_get_type(id));
}

/*
 * Get index (0-3) within a tile type.
 *
 * Preconditions:
 *   - id in [0, 135]
 */
static inline uint8_t
cj4_tile_get_index(cj4_tile_id id)
{
    assert(id >= CJ4_TILE_ID_MIN && id <= CJ4_TILE_ID_MAX);
    return (uint8_t)(id % CJ4_TILE_PER_TYPE);
}

/*
 * Create tile id from tile type and index.
 *
 * Preconditions:
 *   - type in [0, 33]
 *   - index in [0, 3]
 */
static inline cj4_tile_id
cj4_tile_make(cj4_tile_type type, uint8_t index)
{
    assert(type >= CJ4_TILE_TYPE_MIN && type <= CJ4_TILE_TYPE_MAX);
    assert(index < CJ4_TILE_PER_TYPE);
    return (cj4_tile_id)(type * CJ4_TILE_PER_TYPE + index);
}

#ifdef __cplusplus
}
#endif

#endif /* CJ4_TILE_H */