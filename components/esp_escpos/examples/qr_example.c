/*
 * QR code printing example.
 *
 * The library builds the QR bitmap and sends it as raster image data.
 */

#include "esp_err.h"
#include "escpos.h"

void app_main(void)
{
    escpos_printer_t *printer = NULL;

    ESP_ERROR_CHECK(escpos_init());
    ESP_ERROR_CHECK(escpos_new_usb(&printer));
    ESP_ERROR_CHECK(escpos_init_printer_with_width_mm(printer, 58));

    escpos_qr_config_t qr = escpos_qr_get_default_config();
    qr.align = ESCPOS_ALIGN_CENTER;
    qr.size = 6;
    qr.ec_level = ESCPOS_QR_EC_M;

    ESP_ERROR_CHECK(escpos_write_text_aligned(printer, "QR Code", ESCPOS_ALIGN_CENTER));
    ESP_ERROR_CHECK(escpos_write_text(printer, "\n"));
    ESP_ERROR_CHECK(escpos_print_qr(printer, "https://example.com/qr", &qr));

    ESP_ERROR_CHECK(escpos_feed_lines(printer, 3));
    ESP_ERROR_CHECK(escpos_cut_paper(printer));

    escpos_destroy(printer);
    ESP_ERROR_CHECK(escpos_deinit());
}
