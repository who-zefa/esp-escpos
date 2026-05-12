#pragma once

#include "esp_err.h"
#include "escpos_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief USB transport configuration
 */
typedef struct {
    uint16_t vid;                   /*!< Vendor ID  – 0x0000 = match any */
    uint16_t pid;                   /*!< Product ID – 0x0000 = match any */
    uint16_t pid_alt;               /*!< Optional secondary PID (e.g. dual-mode adapters) */
    uint32_t connect_timeout_ms;    /*!< How long to wait for the device to enumerate */
    uint16_t out_buffer_size;       /*!< TX ring buffer size */
    uint16_t in_buffer_size;        /*!< RX ring buffer size (printer rarely sends data) */
    uint8_t  interface_idx;         /*!< CDC interface number, usually 0 */
    bool     set_line_coding;       /*!< Send CDC SetLineCoding before printing */
    uint32_t baud_rate;             /*!< Baud rate for SetLineCoding (default 9600) */
} escpos_usb_config_t;

/**
 * @brief Opaque USB transport context
 */
typedef struct escpos_usb_ctx_s escpos_usb_ctx_t;

/**
 * @brief Install the USB Host + CDC-ACM driver stack.
 *
 * Call once at startup before creating any printer handles.
 * Safe to call multiple times – subsequent calls are no-ops.
 *
 * @return ESP_OK on success
 */
esp_err_t escpos_usb_driver_install(void);

/**
 * @brief Uninstall the USB Host + CDC-ACM driver stack.
 *
 * Call only when all USB printer handles have been destroyed.
 */
esp_err_t escpos_usb_driver_uninstall(void);

/**
 * @brief Open a USB CDC printer and return a write-context.
 *
 * Blocks until a matching device is found or the timeout expires.
 *
 * @param[in]  cfg  USB configuration
 * @param[out] ctx  Allocated context on success (free with escpos_usb_close)
 * @return ESP_OK | ESP_ERR_TIMEOUT | ESP_ERR_NOT_FOUND | ESP_ERR_NO_MEM
 */
esp_err_t escpos_usb_open(const escpos_usb_config_t *cfg, escpos_usb_ctx_t **ctx);

/**
 * @brief Close the USB printer connection and free resources.
 *
 * @param ctx  Context returned by escpos_usb_open
 */
void escpos_usb_close(escpos_usb_ctx_t *ctx);

/**
 * @brief Blocking write to the USB printer.
 *
 * This is the escpos_write_fn_t implementation for USB.
 *
 * @param data  Bytes to send
 * @param len   Number of bytes
 * @param ctx   escpos_usb_ctx_t pointer
 * @return ESP_OK | ESP_ERR_TIMEOUT | ESP_ERR_INVALID_STATE
 */
esp_err_t escpos_usb_write(const uint8_t *data, size_t len, void *ctx);

/**
 * @brief Return true if the USB device is still connected.
 */
bool escpos_usb_is_connected(const escpos_usb_ctx_t *ctx);

/**
 * @brief Block until the USB device disconnects (or timeout).
 *
 * @param ctx         Context
 * @param timeout_ms  portMAX_DELAY for infinite wait
 * @return ESP_OK (disconnected) | ESP_ERR_TIMEOUT
 */
esp_err_t escpos_usb_wait_disconnect(escpos_usb_ctx_t *ctx, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif