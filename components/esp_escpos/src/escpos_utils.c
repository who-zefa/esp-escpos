#include "esp_check.h"

#include "escpos_private.h"

static const char *TAG = "escpos_utils";

esp_err_t escpos_require_paper_width(const escpos_printer_t *printer)
{
    ESP_RETURN_ON_FALSE(printer && printer->width_dots > 0 && printer->width_chars > 0,
                        ESP_ERR_INVALID_STATE, TAG,
                        "Paper width not configured; call escpos_init_printer_with_width_mm()");
    return ESP_OK;
}

uint16_t escpos_get_effective_width_chars(const escpos_printer_t *printer)
{
    if (!printer || printer->width_chars == 0 || printer->width_dots == 0) {
        return 0;
    }

    uint8_t text_width = printer->text_width_multiplier ? printer->text_width_multiplier : 1;
    return (uint16_t)(printer->width_chars / text_width);
}

esp_err_t escpos_write_spaces(escpos_printer_t *printer, uint16_t count)
{
    static const char spaces[] = "                                ";

    while (count > 0) {
        size_t chunk = count < (sizeof(spaces) - 1) ? count : (sizeof(spaces) - 1);
        ESP_RETURN_ON_ERROR(escpos_write_raw(printer, (const uint8_t *)spaces, chunk),
                            TAG, "Failed to write spaces");
        count -= (uint16_t)chunk;
    }

    return ESP_OK;
}

static uint16_t escpos_get_graphic_left_spaces(const escpos_printer_t *printer,
                                               uint16_t graphic_width_dots,
                                               escpos_align_t align)
{
    if (!printer || align == ESCPOS_ALIGN_LEFT ||
        printer->width_dots == 0 || printer->width_chars == 0 ||
        graphic_width_dots >= printer->width_dots) {
        return 0;
    }

    uint16_t char_width_dots = (uint16_t)(printer->width_dots / printer->width_chars);
    if (char_width_dots == 0) {
        return 0;
    }

    uint16_t free_dots = (uint16_t)(printer->width_dots - graphic_width_dots);
    uint16_t left_dots = align == ESCPOS_ALIGN_CENTER ? (uint16_t)(free_dots / 2) : free_dots;
    return (uint16_t)(left_dots / char_width_dots);
}

esp_err_t escpos_write_graphic_alignment(escpos_printer_t *printer,
                                         uint16_t graphic_width_dots,
                                         escpos_align_t align)
{
    ESP_RETURN_ON_FALSE(align <= ESCPOS_ALIGN_RIGHT, ESP_ERR_INVALID_ARG, TAG,
                        "Invalid graphic alignment");
    if (align != ESCPOS_ALIGN_LEFT) {
        ESP_RETURN_ON_ERROR(escpos_require_paper_width(printer),
                            TAG, "Cannot align graphic without paper width");
    }

    return escpos_write_spaces(printer, escpos_get_graphic_left_spaces(printer,
                                                                       graphic_width_dots,
                                                                       align));
}
