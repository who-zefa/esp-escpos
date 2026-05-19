/*
 * Text printing example.
 *
 * Use this file as the app main source for a small test project.
 */

#include "esp_err.h"
#include "escpos.h"

void app_main(void)
{
    escpos_printer_t *printer = NULL;

    ESP_ERROR_CHECK(escpos_init());
    ESP_ERROR_CHECK(escpos_new_usb(&printer));
    ESP_ERROR_CHECK(escpos_init_printer_with_width_mm(printer, 58));

    ESP_ERROR_CHECK(escpos_write_text(printer, "Normal text\n"));

    ESP_ERROR_CHECK(escpos_set_bold(printer, true));
    ESP_ERROR_CHECK(escpos_write_text(printer, "Bold text\n"));
    ESP_ERROR_CHECK(escpos_set_bold(printer, false));

    ESP_ERROR_CHECK(escpos_set_underline(printer, true));
    ESP_ERROR_CHECK(escpos_write_text(printer, "Underlined text\n"));
    ESP_ERROR_CHECK(escpos_set_underline(printer, false));

    ESP_ERROR_CHECK(escpos_set_text_size(printer, 2, 2));
    ESP_ERROR_CHECK(escpos_write_text_aligned(printer, "Large centered text", ESCPOS_ALIGN_CENTER));
    ESP_ERROR_CHECK(escpos_write_text(printer, "\n"));
    ESP_ERROR_CHECK(escpos_reset_text_format(printer));

    ESP_ERROR_CHECK(escpos_write_text_aligned(printer, "Left", ESCPOS_ALIGN_LEFT));
    ESP_ERROR_CHECK(escpos_write_text(printer, "\n"));
    ESP_ERROR_CHECK(escpos_write_text_aligned(printer, "Center", ESCPOS_ALIGN_CENTER));
    ESP_ERROR_CHECK(escpos_write_text(printer, "\n"));
    ESP_ERROR_CHECK(escpos_write_text_aligned(printer, "Right", ESCPOS_ALIGN_RIGHT));
    ESP_ERROR_CHECK(escpos_write_text(printer, "\n"));

    ESP_ERROR_CHECK(escpos_write_text(printer, "\nReceipt columns\n"));
    ESP_ERROR_CHECK(escpos_write_3_columns(printer, "ITEM", "QTY", "PRICE"));
    ESP_ERROR_CHECK(escpos_write_3_columns(printer, "Coffee", "2", "7.00"));
    ESP_ERROR_CHECK(escpos_write_3_columns(printer, "Sandwich", "1", "6.25"));
    ESP_ERROR_CHECK(escpos_write_2_columns(printer, "TOTAL", "$13.25"));

    ESP_ERROR_CHECK(escpos_feed_lines(printer, 3));
    ESP_ERROR_CHECK(escpos_cut_paper(printer));

    escpos_destroy(printer);
    ESP_ERROR_CHECK(escpos_deinit());
}
