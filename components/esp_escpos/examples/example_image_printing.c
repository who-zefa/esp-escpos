/**
 * @file example_image_printing.c
 * @brief Example: Image Printing with ESP32 ESC/POS Library
 * 
 * Demonstrates how to:
 * 1. Load a BMP image from file or memory
 * 2. Process it to monochrome bitmap with dithering
 * 3. Print it to an ESC/POS thermal printer
 */

#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "escpos.h"

static const char *TAG = "IMAGE_EXAMPLE";

/**
 * ============================================================================
 * EXAMPLE 1: Print image from file
 * ============================================================================
 */
void example_print_image_from_file(escpos_printer_t *printer)
{
    ESP_LOGI(TAG, "=== Example 1: Print Image from File ===");

    /* Get default parameters */
    escpos_image_params_t params = escpos_image_get_default_params();
    
    /* Customize if needed */
    params.dither_mode = ESCPOS_DITHER_FLOYD_STEINBERG; /* Better quality dithering */
    params.max_width = 384;  /* Standard 58mm thermal printer width */
    params.auto_scale = true;

    /* Load image from file */
    escpos_image_t image = {0};
    esp_err_t err = escpos_image_load_from_file(
        "/sdcard/logo.bmp",  /* Path to BMP file */
        &params,
        &image
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load image: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Image loaded: %dx%d pixels, %zu bytes", 
             image.width, image.height, image.bitmap_size);

    /* Print the image to thermal printer */
    err = escpos_print_image(printer, &image, ESCPOS_IMAGE_MODE_NORMAL);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to print image: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Image printed successfully!");
    }

    /* Free image resources */
    escpos_image_free(&image);

    /* Feed paper after image */
    escpos_feed_lines(printer, 3);
}

/**
 * ============================================================================
 * EXAMPLE 2: Print image with double-width (larger)
 * ============================================================================
 */
void example_print_image_double_width(escpos_printer_t *printer)
{
    ESP_LOGI(TAG, "=== Example 2: Print Image (Double Width) ===");

    escpos_image_params_t params = escpos_image_get_default_params();
    escpos_image_t image = {0};

    esp_err_t err = escpos_image_load_from_file("/sdcard/qrcode.bmp", &params, &image);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load image: %s", esp_err_to_name(err));
        return;
    }

    /* Print with double-width mode */
    err = escpos_print_image(printer, &image, ESCPOS_IMAGE_MODE_DOUBLE_W);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to print image: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Double-width image printed!");
    }

    escpos_image_free(&image);
    escpos_feed_lines(printer, 3);
}

/**
 * ============================================================================
 * EXAMPLE 3: Print image from memory buffer
 * ============================================================================
 */
void example_print_image_from_buffer(escpos_printer_t *printer, 
                                     const uint8_t *image_data, 
                                     size_t image_size)
{
    ESP_LOGI(TAG, "=== Example 3: Print Image from Memory ===");

    escpos_image_params_t params = escpos_image_get_default_params();
    params.dither_mode = ESCPOS_DITHER_THRESHOLD; /* Simple threshold */
    
    escpos_image_t image = {0};
    esp_err_t err = escpos_image_load_from_buffer(
        image_data,
        image_size,
        &params,
        &image
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load image from buffer: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Image loaded from memory: %dx%d", image.width, image.height);

    err = escpos_print_image(printer, &image, ESCPOS_IMAGE_MODE_NORMAL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to print: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Image from memory printed!");
    }

    escpos_image_free(&image);
    escpos_feed_lines(printer, 3);
}

/**
 * ============================================================================
 * EXAMPLE 4: Complete print cycle with text and image
 * ============================================================================
 */
void example_complete_receipt(escpos_printer_t *printer)
{
    ESP_LOGI(TAG, "=== Example 4: Complete Receipt with Image ===");

    /* Initialize printer */
    escpos_init_printer(printer);

    /* Print header text */
    escpos_write_text(printer, "STORE RECEIPT\n");
    escpos_feed_line(printer);

    /* Print logo image */
    escpos_image_params_t params = escpos_image_get_default_params();
    escpos_image_t logo = {0};
    
    if (escpos_image_load_from_file("/sdcard/logo.bmp", &params, &logo) == ESP_OK) {
        escpos_print_image(printer, &logo, ESCPOS_IMAGE_MODE_NORMAL);
        escpos_image_free(&logo);
        escpos_feed_lines(printer, 2);
    }

    /* Print receipt details */
    escpos_write_text(printer, "Product 1    $10.00\n");
    escpos_write_text(printer, "Product 2    $15.50\n");
    escpos_write_text(printer, "================\n");
    escpos_write_text(printer, "Total        $25.50\n");
    escpos_feed_lines(printer, 2);

    /* Print QR code or barcode image */
    escpos_image_t qrcode = {0};
    if (escpos_image_load_from_file("/sdcard/qrcode.bmp", &params, &qrcode) == ESP_OK) {
        escpos_print_image(printer, &qrcode, ESCPOS_IMAGE_MODE_NORMAL);
        escpos_image_free(&qrcode);
        escpos_feed_lines(printer, 2);
    }

    /* Print footer and cut paper */
    escpos_write_text(printer, "Thank you!\n");
    escpos_feed_lines(printer, 3);
    escpos_cut_paper(printer);
}

/**
 * ============================================================================
 * MAIN APPLICATION
 * ============================================================================
 */
void image_printing_demo_task(void *arg)
{
    ESP_LOGI(TAG, "Starting image printing demo");

    /* Initialize ESC/POS library */
    ESP_ERROR_CHECK(escpos_init());

    while (1) {
        ESP_LOGI(TAG, "Waiting for printer connection...");

        escpos_printer_t *printer = NULL;
        esp_err_t err = escpos_new_usb(&printer);

        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Printer not found, retrying...");
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        ESP_LOGI(TAG, "Printer connected!");
        vTaskDelay(pdMS_TO_TICKS(500));

        /* Run examples */
        example_print_image_from_file(printer);
        vTaskDelay(pdMS_TO_TICKS(1000));

        example_print_image_double_width(printer);
        vTaskDelay(pdMS_TO_TICKS(1000));

        example_complete_receipt(printer);

        /* Wait for printer disconnect */
        ESP_LOGI(TAG, "Waiting for printer disconnect...");
        escpos_wait_disconnect(printer, pdMS_TO_TICKS(30000));

        escpos_destroy(printer);
        ESP_LOGI(TAG, "Printer disconnected");
    }

    vTaskDelete(NULL);
}

/**
 * Application entry point
 */
void app_main(void)
{
    ESP_LOGI(TAG, "Image Printing Demo Application Started");

    /* Create image printing task */
    xTaskCreatePinnedToCore(
        image_printing_demo_task,
        "image_demo",
        4096,
        NULL,
        5,
        NULL,
        0
    );
}
