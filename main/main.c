/**
 * @file main.c
 * @brief ESP32 ESC/POS USB Printer Library - Comprehensive Feature Test
 * 
 * This example demonstrates ALL features of the esp32_escpos library:
 * - Text formatting (alignment, font selection, styles)
 * - IMAGE PRINTING (new feature!) - supports BMP files
 * - Paper control (feed, cut)
 * - Printer feedback (beep)
 * 
 * IMPORTANT: Image file should be in BMP format (.bmp), not PNG
 * The library supports:
 *   - 24-bit RGB BMP files
 *   - 8-bit Grayscale BMP files
 *   - Automatic monochrome conversion
 *   - Floyd-Steinberg or Threshold dithering
 * 
 * ==============================================================================
 * LIBRARY COMPONENTS:
 * 
 * - esp_system, esp_log, esp_err       : ESP-IDF Core Libraries
 * - FreeRTOS                            : Real-time OS (task management)
 * - escpos.h                            : esp32_escpos Main API
 * - escpos_image.h                      : Image printing API (NEW!)
 * 
 * ==============================================================================
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

/* ESP-IDF Core Libraries */
#include "esp_system.h"      /* System initialization and utilities */
#include "esp_log.h"         /* Logging macros (ESP_LOGI, ESP_LOGW, ESP_LOGE) */
#include "esp_err.h"         /* Error handling (esp_err_t, ESP_ERROR_CHECK) */

/* FreeRTOS Real-Time Operating System Libraries */
#include "freertos/FreeRTOS.h"   /* FreeRTOS kernel */
#include "freertos/task.h"       /* Task management (vTaskDelay) */
#include "freertos/semphr.h"     /* Semaphores (for synchronization) */

/* ESP32 ESCPOS Thermal Printer Library - INCLUDES IMAGE API */
#include "escpos.h"          /* Main printer API */
#include "escpos_image.h"    /* Image printing API (NEW!) */

/* Logger tag for this module */
static const char *TAG = "MAIN";

/* ============================================================================
 * Embedded image data
 * ============================================================================ */

/* Image data embedded from image.bmp during build */
extern const uint8_t image_bmp_start[] asm("_binary_image_bmp_start");
extern const uint8_t image_bmp_end[] asm("_binary_image_bmp_end");

#define image_bmp_size (image_bmp_end - image_bmp_start)
 /* ============================================================================
 * Helper function: Print text with specified alignment
 * ============================================================================
 */
static void print_aligned_text(escpos_printer_t *printer, 
                              const char *text, 
                              uint8_t alignment)
{
    static const char *align_names[] = {"LEFT", "CENTER", "RIGHT"};
    ESP_LOGI(TAG, "  → Printing aligned text: %s", align_names[alignment]);
    escpos_write_text(printer, text);
    escpos_write_text(printer, "\r\n");
}

/**
 * ============================================================================
 * Feature 1: TEXT FORMATTING DEMO
 * ============================================================================
 */
static void demo_text_formatting(escpos_printer_t *printer)
{
    ESP_LOGI(TAG, "\n=== TEXT FORMATTING DEMO ===");
    
    escpos_write_text(printer, "========================\r\n");
    escpos_write_text(printer, " TEXT FORMATTING TEST\r\n");
    escpos_write_text(printer, "========================\r\n\r\n");

    /* Standard text */
    escpos_write_text(printer, "1. Standard Text:\r\n");
    escpos_write_text(printer, "Normal output from ESP32\r\n\r\n");

    /* Bold demonstration */
    escpos_write_text(printer, "2. Multiple Sizes:\r\n");
    escpos_write_text(printer, "Regular text line\r\n");
    escpos_write_text(printer, "Different widths available\r\n\r\n");

    /* Left, Center, Right alignment */
    escpos_write_text(printer, "3. Text Alignment:\r\n");
    // print_aligned_text(printer, "LEFT ALIGNED TEXT", 0);
    print_aligned_text(printer, "CENTER ALIGNED TEXT", 1);
    print_aligned_text(printer, "RIGHT ALIGNED TEXT", 2);
    escpos_write_text(printer, "\r\n");
}

/**
 * ============================================================================
 * Feature 2: IMAGE PRINTING DEMO (NEW!)
 * ============================================================================
 */
static void demo_image_printing(escpos_printer_t *printer)
{
    ESP_LOGI(TAG, "\n=== IMAGE PRINTING DEMO (NEW FEATURE!) ===");
    
    escpos_write_text(printer, "========================\r\n");
    escpos_write_text(printer, " IMAGE PRINTING TEST\r\n");
    escpos_write_text(printer, "========================\r\n\r\n");
    
    escpos_write_text(printer, "Loading embedded image...\r\n\r\n");
    
    /* Get default image parameters */
    escpos_image_params_t params = escpos_image_get_default_params();
    
    /* Use Floyd-Steinberg dithering for better quality */
    params.dither_mode = ESCPOS_DITHER_FLOYD_STEINBERG;
    // params.max_width = 384;  /* Standard 58mm thermal printer */
    params.max_width = 576;  /* Standard 80mm thermal printer */
    params.print_width = 0; /* Set 0 to keep original width, or choose a dot width */
    params.align = ESCPOS_ALIGN_CENTER;
    params.threshold = 200;   /* Lower = darker print, higher = lighter print */
    params.auto_scale = true;
    
    /* Create image structure */
    escpos_image_t image = {0};
    
    /* Load image from embedded flash memory (built into firmware) */
    ESP_LOGI(TAG, "Image size: %zu bytes", image_bmp_size);
    ESP_LOGI(TAG, "Loading embedded image from flash...");
    
    esp_err_t err = escpos_image_load_from_buffer(
        image_bmp_start,
        image_bmp_size,
        &params,
        &image
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load embedded image: %s", esp_err_to_name(err));
        
        switch(err) {
            case ESP_ERR_NOT_FOUND:
                escpos_write_text(printer, "ERROR: Image not embedded!\r\n");
                escpos_write_text(printer, "Ensure image.bmp exists in main/\r\n");
                break;
            case ESP_ERR_INVALID_ARG:
                escpos_write_text(printer, "ERROR: Image too large!\r\n");
                escpos_write_text(printer, "Max: 384x1024 px, 400KB\r\n");
                break;
            case ESP_ERR_NO_MEM:
                escpos_write_text(printer, "ERROR: Not enough memory!\r\n");
                break;
            default:
                escpos_write_text(printer, "ERROR: Invalid BMP format!\r\n");
                escpos_write_text(printer, "\r\n");
                escpos_write_text(printer, "SUPPORTED: Uncompressed BMP\r\n");
                escpos_write_text(printer, "  - 24-bit RGB\r\n");
                escpos_write_text(printer, "  - 8-bit Grayscale\r\n");
                escpos_write_text(printer, "\r\n");
                escpos_write_text(printer, "Convert with:\r\n");
                escpos_write_text(printer, "  convert image.png \\\r\n");
                escpos_write_text(printer, "    -resize 384x image.bmp\r\n");
        }
        escpos_write_text(printer, "\r\n");
        return;
    }

    ESP_LOGI(TAG, "Image loaded: %dx%d pixels", 
             image.width, image.height);
    
    char size_str[64];
    sprintf(size_str, " Image: %dx%d px\r\n\r\n", image.width, image.height);
    escpos_write_text(printer, size_str);

    /* Print image at normal size */
    ESP_LOGI(TAG, "Printing image at normal size...");
    err = escpos_print_image(printer, &image, ESCPOS_IMAGE_MODE_NORMAL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to print image: %s", esp_err_to_name(err));
        escpos_write_text(printer, "ERROR: Print failed\r\n");
    } else {
        ESP_LOGI(TAG, " Image printed successfully");
    }
    escpos_feed_lines(printer, 2);

    /* Print image at double width only if it still fits the printer head */
    if ((uint32_t)image.width * 2 <= params.max_width) {
        ESP_LOGI(TAG, "Printing image at double-width...");
        escpos_write_text(printer, "Double-width:\r\n");
        err = escpos_print_image(printer, &image, ESCPOS_IMAGE_MODE_DOUBLE_W);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to print double-width: %s", esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "Double-width printed");
        }
        escpos_feed_lines(printer, 2);
    } else {
        ESP_LOGW(TAG, "Skipping double-width image: %u dots would exceed %u-dot head",
                 (unsigned)(image.width * 2), (unsigned)params.max_width);
    }

    /* Free image resources */
    escpos_image_free(&image);
    ESP_LOGI(TAG, "Image resources freed");
    
    escpos_write_text(printer, "Done!\r\n\r\n");
}

/**
 * ============================================================================
 * Feature 3: PAPER CONTROL DEMO
 * ============================================================================
 */
static void demo_paper_control(escpos_printer_t *printer)
{
    ESP_LOGI(TAG, "\n=== PAPER CONTROL DEMO ===");
    
    escpos_write_text(printer, "========================\r\n");
    escpos_write_text(printer, " PAPER CONTROL TEST\r\n");
    escpos_write_text(printer, "========================\r\n\r\n");

    escpos_write_text(printer, "Line 1: Start of document\r\n");
    escpos_write_text(printer, "Line 2: Some content here\r\n");
    escpos_write_text(printer, "Line 3: More information\r\n\r\n");

    /* Feed specific number of lines */
    escpos_write_text(printer, "Feeding 3 blank lines...\r\n\r\n");
    escpos_feed_lines(printer, 3);
    
    escpos_write_text(printer, "After feed: Ready to cut\r\n\r\n");
}

/**
 * ============================================================================
 * Feature 4: AUDIO FEEDBACK DEMO
 * ============================================================================
 */
static void demo_audio_feedback(escpos_printer_t *printer)
{
    ESP_LOGI(TAG, "\n=== AUDIO FEEDBACK DEMO ===");
    
    escpos_write_text(printer, "========================\r\n");
    escpos_write_text(printer, " AUDIO FEEDBACK TEST\r\n");
    escpos_write_text(printer, "========================\r\n\r\n");

    escpos_write_text(printer, "Beeping printer...\r\n");
    
    /* Send beep command */
    esp_err_t err = escpos_beep(printer);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Beep command failed: %s", esp_err_to_name(err));
    }
    
    vTaskDelay(pdMS_TO_TICKS(500));
    escpos_write_text(printer, "Beep complete!\r\n\r\n");
}

/**
 * ============================================================================
 * Feature 5: COMPLETE RECEIPT EXAMPLE
 * ============================================================================
 */
static void demo_complete_receipt(escpos_printer_t *printer)
{
    ESP_LOGI(TAG, "\n=== COMPLETE RECEIPT DEMO ===");

    /* Initialize printer to defaults */
    escpos_init_printer(printer);
    vTaskDelay(pdMS_TO_TICKS(100));

    /* Receipt Header */
    escpos_write_text(printer, "================================\r\n");
    escpos_write_text(printer, "     SAMPLE STORE RECEIPT\r\n");
    escpos_write_text(printer, "================================\r\n\r\n");

    /* Try to load and print company logo */
    escpos_image_params_t img_params = escpos_image_get_default_params();
    escpos_image_t logo = {0};
    
    if (escpos_image_load_from_buffer(image_bmp_start, image_bmp_size, 
                                      &img_params, &logo) == ESP_OK) {
        ESP_LOGI(TAG, "Logo loaded, printing...");
        escpos_print_image(printer, &logo, ESCPOS_IMAGE_MODE_NORMAL);
        escpos_image_free(&logo);
        escpos_feed_lines(printer, 2);
    }

    /* Receipt Date/Time */
    escpos_write_text(printer, "Date: May 18, 2026\r\n");
    escpos_write_text(printer, "Time: 14:30:00\r\n");
    escpos_write_text(printer, "Transaction ID: TXN12345\r\n\r\n");

    /* Receipt Items */
    escpos_write_text(printer, "================+++++++++++++================\r\n");
    escpos_write_text(printer, " ITEMS\r\n");
    escpos_write_text(printer, "================================\r\n");
    escpos_write_text(printer, "Item 1         x1  $10.00\r\n");
    escpos_write_text(printer, "Item 2         x2  $15.50\r\n");
    escpos_write_text(printer, "Item 3         x1  $8.99\r\n\r\n");

    /* Subtotal, Tax, Total */
    escpos_write_text(printer, "Subtotal:          $34.49\r\n");
    escpos_write_text(printer, "Tax (10%):         $3.45\r\n");
    escpos_write_text(printer, "================================\r\n");
    escpos_write_text(printer, "TOTAL:             $37.94\r\n");
    escpos_write_text(printer, "================================\r\n\r\n");

    /* Payment Info */
    escpos_write_text(printer, "Payment: Card\r\n");
    escpos_write_text(printer, "Card: VISA ****1234\r\n");
    escpos_write_text(printer, "Auth Code: A12345B\r\n\r\n");

    /* QR Code (try to load) */
    escpos_image_t qrcode = {0};
    if (escpos_image_load_from_buffer(image_bmp_start, image_bmp_size,
                                      &img_params, &qrcode) == ESP_OK) {
        escpos_write_text(printer, "Scan for details:\r\n");
        escpos_print_image(printer, &qrcode, ESCPOS_IMAGE_MODE_NORMAL);
        escpos_image_free(&qrcode);
        escpos_feed_lines(printer, 1);
    }

    /* Footer */
    escpos_write_text(printer, "\r\nThank you for your purchase!\r\n");
    escpos_write_text(printer, "Visit us again!\r\n\r\n");

    /* Final spacing before cut */
    escpos_feed_lines(printer, 3);
}

/**
 * ================================================================================
 * MAIN APPLICATION ENTRY POINT
 * 
 * FreeRTOS calls this function after system initialization.
 * This demonstrates all library features: text, images, paper control, audio
 * ================================================================================
 */
void app_main(void)
{
    ESP_LOGI(TAG, "\n\n========================================");
    ESP_LOGI(TAG, "  ESP32-S3 ESC/POS USB PRINTER DEMO");
    ESP_LOGI(TAG, "  Testing ALL Library Features");
    ESP_LOGI(TAG, "========================================\n");
    
    /* ─────────────────────────────────────────────────────────────────────────
     * INITIALIZATION PHASE
     * ─────────────────────────────────────────────────────────────────────────*/
    
    ESP_LOGI(TAG, "Step 1/2: Initializing ESC/POS Library");
    ESP_ERROR_CHECK(escpos_init());
    ESP_LOGI(TAG, " Library initialized");

    /* ─────────────────────────────────────────────────────────────────────────
     * MAIN LOOP: Search for printer, print all features, wait for disconnect
     * ─────────────────────────────────────────────────────────────────────────*/
    uint32_t cycle = 0;
    while (1) {
        escpos_printer_t *printer = NULL;
        cycle++;

        ESP_LOGI(TAG, "\n========== CYCLE %lu ==========", cycle);
        ESP_LOGI(TAG, "Step 2/2: Searching for USB printer...");

        /* ─────────────────────────────────────────────────────────────────────
         * PRINTER CONNECTION PHASE
         * ─────────────────────────────────────────────────────────────────────*/

        esp_err_t err = escpos_new_usb(&printer);

        if (err != ESP_OK) {
            ESP_LOGW(TAG, "✗ Printer not found (%s)", esp_err_to_name(err));
            ESP_LOGW(TAG, "  Retrying in 2 seconds...");
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        ESP_LOGI(TAG, " Printer connected!");
        vTaskDelay(pdMS_TO_TICKS(500));

        /* ─────────────────────────────────────────────────────────────────────
         * PRINT DEMONSTRATIONS
         * ─────────────────────────────────────────────────────────────────────*/

        /* Feature 1: Text Formatting */
        demo_text_formatting(printer);
        vTaskDelay(pdMS_TO_TICKS(500));

        /* Feature 2: IMAGE PRINTING (NEW!) */
        demo_image_printing(printer);
        vTaskDelay(pdMS_TO_TICKS(500));

        /* Feature 3: Paper Control */
        demo_paper_control(printer);
        vTaskDelay(pdMS_TO_TICKS(500));

        /* Feature 4: Audio Feedback */
        demo_audio_feedback(printer);
        vTaskDelay(pdMS_TO_TICKS(500));

        // /* Feature 5: Complete Receipt Example */
        // demo_complete_receipt(printer);
        // vTaskDelay(pdMS_TO_TICKS(500));

        /* ─────────────────────────────────────────────────────────────────────
         * PAPER CUT
         * ─────────────────────────────────────────────────────────────────────*/

        ESP_LOGI(TAG, "Cutting paper...");
        escpos_cut_paper(printer);
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, " All features demonstrated successfully!");
        ESP_LOGI(TAG, "Waiting for printer disconnect...");

        /* ─────────────────────────────────────────────────────────────────────
         * DISCONNECT WAIT PHASE
         * ─────────────────────────────────────────────────────────────────────*/

        escpos_wait_disconnect(printer, portMAX_DELAY);

        /* ─────────────────────────────────────────────────────────────────────
         * CLEANUP PHASE
         * ─────────────────────────────────────────────────────────────────────*/

        escpos_destroy(printer);
        printer = NULL;

        ESP_LOGW(TAG, "✗ Printer disconnected");
        ESP_LOGI(TAG, "Ready for next cycle. Searching again...\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    /* Cleanup (only reached if loop exits) */
    escpos_deinit();
}
