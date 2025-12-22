/**
 * @file stats.h
 * @brief RPM statistics tracking for watch mode
 * @author CSoellinger
 * @date 2025
 * @license LGPL-3.0-or-later
 *
 * This module provides structures and functions for tracking
 * min/max/average RPM statistics in continuous monitoring mode.
 */

#ifndef STATS_H
#define STATS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RPM statistics structure for tracking min/max/average
 */
typedef struct {
    double min;           /**< Minimum RPM value observed */
    double max;           /**< Maximum RPM value observed */
    double sum;           /**< Sum of all RPM values (for average calculation) */
    unsigned long count;  /**< Number of measurements */
} rpm_stats_t;

/**
 * @brief Initialize statistics structure
 *
 * @param stats Pointer to statistics structure
 */
void stats_init(rpm_stats_t *stats);

/**
 * @brief Update statistics with new RPM value
 *
 * @param stats Pointer to statistics structure
 * @param rpm New RPM value to incorporate
 */
void stats_update(rpm_stats_t *stats, double rpm);

/**
 * @brief Calculate average RPM from statistics
 *
 * @param stats Pointer to statistics structure
 * @return double Average RPM, or 0.0 if no measurements
 */
double stats_avg(const rpm_stats_t *stats);

#ifdef __cplusplus
}
#endif

#endif // STATS_H
