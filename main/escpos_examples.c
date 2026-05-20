#include <stdio.h>

#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "escpos_examples.h"

static const char *TAG = "escpos_examples";

extern const uint8_t image_bmp_start[] asm("_binary_image_bmp_start");
extern const uint8_t image_bmp_end[] asm("_binary_image_bmp_end");

#define IMAGE_BMP_SIZE ((size_t)(image_bmp_end - image_bmp_start))

static esp_err_t write_title(escpos_printer_t *printer, const char *title)
{
    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "================================================\r\n"),
                        TAG, "Failed to write title rule");
    ESP_RETURN_ON_ERROR(escpos_write_text_aligned(printer, title, ESCPOS_ALIGN_CENTER),
                        TAG, "Failed to write title");
    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "\r\n"),
                        TAG, "Failed to write title newline");
    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "================================================\r\n\r\n"),
                        TAG, "Failed to write title rule");
    return ESP_OK;
}

esp_err_t example_text_formatting(escpos_printer_t *printer)
{
    ESP_LOGI(TAG, "Running text formatting example");
    ESP_RETURN_ON_ERROR(write_title(printer, "TEXT FORMATTING"), TAG, "Title failed");

    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "Plain text\r\n"),
                        TAG, "Plain text failed");

    ESP_RETURN_ON_ERROR(escpos_set_bold(printer, true), TAG, "Bold on failed");
    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "Bold text\r\n"),
                        TAG, "Bold text failed");
    ESP_RETURN_ON_ERROR(escpos_set_bold(printer, false), TAG, "Bold off failed");

    ESP_RETURN_ON_ERROR(escpos_set_underline(printer, 1), TAG, "Underline on failed");
    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "Underlined text\r\n"),
                        TAG, "Underline text failed");
    ESP_RETURN_ON_ERROR(escpos_set_underline(printer, 0), TAG, "Underline off failed");

    ESP_RETURN_ON_ERROR(escpos_set_font(printer, ESCPOS_FONT_B), TAG, "Font B failed");
    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "Font B text\r\n"),
                        TAG, "Font B text failed");
    ESP_RETURN_ON_ERROR(escpos_set_font(printer, ESCPOS_FONT_A), TAG, "Font A failed");

    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "\r\nText size:\r\n"),
                        TAG, "Text size label failed");
    ESP_RETURN_ON_ERROR(escpos_set_text_size(printer, 2, 1), TAG, "Double width failed");
    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "Double width\r\n"),
                        TAG, "Double width text failed");
    ESP_RETURN_ON_ERROR(escpos_set_text_size(printer, 1, 2), TAG, "Double height failed");
    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "Double height\r\n"),
                        TAG, "Double height text failed");
    ESP_RETURN_ON_ERROR(escpos_set_text_size(printer, 2, 2), TAG, "Double both failed");
    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "Double both\r\n"),
                        TAG, "Double both text failed");
    ESP_RETURN_ON_ERROR(escpos_reset_text_format(printer), TAG, "Format reset failed");

    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "\r\nHost-side alignment:\r\n"),
                        TAG, "Alignment label failed");
    ESP_RETURN_ON_ERROR(escpos_write_text_aligned(printer, "LEFT", ESCPOS_ALIGN_LEFT),
                        TAG, "Left align failed");
    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "\r\n"), TAG, "Newline failed");
    ESP_RETURN_ON_ERROR(escpos_write_text_aligned(printer, "CENTER", ESCPOS_ALIGN_CENTER),
                        TAG, "Center align failed");
    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "\r\n"), TAG, "Newline failed");
    ESP_RETURN_ON_ERROR(escpos_write_text_aligned(printer, "RIGHT", ESCPOS_ALIGN_RIGHT),
                        TAG, "Right align failed");
    return escpos_write_text(printer, "\r\n\r\n");
}

esp_err_t example_receipt_columns(escpos_printer_t *printer)
{
    ESP_LOGI(TAG, "Running receipt columns example");
    ESP_RETURN_ON_ERROR(write_title(printer, "RECEIPT COLUMNS"), TAG, "Title failed");

    ESP_RETURN_ON_ERROR(escpos_write_3_columns(printer, "ITEM", "QTY", "PRICE"),
                        TAG, "3-column header failed");
    ESP_RETURN_ON_ERROR(escpos_write_3_columns(printer, "Coffee", "2", "7.00"),
                        TAG, "3-column item failed");
    ESP_RETURN_ON_ERROR(escpos_write_3_columns(printer, "Sandwich", "1", "6.25"),
                        TAG, "3-column item failed");
    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "\r\n"),
                        TAG, "Column spacing failed");

    ESP_RETURN_ON_ERROR(escpos_write_2_columns(printer, "Subtotal", "$13.25"),
                        TAG, "2-column subtotal failed");
    ESP_RETURN_ON_ERROR(escpos_write_2_columns(printer, "Tax", "$1.06"),
                        TAG, "2-column tax failed");
    ESP_RETURN_ON_ERROR(escpos_write_2_columns(printer, "TOTAL", "$14.31"),
                        TAG, "2-column total failed");
    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "\r\n"),
                        TAG, "Column spacing failed");

    ESP_RETURN_ON_ERROR(escpos_write_4_columns(printer, "ITEM", "QTY", "RATE", "TOTAL"),
                        TAG, "4-column header failed");
    ESP_RETURN_ON_ERROR(escpos_write_4_columns(printer, "Coffee", "2", "3.50", "7.00"),
                        TAG, "4-column item failed");
    return escpos_write_text(printer, "\r\n");
}

esp_err_t example_image_printing(escpos_printer_t *printer)
{
    ESP_LOGI(TAG, "Running image printing example");
    ESP_RETURN_ON_ERROR(write_title(printer, "IMAGE PRINTING"), TAG, "Title failed");

    escpos_image_params_t params = escpos_image_get_default_params();
    params.dither_mode = ESCPOS_DITHER_FLOYD_STEINBERG;
    params.align = ESCPOS_ALIGN_CENTER;
    params.max_width = escpos_get_printer_width_dots(printer);
    params.threshold = 40; /* Lower threshold for darker print (0-255) */

    escpos_image_t image = {0};
    esp_err_t err = escpos_image_load_from_buffer(image_bmp_start,
                                                  IMAGE_BMP_SIZE,
                                                  &params,
                                                  &image);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Image load failed: %s", esp_err_to_name(err));
        escpos_write_text(printer, "Image load failed\r\n\r\n");
        return err;
    }

    char line[64];
    snprintf(line, sizeof(line), "Image: %ux%u\r\n\r\n", image.width, image.height);
    ESP_RETURN_ON_ERROR(escpos_write_text(printer, line), TAG, "Image info failed");

    err = escpos_print_image(printer, &image, ESCPOS_IMAGE_MODE_NORMAL);
    escpos_image_free(&image);
    ESP_RETURN_ON_ERROR(err, TAG, "Image print failed");
    return escpos_write_text(printer, "\r\n\r\n");
}

esp_err_t example_barcode_printing(escpos_printer_t *printer)
{
    ESP_LOGI(TAG, "Running barcode printing example");
    ESP_RETURN_ON_ERROR(write_title(printer, "BARCODE PRINTING"), TAG, "Title failed");

    escpos_barcode_config_t code128 = escpos_barcode_get_default_config(ESCPOS_BARCODE_CODE128);
    code128.align = ESCPOS_ALIGN_CENTER;
    code128.hri = ESCPOS_BARCODE_HRI_BELOW;
    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "CODE128 center:\r\n"),
                        TAG, "Code128 label failed");
    ESP_RETURN_ON_ERROR(escpos_print_barcode(printer, "TXN12345", &code128),
                        TAG, "Code128 print failed");
    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "\r\n\r\n"), TAG, "Spacing failed");

    escpos_barcode_config_t ean13 = escpos_barcode_get_default_config(ESCPOS_BARCODE_EAN13);
    ean13.align = ESCPOS_ALIGN_CENTER;
    ean13.width = 3;
    ean13.height = 70;
    ean13.hri = ESCPOS_BARCODE_HRI_BELOW;
    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "EAN13 raster center:\r\n"),
                        TAG, "EAN13 label failed");
    ESP_RETURN_ON_ERROR(escpos_print_barcode(printer, "5901234123457", &ean13),
                        TAG, "EAN13 print failed");
    return escpos_write_text(printer, "\r\n\r\n");
}

esp_err_t example_qr_printing(escpos_printer_t *printer)
{
    ESP_LOGI(TAG, "Running QR printing example");
    ESP_RETURN_ON_ERROR(write_title(printer, "QR PRINTING"), TAG, "Title failed");

    escpos_qr_config_t qr = escpos_qr_get_default_config();
    qr.size = 6;
    qr.ec_level = ESCPOS_QR_EC_M;

    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "QR left:\r\n"), TAG, "QR label failed");
    qr.align = ESCPOS_ALIGN_LEFT;
    ESP_RETURN_ON_ERROR(escpos_print_qr(printer, "https://example.com/left", &qr),
                        TAG, "QR left failed");
    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "\r\n\r\n"), TAG, "Spacing failed");

    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "QR center:\r\n"), TAG, "QR label failed");
    qr.align = ESCPOS_ALIGN_CENTER;
    ESP_RETURN_ON_ERROR(escpos_print_qr(printer, "https://example.com/center", &qr),
                        TAG, "QR center failed");
    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "\r\n\r\n"), TAG, "Spacing failed");

    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "QR right:\r\n"), TAG, "QR label failed");
    qr.align = ESCPOS_ALIGN_RIGHT;
    ESP_RETURN_ON_ERROR(escpos_print_qr(printer, "https://example.com/right", &qr),
                        TAG, "QR right failed");
    return escpos_write_text(printer, "\r\n\r\n");
}

esp_err_t example_raw_commands(escpos_printer_t *printer)
{
    ESP_LOGI(TAG, "Running raw command example");
    ESP_RETURN_ON_ERROR(write_title(printer, "RAW COMMANDS"), TAG, "Title failed");

    const uint8_t bold_on[] = {0x1B, 0x45, 0x01};
    const uint8_t bold_off[] = {0x1B, 0x45, 0x00};

    ESP_RETURN_ON_ERROR(escpos_write_command(printer, bold_on, sizeof(bold_on)),
                        TAG, "Raw bold-on failed");
    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "This line used raw ESC E 1\r\n"),
                        TAG, "Raw command text failed");
    ESP_RETURN_ON_ERROR(escpos_write_command(printer, bold_off, sizeof(bold_off)),
                        TAG, "Raw bold-off failed");
    return escpos_write_text(printer, "\r\n");
}

esp_err_t example_paper_control(escpos_printer_t *printer)
{
    ESP_LOGI(TAG, "Running paper control example");
    ESP_RETURN_ON_ERROR(write_title(printer, "PAPER CONTROL"), TAG, "Title failed");
    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "Line before feed\r\n"),
                        TAG, "Paper text failed");
    ESP_RETURN_ON_ERROR(escpos_feed_line(printer), TAG, "Single feed failed");
    ESP_RETURN_ON_ERROR(escpos_feed_lines(printer, 3), TAG, "Feed failed");
    return escpos_write_text(printer, "Line after feed\r\n\r\n");
}

esp_err_t example_audio_feedback(escpos_printer_t *printer)
{
    ESP_LOGI(TAG, "Running audio feedback example");
    ESP_RETURN_ON_ERROR(write_title(printer, "AUDIO FEEDBACK"), TAG, "Title failed");
    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "Beep command...\r\n"),
                        TAG, "Beep label failed");
    esp_err_t err = escpos_beep(printer);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Beep failed: %s", esp_err_to_name(err));
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(500));
    return escpos_write_text(printer, "Beep complete\r\n\r\n");
}

esp_err_t example_complete_receipt(escpos_printer_t *printer)
{
    ESP_LOGI(TAG, "Running complete receipt example");
    ESP_RETURN_ON_ERROR(write_title(printer, "SAMPLE STORE"), TAG, "Title failed");

    ESP_RETURN_ON_ERROR(escpos_write_text(printer,
                        "Date: May 19, 2026\r\n"
                        "Transaction: TXN12345\r\n\r\n"),
                        TAG, "Receipt header failed");

    ESP_ERROR_CHECK(escpos_set_bold(printer, true));
    ESP_RETURN_ON_ERROR(escpos_write_4_columns(printer, "ITEM", "QTY", "RATE", "TOTAL"),
                        TAG, "Receipt table header failed");
    ESP_ERROR_CHECK(escpos_set_bold(printer, false));
    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "------------------------------------------------\r\n"),
                        TAG, "Receipt rule failed");
    ESP_RETURN_ON_ERROR(escpos_write_4_columns(printer, "Coffee", "2", "3.50", "7.00"),
                        TAG, "Receipt item failed");
    ESP_RETURN_ON_ERROR(escpos_write_4_columns(printer, "Sandwich", "1", "6.25", "6.25"),
                        TAG, "Receipt item failed");
    ESP_RETURN_ON_ERROR(escpos_write_4_columns(printer, "Cookie", "3", "1.25", "3.75"),
                        TAG, "Receipt item failed");
    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "------------------------------------------------\r\n"),
                        TAG, "Receipt rule failed");
    ESP_RETURN_ON_ERROR(escpos_write_2_columns(printer, "Subtotal", "$17.00"),
                        TAG, "Receipt subtotal failed");
    ESP_RETURN_ON_ERROR(escpos_write_2_columns(printer, "Tax", "$1.36"),
                        TAG, "Receipt tax failed");
    ESP_RETURN_ON_ERROR(escpos_write_2_columns(printer, "TOTAL", "$18.36"),
                        TAG, "Receipt total failed");
    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "\r\n"),
                        TAG, "Receipt spacing failed");

    escpos_qr_config_t qr = escpos_qr_get_default_config();
    qr.align = ESCPOS_ALIGN_CENTER;
    ESP_RETURN_ON_ERROR(escpos_write_text_aligned(printer, "Scan receipt", ESCPOS_ALIGN_CENTER),
                        TAG, "Receipt QR label failed");
    ESP_RETURN_ON_ERROR(escpos_write_text(printer, "\r\n"), TAG, "Newline failed");
    ESP_RETURN_ON_ERROR(escpos_print_qr(printer, "receipt.example/TXN12345", &qr),
                        TAG, "Receipt QR failed");

    return escpos_write_text(printer, "\r\nThank you!\r\n\r\n");
}
