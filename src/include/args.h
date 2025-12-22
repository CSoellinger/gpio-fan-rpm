/**
 * This module handles command-line argument parsing, validation,
 * and help output for the gpio-fan-rpm utility.
 *
 * @author  CSoellinger
 * @license LGPL-3.0-or-later
 */

#ifndef ARGS_H
#define ARGS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "gpio.h"
#include "line.h"  // For edge_type_t

/**
 * @brief Print usage information
 * 
 * @param prog Program name
 */
void print_usage(const char *prog);

/**
 * @brief Parse command-line arguments
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @param gpios Array to store GPIO numbers
 * @param ngpio Number of GPIOs
 * @param chipname GPIO chip name
 * @param duration Measurement duration
 * @param pulses Pulses per revolution
 * @param warmup Warmup duration in seconds
 * @param edge Edge detection type
 * @param debug Debug flag
 * @param watch Watch flag
 * @param mode Output mode
 * @return int 0 on success, -1 on error, 1 for help/version
 */
int parse_arguments(int argc, char **argv, int **gpios, size_t *ngpio,
                   char **chipname, int *duration, int *pulses, int *warmup,
                   edge_type_t *edge, int *debug, int *watch, output_mode_t *mode);

/**
 * @brief Validate parsed arguments
 *
 * @param gpios Array of GPIO numbers
 * @param ngpio Number of GPIOs
 * @param duration Measurement duration
 * @param pulses Pulses per revolution
 * @param warmup Warmup duration in seconds
 * @return int 0 on success, -1 on error
 */
int validate_arguments(int *gpios, size_t ngpio, int duration, int pulses, int warmup, const char *prog);

#ifdef __cplusplus
}
#endif

#endif /* ARGS_H */ 