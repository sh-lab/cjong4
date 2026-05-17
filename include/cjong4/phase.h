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
    CJ4_PHASE_DRAW,       /**< A tile has been drawn; current player may act */
    CJ4_PHASE_DISCARD,    /**< A tile was discarded; reactions may occur */
    CJ4_PHASE_ROUND_END,  /**< The round has ended; no further actions allowed */
    CJ4_PHASE_SETTLE,     /**< Settling scores and determining next state */
    CJ4_PHASE_GAME_END    /**< The game has ended completely */
} cj4_phase;

#ifdef __cplusplus
}
#endif

#endif /* CJ4_PHASE_H */
