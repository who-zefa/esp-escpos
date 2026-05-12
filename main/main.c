/**
 * @file main.c
 * @brief ESP32 ESC/POS USB Printer Library Example
 * 
 * This example demonstrates basic usage of the esp32_escpos library
 * for printing via USB CDC-ACM to thermal printers.
 * 
 * ==============================================================================
 * LIBRARY OVERVIEW:
 * 
 * - esp_system, esp_log, esp_err       : ESP-IDF Core Libraries
 * - FreeRTOS                            : Real-time OS (task management, delays)
 * - escpos.h                            : esp32_escpos Library (main printer API)
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

/* ESP32 ESCPOS Thermal Printer Library */
#include "escpos.h"          /* Main printer API - all printing functions */

/* Logger tag for this module */
static const char *TAG = "MAIN";

/**
 * ================================================================================
 * MAIN APPLICATION ENTRY POINT
 * 
 * FreeRTOS calls this function after system initialization.
 * This demonstrates a complete print cycle: connect → print → disconnect → repeat
 * ================================================================================
 */
void app_main(void)
{
    ESP_LOGI(TAG, "Initializing ESC/POS Library");
    
    /* ─────────────────────────────────────────────────────────────────────────
     * INITIALIZATION PHASE
     * ─────────────────────────────────────────────────────────────────────────*/
    
    /* 
     * Library: esp32_escpos
     * Function: escpos_init()
     * Purpose: Initialize the ESC/POS library
     * Details: Installs USB Host driver and CDC-ACM driver (must be called once)
     * Returns: ESP_OK on success, error code otherwise
     */
    ESP_ERROR_CHECK(escpos_init());

    /* ─────────────────────────────────────────────────────────────────────────
     * MAIN LOOP: Search for printer, print, wait for disconnect, repeat
     * ─────────────────────────────────────────────────────────────────────────*/
    while (1) {
        escpos_printer_t *printer = NULL;

        ESP_LOGI(TAG, "Searching for USB printer...");

        /* ─────────────────────────────────────────────────────────────────────
         * PRINTER CONNECTION PHASE
         * ─────────────────────────────────────────────────────────────────────*/

        /* 
         * Library: esp32_escpos
         * Function: escpos_new_usb()
         * Purpose: Create and connect to a USB printer
         * Details: Blocks until printer is found or timeout (default ~2 seconds)
         *          Opens any CDC-ACM device (VID=0x0000, PID=0x0000)
         * Returns: ESP_OK if connected, ESP_ERR_TIMEOUT if not found
         */
        esp_err_t err = escpos_new_usb(&printer);

        /* Handle connection failure - retry */
        if (err != ESP_OK) {
            /* Library: esp_err.h */
            ESP_LOGW(TAG, "Printer not found (err=%s). Retrying in 2 seconds...",
                     esp_err_to_name(err));
            
            /* Library: FreeRTOS, Function: vTaskDelay() */
            /* pdMS_TO_TICKS converts milliseconds to FreeRTOS tick counts */
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        ESP_LOGI(TAG, "Printer connected!");

        /* Give printer time to settle after USB enumeration */
        vTaskDelay(pdMS_TO_TICKS(500));

        /* ─────────────────────────────────────────────────────────────────────
         * PRINT JOB PHASE
         * ─────────────────────────────────────────────────────────────────────*/

        /* Initialize the printer (reset to default state) */
        ESP_LOGI(TAG, "Initializing printer...");
        
        /* 
         * Library: esp32_escpos
         * Function: escpos_init_printer()
         * Purpose: Send ESC @ command to reset printer to default state
         * Details: Clears any previous settings, initializes paper width, etc.
         * Returns: ESP_OK on success
         */
        escpos_init_printer(printer);
        vTaskDelay(pdMS_TO_TICKS(200));

        /* Send test text to printer */
        ESP_LOGI(TAG, "Printing test content...");
        
        /* 
         * Library: esp32_escpos
         * Function: escpos_write_text()
         * Purpose: Send a text string to the printer
         * Details: Text is sent exactly as provided (\r\n for line breaks)
         * Returns: ESP_OK on success, ESP_ERR_TIMEOUT on TX failure
         */
        escpos_write_text(printer, "========================\r\n");
        escpos_write_text(printer, "   ESP32 USB PRINTER\r\n");
        escpos_write_text(printer, "========================\r\n");
        escpos_write_text(printer, "Hello from ESP32-S3!\r\n");
        escpos_write_text(printer, "Printing via USB CDC\r\n");
        escpos_write_text(printer, "\r\n");
        escpos_write_text(printer, "Thank you!\r\n");
        vTaskDelay(pdMS_TO_TICKS(200));

        /* Audio feedback - beep the printer */
        ESP_LOGI(TAG, "Beeping printer...");
        
        /* 
         * Library: esp32_escpos
         * Function: escpos_beep()
         * Purpose: Send ESC B command to beep the printer speaker
         * Details: Sends beep signal (frequency and duration vary by printer model)
         * Returns: ESP_OK on success
         */
        escpos_beep(printer);
        vTaskDelay(pdMS_TO_TICKS(500));

        /* Feed paper and cut */
        ESP_LOGI(TAG, "Feeding paper and cutting...");
        
        /* 
         * Library: esp32_escpos
         * Function: escpos_feed_lines()
         * Purpose: Feed N blank lines before cutting
         * Details: Sends ESC d command with line count (1-255)
         *          Moves paper down by N lines to prepare for cut
         * Returns: ESP_OK on success
         */
        escpos_feed_lines(printer, 3);
        vTaskDelay(pdMS_TO_TICKS(200));
        
        /* 
         * Library: esp32_escpos
         * Function: escpos_cut_paper()
         * Purpose: Cut the paper (full cut)
         * Details: Sends GS V 0 command for full paper cut
         *          Some printers support partial cut (escpos_cut_paper_partial)
         * Returns: ESP_OK on success
         */
        escpos_cut_paper(printer);
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "Print job completed!");
        ESP_LOGI(TAG, "Waiting for printer disconnect...");

        /* ─────────────────────────────────────────────────────────────────────
         * DISCONNECT WAIT PHASE
         * ─────────────────────────────────────────────────────────────────────*/

        /* 
         * Library: esp32_escpos
         * Function: escpos_wait_disconnect()
         * Purpose: Block until the printer is physically disconnected
         * Details: Waits on an internal FreeRTOS semaphore signaled by USB events
         *          portMAX_DELAY = wait indefinitely (don't timeout)
         * Returns: ESP_OK if disconnected, ESP_ERR_TIMEOUT if timeout occurs
         */
        escpos_wait_disconnect(printer, portMAX_DELAY);

        /* ─────────────────────────────────────────────────────────────────────
         * CLEANUP PHASE
         * ─────────────────────────────────────────────────────────────────────*/

        /* 
         * Library: esp32_escpos
         * Function: escpos_destroy()
         * Purpose: Close printer connection and free allocated memory
         * Details: Closes USB handle, deletes semaphores, frees printer struct
         *          Safe to call even if printer was already disconnected
         * Returns: void (no return value)
         */
        escpos_destroy(printer);
        printer = NULL;

        ESP_LOGW(TAG, "Printer disconnected. Searching again...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    /* ─────────────────────────────────────────────────────────────────────────
     * LIBRARY CLEANUP (only reached if we exit the infinite loop)
     * ─────────────────────────────────────────────────────────────────────────*/
    
    /* 
     * Library: esp32_escpos
     * Function: escpos_deinit()
     * Purpose: Shutdown the ESC/POS library and free global resources
     * Details: Uninstalls USB Host and CDC-ACM drivers
     *          Only call this when all printer handles have been destroyed
     * Returns: ESP_OK on success
     */
    escpos_deinit();
}