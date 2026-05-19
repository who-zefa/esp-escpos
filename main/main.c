#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "escpos.h"
#include "escpos_examples.h"

static const char *TAG = "main";

typedef esp_err_t (*example_fn_t)(escpos_printer_t *printer);

typedef struct {
    const char *name;
    example_fn_t run;
} example_entry_t;

static const example_entry_t EXAMPLES[] = {
    {"image printing", example_image_printing},
    {"text formatting", example_text_formatting},
    {"receipt columns", example_receipt_columns},
    {"barcode printing", example_barcode_printing},
    {"QR printing", example_qr_printing},
    {"raw commands", example_raw_commands},
    {"paper control", example_paper_control},
    {"audio feedback", example_audio_feedback},
    {"complete receipt", example_complete_receipt},
};

static void run_examples(escpos_printer_t *printer)
{
    for (size_t i = 0; i < sizeof(EXAMPLES) / sizeof(EXAMPLES[0]); i++) {
        ESP_LOGI(TAG, "Running example: %s", EXAMPLES[i].name);

        esp_err_t err = EXAMPLES[i].run(printer);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Example failed: %s (%s)",
                     EXAMPLES[i].name, esp_err_to_name(err));
            escpos_write_text(printer, "\r\nExample failed: ");
            escpos_write_text(printer, EXAMPLES[i].name);
            escpos_write_text(printer, "\r\n\r\n");
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing ESC/POS library");
    ESP_ERROR_CHECK(escpos_init());

    uint32_t cycle = 0;
    while (true) {
        escpos_printer_t *printer = NULL;
        cycle++;

        ESP_LOGI(TAG, "Cycle %" PRIu32 ": searching for USB printer", cycle);
        esp_err_t err = escpos_new_usb(&printer);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Printer not found: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        ESP_LOGI(TAG, "Printer connected");
        vTaskDelay(pdMS_TO_TICKS(500));

        ESP_ERROR_CHECK(escpos_init_printer_with_width_mm(printer, 80));
        escpos_write_text_aligned(printer, "ESP32 ESC/POS EXAMPLES", ESCPOS_ALIGN_CENTER);
        escpos_write_text(printer, "\r\n\r\n");

        run_examples(printer);

        escpos_feed_lines(printer, 3);
        escpos_cut_paper(printer);

        ESP_LOGI(TAG, "Waiting for printer disconnect");
        escpos_wait_disconnect(printer, portMAX_DELAY);

        escpos_destroy(printer);
        ESP_LOGI(TAG, "Printer disconnected");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
