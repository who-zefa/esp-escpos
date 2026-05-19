/*
 * Image printing example.
 *
 * Add an image.bmp file to your main folder and include it with:
 * idf_component_register(... EMBED_FILES "image.bmp")
 */

#include "esp_err.h"
#include "escpos.h"

extern const uint8_t image_bmp_start[] asm("_binary_image_bmp_start");
extern const uint8_t image_bmp_end[] asm("_binary_image_bmp_end");

void app_main(void)
{
    escpos_printer_t *printer = NULL;
    escpos_image_t image = {0};

    ESP_ERROR_CHECK(escpos_init());
    ESP_ERROR_CHECK(escpos_new_usb(&printer));
    ESP_ERROR_CHECK(escpos_init_printer_with_width_mm(printer, 58));

    escpos_image_params_t params = escpos_image_get_default_params();
    params.align = ESCPOS_ALIGN_CENTER;
    params.dither_mode = ESCPOS_DITHER_FLOYD_STEINBERG;
    params.max_width = escpos_get_printer_width_dots(printer);

    ESP_ERROR_CHECK(escpos_image_load_from_buffer(
        image_bmp_start,
        (size_t)(image_bmp_end - image_bmp_start),
        &params,
        &image
    ));
    ESP_ERROR_CHECK(escpos_print_image(printer, &image, ESCPOS_IMAGE_MODE_NORMAL));
    escpos_image_free(&image);

    ESP_ERROR_CHECK(escpos_feed_lines(printer, 3));
    ESP_ERROR_CHECK(escpos_cut_paper(printer));

    escpos_destroy(printer);
    ESP_ERROR_CHECK(escpos_deinit());
}
