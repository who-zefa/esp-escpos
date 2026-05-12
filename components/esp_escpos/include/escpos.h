#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct escpos_printer_s escpos_printer_t;

esp_err_t escpos_init(void);
esp_err_t escpos_deinit(void);
esp_err_t escpos_new_usb(escpos_printer_t **printer);
esp_err_t escpos_new_usb_with_ids(uint16_t vid, uint16_t pid, escpos_printer_t **printer);
void escpos_destroy(escpos_printer_t *printer);

esp_err_t escpos_init_printer(escpos_printer_t *printer);
esp_err_t escpos_write_raw(escpos_printer_t *printer, const uint8_t *data, size_t len);
esp_err_t escpos_write_text(escpos_printer_t *printer, const char *text);
esp_err_t escpos_feed_line(escpos_printer_t *printer);
esp_err_t escpos_feed_lines(escpos_printer_t *printer, uint8_t n);
esp_err_t escpos_cut_paper(escpos_printer_t *printer);
esp_err_t escpos_cut_paper_partial(escpos_printer_t *printer);
esp_err_t escpos_beep(escpos_printer_t *printer);
bool escpos_is_connected(const escpos_printer_t *printer);
esp_err_t escpos_wait_disconnect(escpos_printer_t *printer, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
