/**
 * This module handles single measurement mode with parallel
 * measurement and ordered output for multiple GPIO pins.
 *
 * @author  CSoellinger
 * @license LGPL-3.0-or-later
 */

#include <stdio.h>
#include <stdlib.h>
#include "measure.h"
#include "measurement_common.h"
#include "format.h"

int run_single_measurement(int *gpios, size_t ngpio, char *chipname,
                          int duration, int pulses, int warmup, edge_type_t edge, int debug, output_mode_t mode) {
    measurement_ctx_t ctx;

    if (debug) {
        fprintf(stderr, "DEBUG: Starting measurement for %zu GPIOs\n", ngpio);
    }

    // Initialize context (allocates arrays, mutex/cond, auto-detects chip)
    if (measurement_ctx_init(&ctx, gpios, ngpio, chipname) < 0) {
        return -1;
    }

    // Set up parameters and create threads
    measurement_params_t params = {
        .gpios = gpios,
        .ngpio = ngpio,
        .duration = duration,
        .pulses = pulses,
        .warmup = warmup,
        .edge = edge,
        .debug = debug,
        .watch = 0,  // Single measurement
        .mode = mode
    };

    if (measurement_create_threads(&ctx, &params) < 0) {
        measurement_ctx_cleanup(&ctx);
        return -1;
    }

    // Wait for all threads to finish
    measurement_join_threads(&ctx);

    // Output results in order
    if (mode == MODE_JSON && ngpio > 1) {
        // Output as JSON array
        char *output = format_json_array(gpios, ctx.results, NULL, ngpio);
        if (output) {
            printf("%s", output);
            free(output);
        }
    } else {
        // Output individual results in order
        for (size_t i = 0; i < ngpio; i++) {
            // Skip interrupted measurements (negative values indicate interruption)
            if (ctx.results[i] < 0.0) {
                continue;
            }

            char *output = format_output(gpios[i], ctx.results[i], NULL, mode, duration);
            if (output) {
                printf("%s", output);
                free(output);
            }
        }
    }
    fflush(stdout);

    measurement_ctx_cleanup(&ctx);
    return 0;
}
