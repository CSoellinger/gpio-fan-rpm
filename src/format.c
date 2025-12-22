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

    char tmp[COLLECTD_BUFFER_SIZE];
    int written = snprintf(tmp, sizeof(tmp),
        "PUTVAL \"%s/gpio-fan-%d/gauge-rpm\" interval=%d %ld:%.0f\n",
        host, gpio, duration, (long)now, rpm);

    if (written < 0 || written >= (int)sizeof(tmp)) {
        return NULL;
    }

    char *buf = malloc(written + 1);
    if (!buf) return NULL;

    memcpy(buf, tmp, written);
    buf[written] = '\0';
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
