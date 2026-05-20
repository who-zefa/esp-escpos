/*
 * Barcode printing example.
 *
 * Code128 and EAN13 are rendered by the library and sent as raster image data.
 */

#include "esp_err.h"
#include "escpos.h"

void app_main(void)
{
    escpos_printer_t *printer = NULL;

    ESP_ERROR_CHECK(escpos_init());
    ESP_ERROR_CHECK(escpos_new_usb(&printer));
    ESP_ERROR_CHECK(escpos_init_printer_with_width_mm(printer, 58));

    escpos_barcode_config_t code128 = escpos_barcode_get_default_config(ESCPOS_BARCODE_CODE128);
    code128.align = ESCPOS_ALIGN_CENTER;
    code128.width = 2;
    code128.height = 80;
    ESP_ERROR_CHECK(escpos_write_text_aligned(printer, "Code128", ESCPOS_ALIGN_CENTER));
    ESP_ERROR_CHECK(escpos_write_text(printer, "\n"));
    ESP_ERROR_CHECK(escpos_print_barcode(printer, "ABC123456", &code128));
    ESP_ERROR_CHECK(escpos_feed_lines(printer, 2));

    escpos_barcode_config_t ean13 = escpos_barcode_get_default_config(ESCPOS_BARCODE_EAN13);
    ean13.align = ESCPOS_ALIGN_CENTER;
    ean13.height = 80;
    ESP_ERROR_CHECK(escpos_write_text_aligned(printer, "EAN13", ESCPOS_ALIGN_CENTER));
    ESP_ERROR_CHECK(escpos_write_text(printer, "\n"));
    ESP_ERROR_CHECK(escpos_print_barcode(printer, "5901234123457", &ean13));

    ESP_ERROR_CHECK(escpos_feed_lines(printer, 3));
    ESP_ERROR_CHECK(escpos_cut_paper(printer));

    escpos_destroy(printer);
    ESP_ERROR_CHECK(escpos_deinit());
}
