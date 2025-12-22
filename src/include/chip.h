/**
 * This module handles GPIO chip operations.
 *
 * @author  CSoellinger
 * @license LGPL-3.0-or-later
 */

#ifndef CHIP_H
#define CHIP_H

#include <gpiod.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open GPIO chip by name
 *
 * @param name Chip name (e.g., "gpiochip0")
 * @return struct gpiod_chip* Chip object or NULL on error
 */
struct gpiod_chip* chip_open_by_name(const char *name);

/**
 * Close GPIO chip
 *
 * @param chip Chip to close
 */
void chip_close(struct gpiod_chip *chip);

/**
 * Auto-detect available GPIO chip for given line
 *
 * @param gpio GPIO line number
 * @param chipname_out Output parameter for chip name (caller must free)
 * @return struct gpiod_chip* Chip object or NULL on error
 */
struct gpiod_chip* chip_auto_detect(int gpio, char **chipname_out);

/**
 * Auto-detect GPIO chip and return only the name
 *
 * @param gpio GPIO line number
 * @param chipname_out Output parameter for chip name (caller must free)
 * @return int 0 on success, -1 on error
 */
int chip_auto_detect_for_name(int gpio, char **chipname_out);

/**
 * Get chip information
 *
 * @param chip Chip object
 * @return struct gpiod_chip_info* Chip info or NULL on error
 */
struct gpiod_chip_info* chip_get_info(struct gpiod_chip *chip);

/**
 * Get number of lines on chip
 *
 * @param chip Chip object
 * @return size_t Number of lines, 0 on error
 */
size_t chip_get_num_lines(struct gpiod_chip *chip);

/**
 * Get chip name from chip info
 *
 * @param chip Chip object
 * @return char* Allocated chip name or NULL on error (caller must free)
 */
char* chip_get_name(struct gpiod_chip *chip);

/**
 * Get chip label from chip info
 *
 * @param chip Chip object
 * @return char* Allocated chip label or NULL on error (caller must free)
 */
char* chip_get_label(struct gpiod_chip *chip);

#ifdef __cplusplus
}
#endif

#endif // CHIP_H 