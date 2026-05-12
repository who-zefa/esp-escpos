/**
 * @file transport/escpos_network.h
 * @brief TCP/IP network transport backend for esp32_escpos.
 *
 * Connects to a network-attached printer via a raw TCP socket on port 9100
 * (standard ESC/POS network port).
 *
 * @note Stub implementation — fully functional TCP socket write is provided
 *       but error recovery and reconnect are not yet implemented.
 *
 * @copyright Copyright (c) 2024 esp32-escpos contributors
 * @license   MIT
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "escpos_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief TCP network backend configuration. */
typedef struct {
    char     host[64];           /**< Printer hostname or IPv4 dotted string  */
    uint16_t port;               /**< TCP port (ESC/POS standard = 9100)      */
    uint32_t connect_timeout_ms; /**< Socket connect timeout (ms)             */
    uint32_t send_timeout_ms;    /**< SO_SNDTIMEO per write call (ms)         */
    bool     keepalive;          /**< Enable TCP keepalive probes              */
} escpos_network_config_t;

#define ESCPOS_NETWORK_CONFIG_DEFAULT() { \
    .host               = "192.168.1.100",\
    .port               = 9100,            \
    .connect_timeout_ms = 5000,            \
    .send_timeout_ms    = 3000,            \
    .keepalive          = true,            \
}

/**
 * @brief Populate a transport descriptor for the TCP backend.
 *
 * @param[in]  config    Network configuration.
 * @param[out] transport Transport struct to fill.
 * @return @c ESP_OK on success.
 */
esp_err_t escpos_network_transport_init(const escpos_network_config_t *config,
                                         escpos_transport_t            *transport);

/** @brief Free TCP transport private context (error-path only). */
void escpos_network_transport_deinit(escpos_transport_t *transport);

#ifdef __cplusplus
}
#endif