/**
 * This module provides functions to format RPM measurements in various
 * output formats including human-readable, JSON, numeric, and collectd.
 * 
 * @author  CSoellinger
 * @license LGPL-3.0-or-later
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include "format.h"

// Buffer size constants
#define NUMERIC_BUFFER_SIZE 32
#define HUMAN_BUFFER_SIZE 128
#define JSON_BUFFER_SIZE 128
#define HOSTNAME_BUFFER_SIZE 256
#define COLLECTD_BUFFER_SIZE 512

char* format_numeric(double rpm) {
    char *buf = malloc(NUMERIC_BUFFER_SIZE);
    if (!buf) return NULL;

    if (snprintf(buf, NUMERIC_BUFFER_SIZE, "%.0f\n", rpm) >= NUMERIC_BUFFER_SIZE) {
        free(buf);
        return NULL;
    }

    return buf;
}

char* format_json(int gpio, double rpm, const rpm_stats_t *stats) {
    char *buf = malloc(JSON_BUFFER_SIZE);
    if (!buf) return NULL;

    int written;
    if (stats) {
        double avg = stats_avg(stats);
        written = snprintf(buf, JSON_BUFFER_SIZE,
            "{\"gpio\":%d,\"rpm\":%d,\"min\":%d,\"max\":%d,\"avg\":%d}\n",
            gpio, (int)round(rpm), (int)round(stats->min),
            (int)round(stats->max), (int)round(avg));
    } else {
        written = snprintf(buf, JSON_BUFFER_SIZE,
            "{\"gpio\":%d,\"rpm\":%d}\n", gpio, (int)round(rpm));
    }

    if (written < 0 || written >= JSON_BUFFER_SIZE) {
        free(buf);
        return NULL;
    }

    return buf;
}

char* format_collectd(int gpio, double rpm, int duration) {
    char host[HOSTNAME_BUFFER_SIZE] = {0};

    if (gethostname(host, sizeof(host) - 1) < 0) {
        strncpy(host, "unknown", sizeof(host) - 1);
        host[sizeof(host) - 1] = '\0';
    }

    time_t now = time(NULL);

    char *buf = malloc(COLLECTD_BUFFER_SIZE);
    if (!buf) return NULL;

    int written = snprintf(buf, COLLECTD_BUFFER_SIZE,
        "PUTVAL \"%s/gpio-fan-%d/gauge-rpm\" interval=%d %ld:%.0f\n",
        host, gpio, duration, (long)now, rpm);

    if (written < 0 || written >= COLLECTD_BUFFER_SIZE) {
        free(buf);
        return NULL;
    }

    return buf;
}

char* format_human_readable(int gpio, double rpm, const rpm_stats_t *stats) {
    char *buf = malloc(HUMAN_BUFFER_SIZE);
    if (!buf) return NULL;

    int written;
    if (stats) {
        double avg = stats_avg(stats);
        written = snprintf(buf, HUMAN_BUFFER_SIZE,
            "GPIO%d: RPM: %.0f (min: %.0f, max: %.0f, avg: %.0f)\n",
            gpio, rpm, stats->min, stats->max, avg);
    } else {
        written = snprintf(buf, HUMAN_BUFFER_SIZE,
            "GPIO%d: RPM: %.0f\n", gpio, rpm);
    }

    if (written < 0 || written >= HUMAN_BUFFER_SIZE) {
        free(buf);
        return NULL;
    }

    return buf;
}

char* format_output(int gpio, double rpm, const rpm_stats_t *stats, output_mode_t mode, int duration) {
    switch (mode) {
        case MODE_NUMERIC:
            return format_numeric(rpm);
        case MODE_JSON:
            return format_json(gpio, rpm, stats);
        case MODE_COLLECTD:
            return format_collectd(gpio, rpm, duration);
        case MODE_DEFAULT:
        default:
            return format_human_readable(gpio, rpm, stats);
    }
}

char* format_json_array(const int *gpios, const double *results, const rpm_stats_t *stats, size_t ngpio) {
    if (!gpios || !results || ngpio == 0) return NULL;

    // Estimate buffer size: ~80 chars per entry with stats, ~40 without
    size_t buf_size = (stats ? 80 : 40) * ngpio + 16;
    char *buf = malloc(buf_size);
    if (!buf) return NULL;

    size_t pos = 0;
    buf[pos++] = '[';

    int first = 1;
    for (size_t i = 0; i < ngpio; i++) {
        // Skip interrupted measurements (negative values indicate interruption)
        if (results[i] < 0.0) continue;

        if (!first) {
            buf[pos++] = ',';
        }
        first = 0;

        int written;
        if (stats) {
            double avg = stats_avg(&stats[i]);
            written = snprintf(buf + pos, buf_size - pos,
                "{\"gpio\":%d,\"rpm\":%d,\"min\":%d,\"max\":%d,\"avg\":%d}",
                gpios[i], (int)round(results[i]),
                (int)round(stats[i].min), (int)round(stats[i].max),
                (int)round(avg));
        } else {
            written = snprintf(buf + pos, buf_size - pos,
                "{\"gpio\":%d,\"rpm\":%d}",
                gpios[i], (int)round(results[i]));
        }

        if (written < 0 || (size_t)written >= buf_size - pos) {
            free(buf);
            return NULL;
        }
        pos += written;
    }

    if (pos + 2 >= buf_size) {
        free(buf);
        return NULL;
    }
    buf[pos++] = ']';
    buf[pos++] = '\n';
    buf[pos] = '\0';

    return buf;
}
