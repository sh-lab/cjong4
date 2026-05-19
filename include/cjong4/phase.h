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
    CJ4_PHASE_DRAW,        /**< A tile has been drawn; tsumo/discard actions allowed */
    CJ4_PHASE_KAKAN_RESOLVE, /**< KaKan resolution phase (no reactions) */
    CJ4_PHASE_ANKAN_RESOLVE, /**< AnKan resolution phase (no reactions) */
    CJ4_PHASE_AFTER_CALL,  /**< A call (chi/pon) was made; current player must discard */
    CJ4_PHASE_DISCARD,     /**< A tile was discarded; other players may react */
    CJ4_PHASE_ROUND_END,   /**< The round has ended; no further actions allowed */
    CJ4_PHASE_SETTLE,      /**< Settling scores and determining next state */
    CJ4_PHASE_GAME_END     /**< The game has ended completely */
} cj4_phase;

#ifdef __cplusplus
}
#endif

#endif /* CJ4_PHASE_H */
