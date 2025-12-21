/**
 * @file format.c
 * @brief Output formatting functions for RPM measurements
 * @author CSoellinger
 * @date 2025
 * @license LGPL-3.0-or-later
 * 
 * This module provides functions to format RPM measurements in various
 * output formats including human-readable, JSON, numeric, and collectd.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include "format.h"

// Note: json-c dependency removed - simple JSON now uses snprintf directly

// Buffer size constants
#define NUMERIC_BUFFER_SIZE 32
#define HUMAN_BUFFER_SIZE 64
#define JSON_BUFFER_SIZE 64
#define HOSTNAME_BUFFER_SIZE 256
#define COLLECTD_BUFFER_SIZE 512

/**
 * @brief Format RPM as numeric string (with newline)
 * 
 * @param rpm RPM value to format
 * @return char* Formatted string (caller must free), NULL on error
 */
char* format_numeric(double rpm) {
    char *buf = malloc(NUMERIC_BUFFER_SIZE);
    if (!buf) return NULL;
    
    if (snprintf(buf, NUMERIC_BUFFER_SIZE, "%.0f\n", rpm) >= NUMERIC_BUFFER_SIZE) {
        free(buf);
        return NULL;
    }
    
    return buf;
}

/**
 * @brief Format RPM and GPIO as JSON string (with newline)
 *
 * Uses simple snprintf for efficiency - avoids json-c object overhead
 * which is significant in watch mode with repeated measurements.
 *
 * @param gpio GPIO number
 * @param rpm RPM value to format
 * @return char* Formatted JSON string (caller must free), NULL on error
 */
char* format_json(int gpio, double rpm) {
    char *buf = malloc(JSON_BUFFER_SIZE);
    if (!buf) return NULL;

    int written = snprintf(buf, JSON_BUFFER_SIZE,
                          "{\"gpio\":%d,\"rpm\":%d}\n", gpio, (int)round(rpm));
    if (written < 0 || written >= JSON_BUFFER_SIZE) {
        free(buf);
        return NULL;
    }

    return buf;
}



/**
 * @brief Format RPM and GPIO as collectd PUTVAL string (with newline)
 * 
 * @param gpio GPIO number
 * @param rpm RPM value to format
 * @param duration Measurement duration in seconds
 * @return char* Formatted collectd string (caller must free), NULL on error
 */
char* format_collectd(int gpio, double rpm, int duration) {
    char host[HOSTNAME_BUFFER_SIZE] = {0};

    // Get hostname, use "unknown" as fallback
    if (gethostname(host, sizeof(host) - 1) < 0) {
        strncpy(host, "unknown", sizeof(host) - 1);
        host[sizeof(host) - 1] = '\0'; // Ensure null termination
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

/**
 * @brief Format human-readable output
 * 
 * @param gpio GPIO number
 * @param rpm RPM value to format
 * @return char* Formatted string (caller must free), NULL on error
 */
char* format_human_readable(int gpio, double rpm) {
    char *buf = malloc(HUMAN_BUFFER_SIZE);
    if (!buf) return NULL;

    if (snprintf(buf, HUMAN_BUFFER_SIZE, "GPIO%d: RPM: %.0f\n", gpio, rpm) >= HUMAN_BUFFER_SIZE) {
        free(buf);
        return NULL;
    }

    return buf;
}

/**
 * @brief Format RPM output according to specified mode
 *
 * @param gpio GPIO number
 * @param rpm RPM value to format
 * @param mode Output format mode
 * @param duration Measurement duration (used for collectd format)
 * @return char* Formatted string (caller must free), NULL on error
 */
char* format_output(int gpio, double rpm, output_mode_t mode, int duration) {
    switch (mode) {
    case MODE_NUMERIC:
        return format_numeric(rpm);
    case MODE_JSON:
        return format_json(gpio, rpm);
    case MODE_COLLECTD:
        return format_collectd(gpio, rpm, duration);
    case MODE_DEFAULT:
    default:
        return format_human_readable(gpio, rpm);
    }
} 