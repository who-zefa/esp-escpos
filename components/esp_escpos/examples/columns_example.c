/*
 * Receipt column example.
 *
 * These helpers calculate spaces on the ESP32 side.
 */

#include "esp_err.h"
#include "escpos.h"

void app_main(void)
{
    escpos_printer_t *printer = NULL;

    ESP_ERROR_CHECK(escpos_init());
    ESP_ERROR_CHECK(escpos_new_usb(&printer));
    ESP_ERROR_CHECK(escpos_init_printer_with_width_mm(printer, 58));

    ESP_ERROR_CHECK(escpos_write_text_aligned(printer, "SAMPLE RECEIPT", ESCPOS_ALIGN_CENTER));
    ESP_ERROR_CHECK(escpos_write_text(printer, "\n\n"));

    ESP_ERROR_CHECK(escpos_set_bold(printer, true));
    ESP_ERROR_CHECK(escpos_write_3_columns(printer, "ITEM", "QTY", "PRICE"));
    ESP_ERROR_CHECK(escpos_set_bold(printer, false));
    ESP_ERROR_CHECK(escpos_write_3_columns(printer, "Coffee", "2", "7.00"));
    ESP_ERROR_CHECK(escpos_write_3_columns(printer, "Sandwich", "1", "6.25"));
    ESP_ERROR_CHECK(escpos_write_3_columns(printer, "Cookie", "3", "3.75"));
    ESP_ERROR_CHECK(escpos_write_text(printer, "\n"));

    ESP_ERROR_CHECK(escpos_write_2_columns(printer, "Subtotal", "$17.00"));
    ESP_ERROR_CHECK(escpos_write_2_columns(printer, "Tax", "$1.36"));
    ESP_ERROR_CHECK(escpos_write_2_columns(printer, "TOTAL", "$18.36"));
    ESP_ERROR_CHECK(escpos_write_text(printer, "\n"));

    ESP_ERROR_CHECK(escpos_write_4_columns(printer, "ITEM", "QTY", "RATE", "TOTAL"));
    ESP_ERROR_CHECK(escpos_write_4_columns(printer, "Coffee", "2", "3.50", "7.00"));

    ESP_ERROR_CHECK(escpos_feed_lines(printer, 3));
    ESP_ERROR_CHECK(escpos_cut_paper(printer));

    escpos_destroy(printer);
    ESP_ERROR_CHECK(escpos_deinit());
}
