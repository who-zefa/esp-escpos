#include <string.h>

#include "esp_check.h"

#include "escpos_private.h"

static const char *TAG = "escpos_formatter";

static size_t escpos_single_line_len(const char *text)
{
    size_t len = 0;

    if (!text) {
        return 0;
    }

    while (text[len] && text[len] != '\r' && text[len] != '\n') {
        len++;
    }

    return len;
}

static void escpos_calculate_padding(size_t text_len,
                                     uint16_t width,
                                     escpos_align_t align,
                                     bool fill_right,
                                     uint16_t *left_spaces,
                                     uint16_t *right_spaces)
{
    *left_spaces = 0;
    *right_spaces = 0;

    if (text_len >= width) {
        return;
    }

    uint16_t extra_spaces = (uint16_t)(width - text_len);
    if (align == ESCPOS_ALIGN_RIGHT) {
        *left_spaces = extra_spaces;
    } else if (align == ESCPOS_ALIGN_CENTER) {
        *left_spaces = (uint16_t)(extra_spaces / 2);
        *right_spaces = (uint16_t)(extra_spaces - *left_spaces);
    } else if (fill_right) {
        *right_spaces = extra_spaces;
    }
}

esp_err_t escpos_write_text_aligned(escpos_printer_t *printer, const char *text, escpos_align_t align)
{
    ESP_RETURN_ON_FALSE(printer && text && align <= ESCPOS_ALIGN_RIGHT,
                        ESP_ERR_INVALID_ARG, TAG, "Invalid aligned text argument");
    ESP_RETURN_ON_ERROR(escpos_require_paper_width(printer),
                        TAG, "Cannot align text without paper width");

    uint16_t width_chars = escpos_get_effective_width_chars(printer);
    ESP_RETURN_ON_FALSE(width_chars > 0, ESP_ERR_INVALID_ARG, TAG,
                        "Printer text width is not configured");

    const char *line = text;
    while (*line) {
        const char *line_end = line;
        while (*line_end && *line_end != '\r' && *line_end != '\n') {
            line_end++;
        }

        size_t line_len = (size_t)(line_end - line);
        uint16_t left_spaces = 0;
        uint16_t right_spaces = 0;
        escpos_calculate_padding(line_len, width_chars, align, false,
                                 &left_spaces, &right_spaces);

        ESP_RETURN_ON_ERROR(escpos_write_spaces(printer, left_spaces),
                            TAG, "Failed to write left alignment padding");
        if (line_len > 0) {
            ESP_RETURN_ON_ERROR(escpos_write_raw(printer, (const uint8_t *)line, line_len),
                                TAG, "Failed to write aligned text");
        }
        ESP_RETURN_ON_ERROR(escpos_write_spaces(printer, right_spaces),
                            TAG, "Failed to write right alignment padding");

        if (*line_end == '\r') {
            ESP_RETURN_ON_ERROR(escpos_write_raw(printer, (const uint8_t *)"\r", 1),
                                TAG, "Failed to write carriage return");
            line_end++;
            if (*line_end == '\n') {
                ESP_RETURN_ON_ERROR(escpos_write_raw(printer, (const uint8_t *)"\n", 1),
                                    TAG, "Failed to write line feed");
                line_end++;
            }
        } else if (*line_end == '\n') {
            ESP_RETURN_ON_ERROR(escpos_write_raw(printer, (const uint8_t *)"\n", 1),
                                TAG, "Failed to write line feed");
            line_end++;
        }

        line = line_end;
    }

    return ESP_OK;
}

static esp_err_t escpos_write_column_text(escpos_printer_t *printer,
                                          const char *text,
                                          uint8_t width,
                                          escpos_align_t align)
{
    size_t text_len = escpos_single_line_len(text);
    if (text_len > width) {
        text_len = width;
    }

    uint16_t left_spaces = 0;
    uint16_t right_spaces = 0;
    escpos_calculate_padding(text_len, width, align, true,
                             &left_spaces, &right_spaces);

    ESP_RETURN_ON_ERROR(escpos_write_spaces(printer, left_spaces),
                        TAG, "Failed to write column left padding");
    if (text_len > 0) {
        ESP_RETURN_ON_ERROR(escpos_write_raw(printer, (const uint8_t *)text, text_len),
                            TAG, "Failed to write column text");
    }
    return escpos_write_spaces(printer, right_spaces);
}

esp_err_t escpos_write_columns(escpos_printer_t *printer,
                               const escpos_text_column_t *columns,
                               size_t column_count)
{
    ESP_RETURN_ON_FALSE(printer && columns && column_count > 0 && column_count <= 8,
                        ESP_ERR_INVALID_ARG, TAG, "Invalid columns argument");
    ESP_RETURN_ON_ERROR(escpos_require_paper_width(printer),
                        TAG, "Cannot write columns without paper width");

    uint16_t line_width = escpos_get_effective_width_chars(printer);
    ESP_RETURN_ON_FALSE(line_width > 0 && line_width <= 255,
                        ESP_ERR_INVALID_ARG, TAG,
                        "Printer text width is not configured");

    uint16_t used_width = 0;
    for (size_t i = 0; i < column_count; i++) {
        ESP_RETURN_ON_FALSE(columns[i].text && columns[i].width > 0 &&
                            columns[i].align <= ESCPOS_ALIGN_RIGHT,
                            ESP_ERR_INVALID_ARG, TAG, "Invalid column");
        used_width = (uint16_t)(used_width + columns[i].width);
    }
    ESP_RETURN_ON_FALSE(used_width <= line_width,
                        ESP_ERR_INVALID_ARG, TAG,
                        "Columns use %u chars, printer line has %u chars",
                        used_width, line_width);

    for (size_t i = 0; i < column_count; i++) {
        ESP_RETURN_ON_ERROR(escpos_write_column_text(printer,
                                                     columns[i].text,
                                                     columns[i].width,
                                                     columns[i].align),
                            TAG, "Failed to write column");
    }

    return escpos_write_text(printer, "\r\n");
}

esp_err_t escpos_write_2_columns(escpos_printer_t *printer,
                                 const char *left,
                                 const char *right)
{
    ESP_RETURN_ON_FALSE(printer && left && right, ESP_ERR_INVALID_ARG,
                        TAG, "Invalid 2-column argument");

    uint16_t width = escpos_get_effective_width_chars(printer);
    ESP_RETURN_ON_FALSE(width >= 2 && width <= 255, ESP_ERR_INVALID_ARG,
                        TAG, "Printer text width is too small");

    uint8_t right_width = (uint8_t)(width / 2);
    escpos_text_column_t columns[] = {
        {.text = left, .width = (uint8_t)(width - right_width), .align = ESCPOS_ALIGN_LEFT},
        {.text = right, .width = right_width, .align = ESCPOS_ALIGN_RIGHT},
    };

    return escpos_write_columns(printer, columns, 2);
}

esp_err_t escpos_write_3_columns(escpos_printer_t *printer,
                                 const char *name,
                                 const char *qty,
                                 const char *price)
{
    ESP_RETURN_ON_FALSE(printer && name && qty && price, ESP_ERR_INVALID_ARG,
                        TAG, "Invalid 3-column argument");

    uint16_t width = escpos_get_effective_width_chars(printer);
    ESP_RETURN_ON_FALSE(width >= 13 && width <= 255, ESP_ERR_INVALID_ARG,
                        TAG, "Printer text width is too small");

    uint8_t qty_width = 5;
    uint8_t price_width = 10;
    if (width < 32) {
        qty_width = 4;
        price_width = 8;
    }

    escpos_text_column_t columns[] = {
        {.text = name, .width = (uint8_t)(width - qty_width - price_width), .align = ESCPOS_ALIGN_LEFT},
        {.text = qty, .width = qty_width, .align = ESCPOS_ALIGN_RIGHT},
        {.text = price, .width = price_width, .align = ESCPOS_ALIGN_RIGHT},
    };

    return escpos_write_columns(printer, columns, 3);
}

esp_err_t escpos_write_4_columns(escpos_printer_t *printer,
                                 const char *item,
                                 const char *qty,
                                 const char *rate,
                                 const char *total)
{
    ESP_RETURN_ON_FALSE(printer && item && qty && rate && total, ESP_ERR_INVALID_ARG,
                        TAG, "Invalid 4-column argument");

    uint16_t width = escpos_get_effective_width_chars(printer);
    ESP_RETURN_ON_FALSE(width >= 21 && width <= 255, ESP_ERR_INVALID_ARG,
                        TAG, "Printer text width is too small");

    uint8_t qty_width = 4;
    uint8_t rate_width = 9;
    uint8_t total_width = 10;
    if (width < 42) {
        rate_width = 8;
        total_width = 8;
    }

    escpos_text_column_t columns[] = {
        {.text = item, .width = (uint8_t)(width - qty_width - rate_width - total_width), .align = ESCPOS_ALIGN_LEFT},
        {.text = qty, .width = qty_width, .align = ESCPOS_ALIGN_RIGHT},
        {.text = rate, .width = rate_width, .align = ESCPOS_ALIGN_RIGHT},
        {.text = total, .width = total_width, .align = ESCPOS_ALIGN_RIGHT},
    };

    return escpos_write_columns(printer, columns, 4);
}
