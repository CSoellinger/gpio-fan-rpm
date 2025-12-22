/**
 * This module handles continuous monitoring mode with parallel
 * measurement and ordered output for multiple GPIO pins.
 * 
 * @author  CSoellinger
 * @license LGPL-3.0-or-later
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <math.h>
#include "watch.h"
#include "measurement_common.h"
#include "format.h"
#include "stats.h"

// External variable for signal handling
extern volatile sig_atomic_t stop;

// Global terminal state for atexit restoration
static struct termios saved_termios;
static int saved_flags = 0;
static volatile int terminal_modified = 0;

typedef struct {
    struct termios old_termios;
    int old_flags;
} terminal_cleanup_t;

static void restore_terminal_atexit(void) {
    if (terminal_modified) {
        tcsetattr(STDIN_FILENO, TCSANOW, &saved_termios);
        fcntl(STDIN_FILENO, F_SETFL, saved_flags);
        terminal_modified = 0;
    }
}

static void restore_terminal_cleanup(void *arg) {
    terminal_cleanup_t *cleanup = (terminal_cleanup_t *)arg;
    tcsetattr(STDIN_FILENO, TCSANOW, &cleanup->old_termios);
    fcntl(STDIN_FILENO, F_SETFL, cleanup->old_flags);
    terminal_modified = 0;
}

static void* keyboard_monitor_thread(void *arg) {
    (void)arg;

    terminal_cleanup_t cleanup_data;
    struct termios new_termios;

    if (tcgetattr(STDIN_FILENO, &cleanup_data.old_termios) != 0) {
        return NULL;
    }

    cleanup_data.old_flags = fcntl(STDIN_FILENO, F_GETFL, 0);

    saved_termios = cleanup_data.old_termios;
    saved_flags = cleanup_data.old_flags;

    static int atexit_registered = 0;
    if (!atexit_registered) {
        atexit(restore_terminal_atexit);
        atexit_registered = 1;
    }

    new_termios = cleanup_data.old_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    new_termios.c_cc[VMIN] = 0;
    new_termios.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_termios) != 0) {
        return NULL;
    }

    fcntl(STDIN_FILENO, F_SETFL, cleanup_data.old_flags | O_NONBLOCK);

    terminal_modified = 1;

    pthread_cleanup_push(restore_terminal_cleanup, &cleanup_data);

    while (!stop) {
        char ch;
        if (read(STDIN_FILENO, &ch, 1) == 1) {
            if (ch == 'q' || ch == 'Q') {
                stop = 1;
                break;
            }
        }
        usleep(100000);
    }

    pthread_cleanup_pop(1);

    return NULL;
}

int run_watch_mode(int *gpios, size_t ngpio, char *chipname,
                   int duration, int pulses, int warmup, edge_type_t edge, int debug, output_mode_t mode) {
    measurement_ctx_t ctx;

    fprintf(stderr, "\nWatch mode started. Press 'q' to quit or Ctrl+C to interrupt.\n\n");

    // Initialize context (allocates arrays, mutex/cond, auto-detects chip)
    if (measurement_ctx_init(&ctx, gpios, ngpio, chipname) < 0) {
        return -1;
    }

    // Allocate statistics array (watch-mode specific)
    rpm_stats_t *stats = calloc(ngpio, sizeof(*stats));
    if (!stats) {
        fprintf(stderr, "Error: memory allocation failed\n");
        measurement_ctx_cleanup(&ctx);
        return -1;
    }

    for (size_t i = 0; i < ngpio; i++) {
        stats_init(&stats[i]);
    }

    // Create keyboard monitor thread
    pthread_t keyboard_thread;
    int keyboard_ret = pthread_create(&keyboard_thread, NULL, keyboard_monitor_thread, NULL);
    if (keyboard_ret) {
        fprintf(stderr, "Warning: cannot create keyboard monitor thread: %s\n", strerror(keyboard_ret));
        fprintf(stderr, "Use Ctrl+C to quit watch mode\n");
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
        .watch = 1,  // Watch mode
        .mode = mode
    };

    if (measurement_create_threads(&ctx, &params) < 0) {
        free(stats);
        measurement_ctx_cleanup(&ctx);
        return -1;
    }

    // Main loop for watch mode
    while (!stop) {
        // Wait for all threads to complete one measurement round
        pthread_mutex_lock(&ctx.results_mutex);
        while (!stop) {
            int all_done = 1;
            for (size_t i = 0; i < ngpio; i++) {
                if (!ctx.finished[i]) {
                    all_done = 0;
                    break;
                }
            }
            if (all_done) break;

            // Use timed wait to check stop flag periodically
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 1;
            int wait_ret = pthread_cond_timedwait(&ctx.all_finished, &ctx.results_mutex, &ts);
            if (wait_ret != 0 && wait_ret != ETIMEDOUT) {
                if (debug) {
                    fprintf(stderr, "Warning: pthread_cond_timedwait failed: %s\n",
                            strerror(wait_ret));
                }
            }
        }

        // Only output if we weren't interrupted
        if (!stop) {
            // Update statistics for each GPIO
            for (size_t i = 0; i < ngpio; i++) {
                stats_update(&stats[i], ctx.results[i]);
            }

            // Output results in order
            if (mode == MODE_JSON && ngpio > 1) {
                // Output as JSON array with stats
                printf("[");
                for (size_t i = 0; i < ngpio; i++) {
                    if (i > 0) printf(",");
                    double avg = stats_avg(&stats[i]);
                    printf("{\"gpio\":%d,\"rpm\":%d,\"min\":%d,\"max\":%d,\"avg\":%d}",
                           gpios[i], (int)round(ctx.results[i]),
                           (int)round(stats[i].min), (int)round(stats[i].max),
                           (int)round(avg));
                }
                printf("]\n");
            } else {
                // Output individual results in order with stats
                for (size_t i = 0; i < ngpio; i++) {
                    char *output = format_output(gpios[i], ctx.results[i], &stats[i], mode, duration);
                    if (output) {
                        printf("%s", output);
                        free(output);
                    }
                }
            }
            fflush(stdout);

            // Reset finished flags for next round
            memset(ctx.finished, 0, ngpio * sizeof(*ctx.finished));
        }

        pthread_mutex_unlock(&ctx.results_mutex);
    }

    // Wait for all measurement threads to finish
    measurement_join_threads(&ctx);

    // Wait for keyboard monitor thread
    if (keyboard_ret == 0) {
        pthread_join(keyboard_thread, NULL);
    }

    // Cleanup
    free(stats);
    measurement_ctx_cleanup(&ctx);

    return 0;
}
