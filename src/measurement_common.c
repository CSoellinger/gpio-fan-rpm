/**
 * This module provides common functionality shared between
 * measure and watch modes.
 *
 * @author  CSoellinger
 * @license LGPL-3.0-or-later
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "measurement_common.h"
#include "chip.h"

int measurement_ctx_init(measurement_ctx_t *ctx, int *gpios, size_t ngpio, char *chipname) {
    if (!ctx || !gpios || ngpio == 0) return -1;

    // Zero-initialize context
    memset(ctx, 0, sizeof(*ctx));
    ctx->ngpio = ngpio;

    // Allocate arrays
    ctx->results = calloc(ngpio, sizeof(*ctx->results));
    ctx->finished = calloc(ngpio, sizeof(*ctx->finished));
    ctx->threads = calloc(ngpio, sizeof(*ctx->threads));

    if (!ctx->results || !ctx->finished || !ctx->threads) {
        fprintf(stderr, "Error: memory allocation failed\n");
        measurement_ctx_cleanup(ctx);
        return -1;
    }

    // Initialize synchronization primitives
    if (pthread_mutex_init(&ctx->results_mutex, NULL) != 0) {
        fprintf(stderr, "Error: mutex init failed\n");
        measurement_ctx_cleanup(ctx);
        return -1;
    }

    if (pthread_cond_init(&ctx->all_finished, NULL) != 0) {
        fprintf(stderr, "Error: cond init failed\n");
        pthread_mutex_destroy(&ctx->results_mutex);
        measurement_ctx_cleanup(ctx);
        return -1;
    }

    // Auto-detect chip if not specified
    if (!chipname) {
        if (chip_auto_detect_for_name(gpios[0], &ctx->chipname) < 0) {
            fprintf(stderr, "Error: cannot auto-detect GPIO chip\n");
            pthread_mutex_destroy(&ctx->results_mutex);
            pthread_cond_destroy(&ctx->all_finished);
            measurement_ctx_cleanup(ctx);
            return -1;
        }
        ctx->chipname_allocated = 1;
    } else {
        ctx->chipname = chipname;
        ctx->chipname_allocated = 0;
    }

    return 0;
}

int measurement_create_threads(measurement_ctx_t *ctx, const measurement_params_t *params) {
    if (!ctx || !params) return -1;

    for (size_t i = 0; i < ctx->ngpio; i++) {
        thread_args_t *a = calloc(1, sizeof(*a));
        if (!a) {
            fprintf(stderr, "Error: memory allocation failed\n");
            continue;
        }

        a->gpio = params->gpios[i];
        a->chipname = ctx->chipname;
        a->duration = params->duration;
        a->pulses = params->pulses;
        a->warmup = params->warmup;
        a->edge = params->edge;
        a->debug = params->debug;
        a->watch = params->watch;
        a->mode = params->mode;
        a->thread_index = i;
        a->total_threads = ctx->ngpio;
        a->results = ctx->results;
        a->finished = ctx->finished;
        a->results_mutex = &ctx->results_mutex;
        a->all_finished = &ctx->all_finished;

        int ret = pthread_create(&ctx->threads[i], NULL, gpio_thread_fn, a);
        if (ret) {
            fprintf(stderr, "Error: cannot create thread for GPIO %d: %s\n",
                    a->gpio, strerror(ret));
            free(a);
            ctx->threads[i] = 0;
        }
    }

    return 0;
}

void measurement_join_threads(measurement_ctx_t *ctx) {
    if (!ctx) return;

    for (size_t i = 0; i < ctx->ngpio; i++) {
        if (ctx->threads[i]) {
            pthread_join(ctx->threads[i], NULL);
        }
    }
}

void measurement_ctx_cleanup(measurement_ctx_t *ctx) {
    if (!ctx) return;

    pthread_mutex_destroy(&ctx->results_mutex);
    pthread_cond_destroy(&ctx->all_finished);

    free(ctx->threads);
    free(ctx->results);
    free(ctx->finished);

    if (ctx->chipname_allocated && ctx->chipname) {
        free(ctx->chipname);
    }

    // Zero out the structure
    memset(ctx, 0, sizeof(*ctx));
}
