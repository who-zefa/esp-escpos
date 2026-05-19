#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"
#include "escpos_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct escpos_printer_s escpos_printer_t;

/* Public APIs - core functionality */
esp_err_t escpos_init(void);
esp_err_t escpos_deinit(void);
esp_err_t escpos_new_usb(escpos_printer_t **printer);
esp_err_t escpos_new_usb_with_ids(uint16_t vid, uint16_t pid, escpos_printer_t **printer);
void escpos_destroy(escpos_printer_t *printer);

esp_err_t escpos_init_printer(escpos_printer_t *printer);
esp_err_t escpos_init_printer_with_width_mm(escpos_printer_t *printer, uint16_t paper_width_mm);
esp_err_t escpos_write_raw(escpos_printer_t *printer, const uint8_t *data, size_t len);
esp_err_t escpos_write_command(escpos_printer_t *printer, const uint8_t *data, size_t len);
esp_err_t escpos_write_text(escpos_printer_t *printer, const char *text);
esp_err_t escpos_write_text_aligned(escpos_printer_t *printer, const char *text, escpos_align_t align);
esp_err_t escpos_write_columns(escpos_printer_t *printer,
                               const escpos_text_column_t *columns,
                               size_t column_count);
esp_err_t escpos_write_2_columns(escpos_printer_t *printer,
                                 const char *left,
                                 const char *right);
esp_err_t escpos_write_3_columns(escpos_printer_t *printer,
                                 const char *name,
                                 const char *qty,
                                 const char *price);
esp_err_t escpos_write_4_columns(escpos_printer_t *printer,
                                 const char *item,
                                 const char *qty,
                                 const char *rate,
                                 const char *total);
esp_err_t escpos_set_align(escpos_printer_t *printer, escpos_align_t align);
esp_err_t escpos_set_bold(escpos_printer_t *printer, bool enabled);
esp_err_t escpos_set_underline(escpos_printer_t *printer, uint8_t thickness);
esp_err_t escpos_set_font(escpos_printer_t *printer, escpos_font_t font);
esp_err_t escpos_set_text_size(escpos_printer_t *printer, uint8_t width, uint8_t height);
esp_err_t escpos_set_style(escpos_printer_t *printer, escpos_style_t style);
esp_err_t escpos_reset_text_format(escpos_printer_t *printer);
/* Sets configured printable width from paper size. Common mappings: 58mm=384 dots/32 chars, 80mm=576 dots/48 chars. */
esp_err_t escpos_set_paper_width_mm(escpos_printer_t *printer, uint16_t paper_width_mm);
/* Advanced override for unusual mechanisms. This configures local layout only; it does not query the printer. */
esp_err_t escpos_set_printer_width(escpos_printer_t *printer, uint16_t dots, uint16_t chars);
/* Returns configured local layout width only; it does not query the printer. */
uint16_t escpos_get_printer_width_dots(const escpos_printer_t *printer);
/* Returns configured local text width only; it does not query the printer. */
uint16_t escpos_get_printer_width_chars(const escpos_printer_t *printer);
escpos_barcode_config_t escpos_barcode_get_default_config(escpos_barcode_type_t type);
esp_err_t escpos_print_barcode(escpos_printer_t *printer,
                               const char *data,
                               const escpos_barcode_config_t *config);
escpos_qr_config_t escpos_qr_get_default_config(void);
esp_err_t escpos_print_qr(escpos_printer_t *printer,
                          const char *data,
                          const escpos_qr_config_t *config);
esp_err_t escpos_feed_line(escpos_printer_t *printer);
esp_err_t escpos_feed_lines(escpos_printer_t *printer, uint8_t n);
esp_err_t escpos_cut_paper(escpos_printer_t *printer);
esp_err_t escpos_cut_paper_partial(escpos_printer_t *printer);
esp_err_t escpos_beep(escpos_printer_t *printer);
bool escpos_is_connected(const escpos_printer_t *printer);
esp_err_t escpos_wait_disconnect(escpos_printer_t *printer, uint32_t timeout_ms);

/* Image printing API - now included in main library */
#include "escpos_image.h"

#ifdef __cplusplus
}
#endif
