#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "escpos.h"
#include "transport/escpos_usb.h"

#ifdef __cplusplus
extern "C" {
#endif

struct escpos_printer_s {
    escpos_usb_ctx_t *usb_ctx;
    escpos_write_fn_t write_fn;
    void *transport_ctx;
    uint16_t width_dots;
    uint16_t width_chars;
    uint8_t text_width_multiplier;
    uint8_t text_height_multiplier;
};

esp_err_t escpos_require_paper_width(const escpos_printer_t *printer);
uint16_t escpos_get_effective_width_chars(const escpos_printer_t *printer);
esp_err_t escpos_write_spaces(escpos_printer_t *printer, uint16_t count);
esp_err_t escpos_write_graphic_alignment(escpos_printer_t *printer,
                                         uint16_t graphic_width_dots,
                                         escpos_align_t align);

#ifdef __cplusplus
}
#endif
