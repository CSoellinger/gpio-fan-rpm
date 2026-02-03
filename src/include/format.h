/**
 * This module provides functions to format RPM measurements in various
 * output formats including human-readable, JSON, numeric, and collectd.
 *
 * @author  CSoellinger
 * @license LGPL-3.0-or-later
 */

#ifndef FORMAT_H
#define FORMAT_H

#include <stddef.h>
#include "stats.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MODE_DEFAULT,
    MODE_NUMERIC,
    MODE_JSON,
    MODE_COLLECTD
} output_mode_t;

/**
 * Format RPM as numeric string
 *
 * @param rpm RPM value to format
 * @return char* Formatted string (caller must free), NULL on error
 */
char* format_numeric(double rpm);

/**
 * Format RPM and GPIO as JSON string
 *
 * @param gpio GPIO number
 * @param rpm RPM value to format
 * @param stats Optional statistics (NULL for basic output)
 * @return char* Formatted string (caller must free), NULL on error
 */
char* format_json(int gpio, double rpm, const rpm_stats_t *stats);

/**
 * Format RPM and GPIO as collectd PUTVAL string
 *
 * @param gpio GPIO number
 * @param rpm RPM value to format
 * @param duration Measurement duration
 * @return char* Formatted string (caller must free), NULL on error
 */
char* format_collectd(int gpio, double rpm, int duration);

/**
 * Format human-readable output
 *
 * @param gpio GPIO number
 * @param rpm RPM value to format
 * @param stats Optional statistics (NULL for basic output)
 * @return char* Formatted string (caller must free), NULL on error
 */
char* format_human_readable(int gpio, double rpm, const rpm_stats_t *stats);

/**
 * Format RPM output according to specified mode
 *
 * @param gpio GPIO number
 * @param rpm RPM value to format
 * @param stats Optional statistics (NULL for basic output)
 * @param mode Output format mode
 * @param duration Measurement duration (for collectd)
 * @return char* Formatted string (caller must free), NULL on error
 */
char* format_output(int gpio, double rpm, const rpm_stats_t *stats, output_mode_t mode, int duration);

/**
 * Format multiple GPIOs as JSON array
 *
 * @param gpios Array of GPIO numbers
 * @param results Array of RPM values
 * @param stats Optional array of statistics (NULL for basic output)
 * @param ngpio Number of GPIOs
 * @return char* Formatted JSON array string (caller must free), NULL on error
 */
char* format_json_array(const int *gpios, const double *results, const rpm_stats_t *stats, size_t ngpio);

#ifdef __cplusplus
}
#endif

#endif // FORMAT_H
