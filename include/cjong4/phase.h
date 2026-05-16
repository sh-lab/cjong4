#ifndef CJ4_PHASE_H
#define CJ4_PHASE_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * cj4_phase - High-level phases used by the engine
 *
 * These values represent the current phase of play.
 */
typedef enum {
    CJ4_PHASE_DRAW,      /**< Current player has drawn a tile and can act */
    CJ4_PHASE_DISCARD,   /**< A tile was discarded; other players may react */
    CJ4_PHASE_ROUND_END, /**< Round has ended */
    CJ4_PHASE_GAME_END   /**< Game has ended */
} cj4_phase;

#ifdef __cplusplus
}
#endif

#endif /* CJ4_PHASE_H */
