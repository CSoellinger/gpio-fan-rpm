/**
 * This module implements GPIO line operations
 * 
 * @author  CSoellinger
 * @license LGPL-3.0-or-later
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include "line.h"

line_request_t* line_request_events(struct gpiod_chip *chip, int gpio, const char *consumer, edge_type_t edge) {
    if (!chip || !consumer) return NULL;

    line_request_t *req = calloc(1, sizeof(*req));
    if (!req) return NULL;

    req->gpio = gpio;

    // Create request configuration
    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    if (!req_cfg) {
        free(req);
        return NULL;
    }

    gpiod_request_config_set_consumer(req_cfg, consumer);

    // Create line configuration
    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    if (!line_cfg) {
        gpiod_request_config_free(req_cfg);
        free(req);
        return NULL;
    }

    // Configure line for edge detection
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    if (!settings) {
        gpiod_line_config_free(line_cfg);
        gpiod_request_config_free(req_cfg);
        free(req);
        return NULL;
    }

    // Map edge_type_t to libgpiod edge constant
    enum gpiod_line_edge gpiod_edge;
    switch (edge) {
        case EDGE_RISING:
            gpiod_edge = GPIOD_LINE_EDGE_RISING;
            break;
        case EDGE_FALLING:
            gpiod_edge = GPIOD_LINE_EDGE_FALLING;
            break;
        case EDGE_BOTH:
        default:
            gpiod_edge = GPIOD_LINE_EDGE_BOTH;
            break;
    }

    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_edge_detection(settings, gpiod_edge);
    // Note: Clock setting is not needed for basic edge detection
    
    // Add line to configuration
    unsigned int gpio_offset = (unsigned int)gpio;
    if (gpiod_line_config_add_line_settings(line_cfg, &gpio_offset, 1, settings) < 0) {
        gpiod_line_settings_free(settings);
        gpiod_line_config_free(line_cfg);
        gpiod_request_config_free(req_cfg);
        free(req);
        return NULL;
    }
    
    // Request the line
    req->request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
    if (!req->request) {
        gpiod_line_settings_free(settings);
        gpiod_line_config_free(line_cfg);
        gpiod_request_config_free(req_cfg);
        free(req);
        return NULL;
    }
    
    // Get event file descriptor
    req->event_fd = gpiod_line_request_get_fd(req->request);

    // Allocate reusable event buffer
    req->event_buffer = gpiod_edge_event_buffer_new(1);
    if (!req->event_buffer) {
        gpiod_line_request_release(req->request);
        gpiod_line_settings_free(settings);
        gpiod_line_config_free(line_cfg);
        gpiod_request_config_free(req_cfg);
        free(req);
        return NULL;
    }

    // Clean up configuration objects
    gpiod_line_settings_free(settings);
    gpiod_line_config_free(line_cfg);
    gpiod_request_config_free(req_cfg);

    return req;
}

void line_release(line_request_t *req) {
    if (!req) return;

    if (req->event_buffer) {
        gpiod_edge_event_buffer_free(req->event_buffer);
    }

    if (req->request) {
        gpiod_line_request_release(req->request);
    }

    free(req);
}

int line_wait_event(line_request_t *req, int64_t timeout_ns) {
    if (!req) return -1;
    
    if (req->event_fd < 0) return -1;
    
    struct pollfd pfd;
    pfd.fd = req->event_fd;
    pfd.events = POLLIN;
    
    int timeout_ms = (timeout_ns >= 0) ? (timeout_ns / 1000000LL) : -1;
    int ret = poll(&pfd, 1, timeout_ms);
    
    if (ret < 0) return -1;  // Error
    if (ret == 0) return 0;  // Timeout
    return 1;  // Event available
}

int line_read_event(line_request_t *req) {
    if (!req) return -1;

    if (!req->request || !req->event_buffer) return -1;

    int ret = gpiod_line_request_read_edge_events(req->request, req->event_buffer, 1);
    // Note: We don't need to process the event details, just count it
    // The buffer is reused across calls for efficiency
    return ret;
}
