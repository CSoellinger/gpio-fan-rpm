/**
 * This module implements GPIO operations.
 * 
 * @author  CSoellinger
 * @license LGPL-3.0-or-later
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <poll.h>
#include <sys/timerfd.h>
#include "gpio.h"
#include "chip.h"
#include "line.h"
#include "format.h"
#include "measurement_common.h"

// Global variables (extern declaration - defined in main.c)
// Note: sig_atomic_t is included via gpio.h -> signal.h
extern volatile sig_atomic_t stop;
extern pthread_mutex_t print_mutex;

/**
 * Result codes for timed_event_loop
 */
typedef enum {
    TIMED_LOOP_COMPLETED = 0,   /**< Timer expired normally */
    TIMED_LOOP_INTERRUPTED = 1, /**< Interrupted by stop flag */
    TIMED_LOOP_ERROR = -1       /**< Error occurred */
} timed_loop_result_t;

/**
 * Run a timed event loop that counts GPIO edge events
 *
 * @param ctx GPIO context
 * @param duration_sec Duration in seconds
 * @param count Pointer to store event count (only updated if not NULL)
 * @param debug Enable debug output
 * @param phase_name Name for debug output (e.g., "Warmup", "Measurement")
 * @return timed_loop_result_t Result code
 */
static timed_loop_result_t timed_event_loop(gpio_context_t *ctx, int duration_sec,
                                            unsigned int *count, int debug,
                                            const char *phase_name) {
    if (debug && phase_name) {
        fprintf(stderr, "%s phase: %d seconds\n", phase_name, duration_sec);
    }

    struct timespec start_ts;
    clock_gettime(CLOCK_MONOTONIC, &start_ts);

    int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (timerfd < 0) {
        if (debug) fprintf(stderr, "Warning: failed to create %s timer, using fallback\n",
                          phase_name ? phase_name : "");
        // Fallback to polling method
        struct timespec end_ts = start_ts;
        end_ts.tv_sec += duration_sec;

        while (!stop) {
            struct timespec current_ts;
            clock_gettime(CLOCK_MONOTONIC, &current_ts);
            if (current_ts.tv_sec >= end_ts.tv_sec) {
                return TIMED_LOOP_COMPLETED;
            }
            int wait_result = gpio_wait_event(ctx, 100000000LL);
            if (wait_result > 0) {
                if (gpio_read_event(ctx) < 0) {
                    if (debug) fprintf(stderr, "Warning: error reading event\n");
                    break;
                }
                if (count) (*count)++;
            }
        }
        return stop ? TIMED_LOOP_INTERRUPTED : TIMED_LOOP_COMPLETED;
    }

    // Set timer to expire after duration_sec seconds
    struct itimerspec timer_spec = {0};
    timer_spec.it_value.tv_sec = duration_sec;
    if (timerfd_settime(timerfd, 0, &timer_spec, NULL) < 0) {
        if (debug) fprintf(stderr, "Warning: failed to arm %s timer\n",
                          phase_name ? phase_name : "");
        close(timerfd);
        return TIMED_LOOP_ERROR;
    }

    // Use poll() on both GPIO event_fd and timerfd
    struct pollfd pfds[2];
    pfds[0].fd = ctx->event_fd;
    pfds[0].events = POLLIN;
    pfds[1].fd = timerfd;
    pfds[1].events = POLLIN;

    timed_loop_result_t result = TIMED_LOOP_INTERRUPTED;
    while (!stop) {
        int ret = poll(pfds, 2, 100);  // 100ms timeout for stop check
        if (ret < 0) {
            if (errno == EINTR) continue;
            break;
        }

        // Check if timer expired
        if (pfds[1].revents & POLLIN) {
            uint64_t expirations;
            ssize_t n = read(timerfd, &expirations, sizeof(expirations));
            (void)n;  // Intentionally ignoring read result (just consuming timer)
            result = TIMED_LOOP_COMPLETED;
            break;
        }

        // Read GPIO events if available
        if (pfds[0].revents & POLLIN) {
            if (gpio_read_event(ctx) < 0) {
                if (debug) fprintf(stderr, "Warning: error reading event\n");
                break;
            }
            if (count) (*count)++;
        }
    }

    close(timerfd);
    return result;
}

gpio_context_t* gpio_init(int gpio, const char *chipname) {
    gpio_context_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) return NULL;
    
    ctx->gpio = gpio;
    
    if (chipname) {
        // Use specified chip
        ctx->chip = chip_open_by_name(chipname);
        if (!ctx->chip) {
            fprintf(stderr, "Error: cannot open chip '%s'\n", chipname);
            free(ctx);
            return NULL;
        }
        ctx->chipname = strdup(chipname);
        if (!ctx->chipname) {
            fprintf(stderr, "Error: memory allocation failed\n");
            chip_close(ctx->chip);
            free(ctx);
            return NULL;
        }
    } else {
        // Auto-detect chip (this should rarely happen now)
        ctx->chip = chip_auto_detect(gpio, &ctx->chipname);
        if (!ctx->chip) {
            fprintf(stderr, "Error: cannot find suitable chip for GPIO %d\n", gpio);
            free(ctx);
            return NULL;
        }
    }
    
    return ctx;
}

void gpio_cleanup(gpio_context_t *ctx) {
    if (!ctx) return;

    if (ctx->event_buffer) {
        gpiod_edge_event_buffer_free(ctx->event_buffer);
        ctx->event_buffer = NULL;
    }

    if (ctx->request) {
        gpiod_line_request_release(ctx->request);
        ctx->request = NULL;
    }
    ctx->event_fd = -1;

    if (ctx->chip) {
        chip_close(ctx->chip);
        ctx->chip = NULL;
    }

    if (ctx->chipname) {
        free(ctx->chipname);
        ctx->chipname = NULL;
    }

    free(ctx);
}

int gpio_request_events(gpio_context_t *ctx, const char *consumer, edge_type_t edge) {
    if (!ctx || !ctx->chip) return -1;

    // Use the line module to request events
    line_request_t *line_req = line_request_events(ctx->chip, ctx->gpio, consumer, edge);
    if (!line_req) return -1;

    ctx->request = line_req->request;
    ctx->event_fd = line_req->event_fd;

    // Free the line request wrapper (but keep the underlying request)
    free(line_req);

    // Allocate reusable event buffer
    ctx->event_buffer = gpiod_edge_event_buffer_new(1);
    if (!ctx->event_buffer) {
        gpiod_line_request_release(ctx->request);
        ctx->request = NULL;
        ctx->event_fd = -1;
        return -1;
    }

    return 0;
}

int gpio_wait_event(gpio_context_t *ctx, int64_t timeout_ns) {
    if (!ctx) return -1;
    
    if (ctx->event_fd < 0) return -1;
    
    struct pollfd pfd;
    pfd.fd = ctx->event_fd;
    pfd.events = POLLIN;
    
    int timeout_ms = (timeout_ns >= 0) ? (timeout_ns / 1000000LL) : -1;
    int ret = poll(&pfd, 1, timeout_ms);
    
    if (ret < 0) return -1;  // Error
    if (ret == 0) return 0;  // Timeout
    return 1;  // Event available
}

int gpio_read_event(gpio_context_t *ctx) {
    if (!ctx) return -1;

    if (!ctx->request || !ctx->event_buffer) return -1;

    int ret = gpiod_line_request_read_edge_events(ctx->request, ctx->event_buffer, 1);
    // Note: We don't need to process the event details, just count it
    // The buffer is reused across calls for efficiency
    return ret;
}

/**
 * Measure RPM using GPIO edge detection
 *
 * This function performs a two-phase measurement:
 * 1. Warmup phase: configurable duration for fan stabilization
 * 2. Measurement phase: (duration - warmup) seconds for actual RPM calculation
 *
 * @param ctx GPIO context
 * @param pulses_per_rev Pulses per revolution
 * @param duration Total measurement duration
 * @param warmup Warmup duration
 * @param debug Enable debug output
 * @return double RPM value, -1.0 if interrupted, 0.0 if no pulses
 */
double gpio_measure_rpm(gpio_context_t *ctx, int pulses_per_rev, int duration, int warmup, int debug) {
    if (!ctx) return 0.0;

    int measurement_duration = duration - warmup;

    // Warmup phase (skip if warmup is 0)
    if (warmup > 0) {
        timed_loop_result_t warmup_result = timed_event_loop(ctx, warmup, NULL, debug, "Warmup");
        if (warmup_result == TIMED_LOOP_INTERRUPTED) {
            return -1.0;
        }
        if (warmup_result == TIMED_LOOP_ERROR) {
            return -1.0;
        }
    }

    // Measurement phase - run for full measurement duration
    struct timespec start_ts;
    clock_gettime(CLOCK_MONOTONIC, &start_ts);

    unsigned int count = 0;
    timed_loop_result_t measure_result = timed_event_loop(ctx, measurement_duration, &count, debug, "Measurement");

    if (measure_result == TIMED_LOOP_INTERRUPTED) {
        return -1.0;
    }
    if (measure_result == TIMED_LOOP_ERROR) {
        return -1.0;
    }

    // Get current time for elapsed calculation
    struct timespec current_ts;
    clock_gettime(CLOCK_MONOTONIC, &current_ts);

    double elapsed = (current_ts.tv_sec - start_ts.tv_sec) +
                     (current_ts.tv_nsec - start_ts.tv_nsec) / 1e9;

    if (elapsed <= 0.0) return 0.0;

    // Calculate RPM: (pulses / pulses_per_rev) / time * 60
    // This is equivalent to: frequency(Hz) * 60 / pulses_per_rev
    double revs = (double)count / pulses_per_rev;
    double rpm = revs / elapsed * 60.0;

    if (debug) {
        fprintf(stderr, "Counted %u pulses in %.3f s, RPM=%.1f\n",
                count, elapsed, rpm);
        fprintf(stderr, "  Pulses per revolution: %d\n", pulses_per_rev);
        fprintf(stderr, "  Revolutions: %.2f\n", revs);
        fprintf(stderr, "  Frequency: %.2f Hz\n", count / elapsed);
    }

    return rpm;
}

void* gpio_thread_fn(void *arg) {
    thread_args_t *a = arg;
    gpio_context_t *ctx;
    
    // Initialize GPIO context
    ctx = gpio_init(a->gpio, a->chipname);
    if (!ctx) {
        pthread_mutex_lock(&print_mutex);
        fprintf(stderr, "Error: cannot open chip for GPIO %d\n", a->gpio);
        pthread_mutex_unlock(&print_mutex);
        free(a);
        return NULL;
    }
    
    // Request edge events (include PID for unique identification)
    char consumer[32];
    snprintf(consumer, sizeof(consumer), "gpio-fan-rpm-%d", (int)getpid());
    if (gpio_request_events(ctx, consumer, a->edge) < 0) {
        pthread_mutex_lock(&print_mutex);
        fprintf(stderr, "Error: cannot request events for GPIO %d\n", a->gpio);
        pthread_mutex_unlock(&print_mutex);
        gpio_cleanup(ctx);
        free(a);
        return NULL;
    }
    
    // Warmup once for watch mode
    if (a->watch) {
        gpio_measure_rpm(ctx, a->pulses, a->duration, a->warmup, a->debug);
    }
    
    // Measurement loop
    do {
        double rpm = gpio_measure_rpm(ctx, a->pulses, a->duration, a->warmup, a->debug);
        
        // Don't output interrupted measurements
        if (rpm < 0.0) {
            // Interrupted during measurement, exit cleanly
            break;
        }
        
        // For multiple GPIOs, store result and signal completion
        if (a->total_threads > 1 && a->results && a->finished &&
            a->results_mutex && a->all_finished) {
            pthread_mutex_lock(a->results_mutex);

            // Store result
            a->results[a->thread_index] = rpm;
            a->finished[a->thread_index] = 1;

            // Check if all threads finished and signal if so
            if (measurement_all_done(a->finished, a->total_threads)) {
                int ret = pthread_cond_signal(a->all_finished);
                if (ret != 0) {
                    fprintf(stderr, "Warning: pthread_cond_signal failed: %s\n", strerror(ret));
                }
            }

            pthread_mutex_unlock(a->results_mutex);
        } else {
            // Single GPIO - store result for main thread to output
            // This prevents double output in single measurement mode
            if (a->results && a->finished) {
                pthread_mutex_lock(a->results_mutex);
                a->results[a->thread_index] = rpm;
                a->finished[a->thread_index] = 1;
                pthread_mutex_unlock(a->results_mutex);
            } else {
                // Fallback: direct output if no synchronization primitives available
                pthread_mutex_lock(&print_mutex);
                char *output = format_output(a->gpio, rpm, NULL, a->mode, a->duration);
                if (output) {
                    printf("%s", output);
                    free(output);
                }
                fflush(stdout);
                pthread_mutex_unlock(&print_mutex);
            }
        }
        
        // For single measurement mode, only run once
        if (!a->watch) {
            break;
        }
        
    } while (a->watch && !stop);
    
    gpio_cleanup(ctx);
    free(a);

    return NULL;
}
