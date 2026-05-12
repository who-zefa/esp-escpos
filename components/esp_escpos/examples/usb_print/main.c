/**
 * @file main.c
 * @brief USB ESC/POS Printer Example
 *
 * This example demonstrates how to use the ESP32 ESC/POS library
 * with a USB thermal printer.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

#include "escpos.h"
#include "transport/escpos_usb.h"

static const char *TAG = "usb_print_example";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting USB ESC/POS printer example");

    // USB transport configuration
    escpos_usb_config_t usb_config = {
        .vid = 0x04B8,  // Example: Epson vendor ID
        .pid = 0x0202,  // Example: TM-T88V product ID
        .interface_num = 0,
        .timeout_ms = 5000
    };

    // Create USB transport
    escpos_transport_t *transport = escpos_usb_transport_create(&usb_config);
    if (transport == NULL) {
        ESP_LOGE(TAG, "Failed to create USB transport");
        return;
    }

    // Library configuration
    escpos_config_t config = ESCPOS_CONFIG_DEFAULT();

    // Initialize library
    escpos_handle_t printer;
    esp_err_t err = escpos_init(&config, transport, &printer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ESC/POS library: %s", esp_err_to_name(err));
        escpos_usb_transport_destroy(transport);
        return;
    }

    ESP_LOGI(TAG, "Printer initialized successfully");

    // Print some text
    escpos_style_t style = ESCPOS_STYLE_DEFAULT();
    style.bold = true;
    style.align = ESCPOS_ALIGN_CENTER;

    err = escpos_print(printer, "ESP32 ESC/POS Demo\n", &style);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to print text: %s", esp_err_to_name(err));
    }

    // Print normal text
    style = ESCPOS_STYLE_DEFAULT();
    err = escpos_print(printer, "Hello from ESP32!\n", &style);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to print text: %s", esp_err_to_name(err));
    }

    // Feed and cut
    err = escpos_feed(printer, 3);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to feed paper: %s", esp_err_to_name(err));
    }

    err = escpos_cut(printer, ESCPOS_CUT_FULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to cut paper: %s", esp_err_to_name(err));
    }

    // Cleanup
    err = escpos_deinit(printer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to deinitialize printer: %s", esp_err_to_name(err));
    }

    escpos_usb_transport_destroy(transport);

    ESP_LOGI(TAG, "Example completed");
}