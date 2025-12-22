/**
 * @file format.h
 * @brief Output formatting functions for RPM measurements
 * @author CSoellinger
 * @date 2025
 * @license LGPL-3.0-or-later
 *
 * This module provides functions to format RPM measurements in various
 * output formats including human-readable, JSON, numeric, and collectd.
 */

#ifndef FORMAT_H
#define FORMAT_H

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
 * @brief Format RPM as numeric string (with newline)
 *
 * @param rpm RPM value to format
 * @return char* Formatted string (caller must free), NULL on error
 */
char* format_numeric(double rpm);

/**
 * @brief Format RPM and GPIO as JSON string (with newline)
 *
 * If stats is provided, includes min/max/avg fields.
 *
 * @param gpio GPIO number
 * @param rpm RPM value to format
 * @param stats Optional statistics (NULL for basic output)
 * @return char* Formatted JSON string (caller must free), NULL on error
 */
char* format_json(int gpio, double rpm, const rpm_stats_t *stats);

/**
 * @brief Format RPM and GPIO as collectd PUTVAL string (with newline)
 *
 * @param gpio GPIO number
 * @param rpm RPM value to format
 * @param duration Measurement duration in seconds
 * @return char* Formatted collectd string (caller must free), NULL on error
 */
char* format_collectd(int gpio, double rpm, int duration);

/**
 * @brief Format human-readable output
 *
 * If stats is provided, includes min/max/avg in parentheses.
 *
 * @param gpio GPIO number
 * @param rpm RPM value to format
 * @param stats Optional statistics (NULL for basic output)
 * @return char* Formatted string (caller must free), NULL on error
 */
char* format_human_readable(int gpio, double rpm, const rpm_stats_t *stats);

/**
 * @brief Format RPM output according to specified mode
 *
 * Helper function that dispatches to appropriate formatting function.
 *
 * @param gpio GPIO number
 * @param rpm RPM value to format
 * @param stats Optional statistics (NULL for basic output)
 * @param mode Output format mode
 * @param duration Measurement duration (used for collectd format)
 * @return char* Formatted string (caller must free), NULL on error
 */
char* format_output(int gpio, double rpm, const rpm_stats_t *stats, output_mode_t mode, int duration);

#ifdef __cplusplus
}
#endif

#endif // FORMAT_H
