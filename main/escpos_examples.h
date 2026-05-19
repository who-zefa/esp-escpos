#pragma once

#include "esp_err.h"
#include "escpos.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t example_text_formatting(escpos_printer_t *printer);
esp_err_t example_receipt_columns(escpos_printer_t *printer);
esp_err_t example_image_printing(escpos_printer_t *printer);
esp_err_t example_barcode_printing(escpos_printer_t *printer);
esp_err_t example_qr_printing(escpos_printer_t *printer);
esp_err_t example_raw_commands(escpos_printer_t *printer);
esp_err_t example_paper_control(escpos_printer_t *printer);
esp_err_t example_audio_feedback(escpos_printer_t *printer);
esp_err_t example_complete_receipt(escpos_printer_t *printer);

#ifdef __cplusplus
}
#endif
