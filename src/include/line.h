/**
 * This module handles GPIO line operations.
 *
 * @author  CSoellinger
 * @license LGPL-3.0-or-later
 */

#ifndef LINE_H
#define LINE_H

#include <gpiod.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Edge detection type (EDGE_BOTH is default for zero-initialized structs)
 */
typedef enum {
    EDGE_BOTH = 0,   /**< Detect both rising and falling edges (default) */
    EDGE_RISING,     /**< Detect rising edges only */
    EDGE_FALLING     /**< Detect falling edges only */
} edge_type_t;

/**
 * Line request context for version compatibility
 */
typedef struct line_request {
    struct gpiod_line_request *request;
    struct gpiod_edge_event_buffer *event_buffer;  /**< Reusable event buffer */
    int event_fd;
    int gpio;
} line_request_t;

/**
 * Request line for edge events
 *
 * @param chip GPIO chip
 * @param gpio GPIO line number
 * @param consumer Consumer name
 * @param edge Edge detection type
 * @return line_request_t* Line request context or NULL on error
 */
line_request_t* line_request_events(struct gpiod_chip *chip, int gpio, const char *consumer, edge_type_t edge);

/**
 * Release line request
 *
 * @param req Line request context
 */
void line_release(line_request_t *req);

/**
 * Wait for edge event
 *
 * @param req Line request context
 * @param timeout_ns Timeout in nanoseconds
 * @return int 0 on timeout, 1 on event, -1 on error
 */
int line_wait_event(line_request_t *req, int64_t timeout_ns);

/**
 * Read edge event
 *
 * @param req Line request context
 * @return int Number of events read, 0 if none, -1 on error
 */
int line_read_event(line_request_t *req);

#ifdef __cplusplus
}
#endif

#endif // LINE_H 