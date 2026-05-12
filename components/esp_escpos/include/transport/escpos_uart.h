/**
 * @file transport/escpos_uart.h
 * @brief UART transport backend for esp32_escpos.
 *
 * @note This backend is a stub.  The factory function allocates and populates
 *       the transport descriptor but the driver is not yet fully validated.
 *       Contributions welcome.
 *
 * @copyright Copyright (c) 2024 esp32-escpos contributors
 * @license   MIT
 */

#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "driver/uart.h"
#include "escpos_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief UART backend configuration. */
typedef struct {
    uart_port_t port;        /**< UART peripheral (UART_NUM_0 … UART_NUM_2)   */
    int         tx_pin;      /**< GPIO number for TX                           */
    int         rx_pin;      /**< GPIO number for RX (-1 = TX-only)            */
    uint32_t    baud_rate;   /**< Baud rate, e.g. 9600 or 115200               */
    size_t      tx_buf;      /**< UART driver TX ring buffer (bytes)           */
    size_t      rx_buf;      /**< UART driver RX ring buffer (bytes, ≥ 128)   */
} escpos_uart_config_t;

#define ESCPOS_UART_CONFIG_DEFAULT() { \
    .port      = UART_NUM_1,            \
    .tx_pin    = 17,                    \
    .rx_pin    = 16,                    \
    .baud_rate = 9600,                  \
    .tx_buf    = 1024,                  \
    .rx_buf    = 256,                   \
}

/**
 * @brief Populate a transport descriptor for the UART backend.
 *
 * @param[in]  config    UART configuration.
 * @param[out] transport Transport struct to fill.
 * @return @c ESP_OK on success, @c ESP_ERR_NO_MEM on allocation failure.
 */
esp_err_t escpos_uart_transport_init(const escpos_uart_config_t *config,
                                      escpos_transport_t         *transport);

/** @brief Free UART transport private context (error-path only). */
void escpos_uart_transport_deinit(escpos_transport_t *transport);

#ifdef __cplusplus
}
#endif