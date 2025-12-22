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
 * Print usage information
 *
 * @param prog Program name
 */
void print_usage(const char *prog);

/**
 * Parse command-line arguments
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @param gpios Output array for GPIO numbers
 * @param ngpio Output number of GPIOs
 * @param chipname Output GPIO chip name
 * @param duration Output measurement duration
 * @param pulses Output pulses per revolution
 * @param warmup Output warmup duration
 * @param edge Output edge detection type
 * @param debug Output debug flag
 * @param watch Output watch flag
 * @param mode Output format mode
 * @return int 0 on success, -1 on error, 1 for help/version
 */
int parse_arguments(int argc, char **argv, int **gpios, size_t *ngpio,
                   char **chipname, int *duration, int *pulses, int *warmup,
                   edge_type_t *edge, int *debug, int *watch, output_mode_t *mode);

/**
 * Validate parsed arguments
 *
 * @param gpios Array of GPIO numbers
 * @param ngpio Number of GPIOs
 * @param duration Measurement duration
 * @param warmup Warmup duration
 * @param prog Program name for error messages
 * @return int 0 on success, -1 on error
 */
int validate_arguments(int *gpios, size_t ngpio, int duration, int warmup, const char *prog);

#ifdef __cplusplus
}
#endif

#endif /* ARGS_H */ 