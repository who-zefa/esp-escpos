/**
 * @file transport/escpos_ble.h
 * @brief BLE transport backend placeholder for esp32_escpos.
 *
 * This header reserves the API surface for a future Bluetooth Low Energy
 * Serial Port Profile (SPP-over-BLE) transport.
 *
 * @note NOT YET IMPLEMENTED. escpos_ble_transport_init() returns
 *       ESP_ERR_NOT_SUPPORTED until a future release.
 *
 * @copyright Copyright (c) 2024 esp32-escpos contributors
 * @license   MIT
 */

#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "escpos_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief BLE backend configuration (reserved). */
typedef struct {
    char     remote_addr[18]; /**< Remote MAC "AA:BB:CC:DD:EE:FF"             */
    uint16_t mtu;             /**< Preferred ATT MTU size (bytes, default 512) */
    uint32_t connect_timeout_ms; /**< BLE connection timeout (ms)             */
} escpos_ble_config_t;

#define ESCPOS_BLE_CONFIG_DEFAULT() { \
    .remote_addr        = "",          \
    .mtu                = 512,         \
    .connect_timeout_ms = 10000,       \
}

/**
 * @brief Populate a transport descriptor for the BLE backend (not implemented).
 *
 * @return @c ESP_ERR_NOT_SUPPORTED always.
 */
esp_err_t escpos_ble_transport_init(const escpos_ble_config_t *config,
                                     escpos_transport_t        *transport);

#ifdef __cplusplus
}
#endif