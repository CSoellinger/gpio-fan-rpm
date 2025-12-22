/**
 * This module provides common functionality shared between
 * measure and watch modes.
 *
 * @author  CSoellinger
 * @license LGPL-3.0-or-later
 */

#ifndef MEASUREMENT_COMMON_H
#define MEASUREMENT_COMMON_H

#include <pthread.h>
#include <stddef.h>
#include "gpio.h"
#include "format.h"
#include "line.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Measurement context for shared state between threads
 */
typedef struct {
    double *results;              /**< Array of measurement results */
    int *finished;                /**< Array tracking thread completion */
    pthread_t *threads;           /**< Array of thread handles */
    pthread_mutex_t results_mutex; /**< Mutex for results synchronization */
    pthread_cond_t all_finished;  /**< Condition for all threads done */
    char *chipname;               /**< GPIO chip name */
    int chipname_allocated;       /**< Whether chipname was allocated by us */
    size_t ngpio;                 /**< Number of GPIOs */
} measurement_ctx_t;

/**
 * Measurement parameters passed to threads
 */
typedef struct {
    int *gpios;                   /**< Array of GPIO numbers */
    size_t ngpio;                 /**< Number of GPIOs */
    int duration;                 /**< Measurement duration */
    int pulses;                   /**< Pulses per revolution */
    int warmup;                   /**< Warmup duration */
    edge_type_t edge;             /**< Edge detection type */
    int debug;                    /**< Debug flag */
    int watch;                    /**< Watch mode flag */
    output_mode_t mode;           /**< Output mode */
} measurement_params_t;

/**
 * Initialize measurement context
 *
 * @param ctx Context to initialize
 * @param gpios Array of GPIO numbers
 * @param ngpio Number of GPIOs
 * @param chipname GPIO chip name (NULL for auto-detect)
 * @return int 0 on success, -1 on error
 */
int measurement_ctx_init(measurement_ctx_t *ctx, int *gpios, size_t ngpio, char *chipname);

/**
 * Create measurement threads for all GPIOs
 *
 * @param ctx Initialized measurement context
 * @param params Measurement parameters
 * @return int 0 on success, -1 on error
 */
int measurement_create_threads(measurement_ctx_t *ctx, const measurement_params_t *params);

/**
 * Join all measurement threads
 *
 * @param ctx Measurement context
 */
void measurement_join_threads(measurement_ctx_t *ctx);

/**
 * Clean up measurement context
 *
 * @param ctx Measurement context
 */
void measurement_ctx_cleanup(measurement_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* MEASUREMENT_COMMON_H */
