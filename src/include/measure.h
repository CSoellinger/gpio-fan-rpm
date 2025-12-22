/**
 * This module handles single measurement mode with parallel
 * measurement and ordered output for multiple GPIO pins.
 *
 * @author  CSoellinger
 * @license LGPL-3.0-or-later
 */

#ifndef MEASURE_H
#define MEASURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "gpio.h"

/**
 * Run single measurement mode for multiple GPIO pins
 *
 * @param gpios Array of GPIO numbers
 * @param ngpio Number of GPIOs
 * @param chipname GPIO chip name
 * @param duration Measurement duration
 * @param pulses Pulses per revolution
 * @param warmup Warmup duration
 * @param edge Edge detection type
 * @param debug Debug flag
 * @param mode Output mode
 * @return int 0 on success, -1 on error
 */
int run_single_measurement(int *gpios, size_t ngpio, char *chipname,
                          int duration, int pulses, int warmup, edge_type_t edge, int debug, output_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* MEASURE_H */ 