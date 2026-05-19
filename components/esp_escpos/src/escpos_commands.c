#include "esp_check.h"

#include "escpos_private.h"
#include "escpos_commands.h"

static const char *TAG = "escpos_commands";

esp_err_t escpos_set_align(escpos_printer_t *printer, escpos_align_t align)
{
    ESP_RETURN_ON_FALSE(printer && align <= ESCPOS_ALIGN_RIGHT,
                        ESP_ERR_INVALID_ARG, TAG, "Invalid alignment");

    uint8_t cmd[] = {CMD_ALIGN, (uint8_t)align};
    return escpos_write_raw(printer, cmd, sizeof(cmd));
}

esp_err_t escpos_set_bold(escpos_printer_t *printer, bool enabled)
{
    uint8_t cmd[] = {CMD_BOLD, enabled ? 1 : 0};
    return escpos_write_raw(printer, cmd, sizeof(cmd));
}

esp_err_t escpos_set_underline(escpos_printer_t *printer, uint8_t thickness)
{
    ESP_RETURN_ON_FALSE(thickness <= 2, ESP_ERR_INVALID_ARG, TAG,
                        "Underline thickness must be 0, 1, or 2");

    uint8_t cmd[] = {CMD_UNDERLINE, thickness};
    return escpos_write_raw(printer, cmd, sizeof(cmd));
}

esp_err_t escpos_set_font(escpos_printer_t *printer, escpos_font_t font)
{
    ESP_RETURN_ON_FALSE(font <= ESCPOS_FONT_C, ESP_ERR_INVALID_ARG, TAG,
                        "Invalid font");

    uint8_t cmd[] = {CMD_FONT_SELECT, (uint8_t)font};
    return escpos_write_raw(printer, cmd, sizeof(cmd));
}

esp_err_t escpos_set_text_size(escpos_printer_t *printer, uint8_t width, uint8_t height)
{
    ESP_RETURN_ON_FALSE(printer, ESP_ERR_INVALID_ARG, TAG, "NULL printer");
    ESP_RETURN_ON_FALSE(width >= 1 && width <= 8 && height >= 1 && height <= 8,
                        ESP_ERR_INVALID_ARG, TAG,
                        "Text width and height multipliers must be 1..8");

    uint8_t size = (uint8_t)(((width - 1) << 4) | (height - 1));
    uint8_t cmd[] = {CMD_CHAR_SIZE, size};
    ESP_RETURN_ON_ERROR(escpos_write_raw(printer, cmd, sizeof(cmd)),
                        TAG, "Failed to set text size");

    printer->text_width_multiplier = width;
    printer->text_height_multiplier = height;
    return ESP_OK;
}

esp_err_t escpos_set_style(escpos_printer_t *printer, escpos_style_t style)
{
    ESP_RETURN_ON_FALSE(printer, ESP_ERR_INVALID_ARG, TAG, "NULL printer");

    uint8_t cmd[] = {CMD_PRINT_MODE, (uint8_t)style};
    ESP_RETURN_ON_ERROR(escpos_write_raw(printer, cmd, sizeof(cmd)),
                        TAG, "Failed to set text style");

    printer->text_width_multiplier = (style & ESCPOS_STYLE_DOUBLE_WIDTH) ? 2 : 1;
    printer->text_height_multiplier = (style & ESCPOS_STYLE_DOUBLE_HEIGHT) ? 2 : 1;
    return ESP_OK;
}

esp_err_t escpos_reset_text_format(escpos_printer_t *printer)
{
    ESP_RETURN_ON_ERROR(escpos_set_style(printer, ESCPOS_STYLE_NORMAL), TAG,
                        "Failed to reset print mode");
    ESP_RETURN_ON_ERROR(escpos_set_text_size(printer, 1, 1), TAG,
                        "Failed to reset text size");
    ESP_RETURN_ON_ERROR(escpos_set_bold(printer, false), TAG,
                        "Failed to reset bold");
    ESP_RETURN_ON_ERROR(escpos_set_underline(printer, 0), TAG,
                        "Failed to reset underline");
    return escpos_set_font(printer, ESCPOS_FONT_A);
}

esp_err_t escpos_set_paper_width_mm(escpos_printer_t *printer, uint16_t paper_width_mm)
{
    ESP_RETURN_ON_FALSE(printer && paper_width_mm > 0,
                        ESP_ERR_INVALID_ARG, TAG, "Invalid paper width");

    uint16_t dots = 0;
    uint16_t chars = 0;

    if (paper_width_mm >= 57 && paper_width_mm <= 58) {
        dots = 384;
        chars = 32;
    } else if (paper_width_mm >= 76 && paper_width_mm <= 80) {
        dots = 576;
        chars = 48;
    } else {
        dots = (uint16_t)(((uint32_t)paper_width_mm * 2030U + 127U) / 254U);
        chars = (uint16_t)(dots / 12U);
    }

    if (chars == 0) {
        chars = 1;
    }

    return escpos_set_printer_width(printer, dots, chars);
}

esp_err_t escpos_set_printer_width(escpos_printer_t *printer, uint16_t dots, uint16_t chars)
{
    ESP_RETURN_ON_FALSE(printer && dots > 0 && chars > 0,
                        ESP_ERR_INVALID_ARG, TAG, "Invalid printer width");

    printer->width_dots = dots;
    printer->width_chars = chars;
    return ESP_OK;
}

esp_err_t escpos_init_printer(escpos_printer_t *printer)
{
    static const uint8_t cmd[] = {CMD_INIT};
    return escpos_write_raw(printer, cmd, sizeof(cmd));
}

esp_err_t escpos_init_printer_with_width_mm(escpos_printer_t *printer, uint16_t paper_width_mm)
{
    ESP_RETURN_ON_ERROR(escpos_init_printer(printer), TAG, "Failed to initialize printer");
    return escpos_set_paper_width_mm(printer, paper_width_mm);
}

esp_err_t escpos_feed_line(escpos_printer_t *printer)
{
    static const uint8_t lf = CMD_LF;
    return escpos_write_raw(printer, &lf, 1);
}

esp_err_t escpos_feed_lines(escpos_printer_t *printer, uint8_t n)
{
    ESP_RETURN_ON_FALSE(printer && n > 0, ESP_ERR_INVALID_ARG, TAG,
                        "Invalid feed count");

    uint8_t cmd[] = {CMD_FEED_N, n};
    return escpos_write_raw(printer, cmd, sizeof(cmd));
}

esp_err_t escpos_cut_paper(escpos_printer_t *printer)
{
    static const uint8_t cmd[] = {CMD_CUT_FULL};
    return escpos_write_raw(printer, cmd, sizeof(cmd));
}

esp_err_t escpos_cut_paper_partial(escpos_printer_t *printer)
{
    static const uint8_t cmd[] = {CMD_CUT_PARTIAL};
    return escpos_write_raw(printer, cmd, sizeof(cmd));
}

esp_err_t escpos_beep(escpos_printer_t *printer)
{
    static const uint8_t cmd[] = {CMD_BEEP, 0x03, 0x02};
    return escpos_write_raw(printer, cmd, sizeof(cmd));
}
