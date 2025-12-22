/**
 * This module provides functions for tracking min/max/average
 * RPM statistics in continuous monitoring mode.
 * 
 * @author  CSoellinger
 * @license LGPL-3.0-or-later
 */

#include "stats.h"

void stats_init(rpm_stats_t *stats) {
    if (!stats) return;
    stats->min = 0.0;
    stats->max = 0.0;
    stats->sum = 0.0;
    stats->count = 0;
}

void stats_update(rpm_stats_t *stats, double rpm) {
    if (!stats) return;

    if (stats->count == 0) {
        stats->min = rpm;
        stats->max = rpm;
    } else {
        if (rpm < stats->min) stats->min = rpm;
        if (rpm > stats->max) stats->max = rpm;
    }
    stats->sum += rpm;
    stats->count++;
}

double stats_avg(const rpm_stats_t *stats) {
    if (!stats || stats->count == 0) return 0.0;
    return stats->sum / (double)stats->count;
}
