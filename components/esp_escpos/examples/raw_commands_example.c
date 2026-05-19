/*
 * Raw ESC/POS command example.
 *
 * Use escpos_write_command() when you need to send bytes directly.
 */

#include "esp_err.h"
#include "escpos.h"

void app_main(void)
{
    escpos_printer_t *printer = NULL;

    const uint8_t bold_on[] = {0x1B, 0x45, 0x01};
    const uint8_t bold_off[] = {0x1B, 0x45, 0x00};
    const uint8_t feed_3[] = {0x1B, 0x64, 0x03};

    ESP_ERROR_CHECK(escpos_init());
    ESP_ERROR_CHECK(escpos_new_usb(&printer));
    ESP_ERROR_CHECK(escpos_init_printer_with_width_mm(printer, 58));

    ESP_ERROR_CHECK(escpos_write_command(printer, bold_on, sizeof(bold_on)));
    ESP_ERROR_CHECK(escpos_write_text(printer, "Raw command: bold on\n"));
    ESP_ERROR_CHECK(escpos_write_command(printer, bold_off, sizeof(bold_off)));
    ESP_ERROR_CHECK(escpos_write_text(printer, "Raw command: bold off\n"));
    ESP_ERROR_CHECK(escpos_write_command(printer, feed_3, sizeof(feed_3)));
    ESP_ERROR_CHECK(escpos_cut_paper(printer));

    escpos_destroy(printer);
    ESP_ERROR_CHECK(escpos_deinit());
}
