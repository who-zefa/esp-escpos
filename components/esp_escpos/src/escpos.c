#include <string.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "escpos.h"
#include "escpos_config.h"
#include "transport/escpos_usb.h"

static const char *TAG = "escpos";

/* ─────────────────────────────────────────────────────────────────────────────
 * Printer handle (internal structure)
 * ───────────────────────────────────────────────────────────────────────────*/
struct escpos_printer_s {
    escpos_usb_ctx_t *usb_ctx;
    escpos_write_fn_t write_fn;
    void *transport_ctx;
    uint16_t width_dots;
    uint16_t width_chars;
};

/* ─────────────────────────────────────────────────────────────────────────────
 * Core API implementation
 * ───────────────────────────────────────────────────────────────────────────*/

esp_err_t escpos_init(void)
{
    ESP_LOGI(TAG, "Initializing ESC/POS library");
    return escpos_usb_driver_install();
}

esp_err_t escpos_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing ESC/POS library");
    return escpos_usb_driver_uninstall();
}

esp_err_t escpos_new_usb(escpos_printer_t **printer)
{
    return escpos_new_usb_with_ids(ESCPOS_USB_VID, ESCPOS_USB_PID, printer);
}

esp_err_t escpos_new_usb_with_ids(uint16_t vid, uint16_t pid, escpos_printer_t **printer)
{
    ESP_RETURN_ON_FALSE(printer, ESP_ERR_INVALID_ARG, TAG, "NULL printer pointer");

    escpos_printer_t *p = calloc(1, sizeof(escpos_printer_t));
    ESP_RETURN_ON_FALSE(p, ESP_ERR_NO_MEM, TAG, "Failed to allocate printer");

    escpos_usb_config_t usb_cfg = {
        .vid                 = vid,
        .pid                 = pid,
        .pid_alt             = 0,
        .connect_timeout_ms  = ESCPOS_USB_CONNECT_TIMEOUT_MS,
        .out_buffer_size     = ESCPOS_USB_OUT_BUFFER_SIZE,
        .in_buffer_size      = ESCPOS_USB_IN_BUFFER_SIZE,
        .interface_idx       = 0,
        .set_line_coding     = true,
        .baud_rate           = 9600,
    };

    escpos_usb_ctx_t *usb_ctx = NULL;
    esp_err_t err = escpos_usb_open(&usb_cfg, &usb_ctx);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open USB printer: %s", esp_err_to_name(err));
        free(p);
        return err;
    }

    p->usb_ctx        = usb_ctx;
    p->write_fn       = escpos_usb_write;
    p->transport_ctx  = usb_ctx;
    p->width_dots     = ESCPOS_PRINTER_WIDTH_DOTS;
    p->width_chars    = ESCPOS_PAPER_WIDTH;

    *printer = p;
    ESP_LOGI(TAG, "USB printer connected");
    return ESP_OK;
}

void escpos_destroy(escpos_printer_t *printer)
{
    if (!printer) return;

    if (printer->usb_ctx) {
        escpos_usb_close(printer->usb_ctx);
        printer->usb_ctx = NULL;
    }

    free(printer);
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Raw write and text output
 * ───────────────────────────────────────────────────────────────────────────*/

esp_err_t escpos_write_raw(escpos_printer_t *printer, const uint8_t *data, size_t len)
{
    ESP_RETURN_ON_FALSE(printer && printer->write_fn, ESP_ERR_INVALID_ARG, TAG,
                        "Invalid printer handle");
    ESP_RETURN_ON_FALSE(data && len > 0, ESP_ERR_INVALID_ARG, TAG, "Invalid data");

    return printer->write_fn(data, len, printer->transport_ctx);
}

esp_err_t escpos_write_text(escpos_printer_t *printer, const char *text)
{
    ESP_RETURN_ON_FALSE(printer && text, ESP_ERR_INVALID_ARG, TAG, "NULL argument");
    return escpos_write_raw(printer, (const uint8_t *)text, strlen(text));
}

esp_err_t escpos_set_align(escpos_printer_t *printer, escpos_align_t align)
{
    ESP_RETURN_ON_FALSE(printer && align <= ESCPOS_ALIGN_RIGHT,
                        ESP_ERR_INVALID_ARG, TAG, "Invalid alignment");

    uint8_t cmd[] = {0x1B, 0x61, (uint8_t)align};  /* ESC a n */
    return escpos_write_raw(printer, cmd, sizeof(cmd));
}

esp_err_t escpos_set_bold(escpos_printer_t *printer, bool enabled)
{
    uint8_t cmd[] = {0x1B, 0x45, enabled ? 1 : 0};  /* ESC E n */
    return escpos_write_raw(printer, cmd, sizeof(cmd));
}

esp_err_t escpos_set_underline(escpos_printer_t *printer, uint8_t thickness)
{
    ESP_RETURN_ON_FALSE(thickness <= 2, ESP_ERR_INVALID_ARG, TAG,
                        "Underline thickness must be 0, 1, or 2");

    uint8_t cmd[] = {0x1B, 0x2D, thickness};  /* ESC - n */
    return escpos_write_raw(printer, cmd, sizeof(cmd));
}

esp_err_t escpos_set_font(escpos_printer_t *printer, escpos_font_t font)
{
    ESP_RETURN_ON_FALSE(font <= ESCPOS_FONT_C, ESP_ERR_INVALID_ARG, TAG,
                        "Invalid font");

    uint8_t cmd[] = {0x1B, 0x4D, (uint8_t)font};  /* ESC M n */
    return escpos_write_raw(printer, cmd, sizeof(cmd));
}

esp_err_t escpos_set_text_size(escpos_printer_t *printer, uint8_t width, uint8_t height)
{
    ESP_RETURN_ON_FALSE(width >= 1 && width <= 8 && height >= 1 && height <= 8,
                        ESP_ERR_INVALID_ARG, TAG,
                        "Text width and height multipliers must be 1..8");

    uint8_t size = (uint8_t)(((width - 1) << 4) | (height - 1));
    uint8_t cmd[] = {0x1D, 0x21, size};  /* GS ! n */
    return escpos_write_raw(printer, cmd, sizeof(cmd));
}

esp_err_t escpos_set_style(escpos_printer_t *printer, escpos_style_t style)
{
    uint8_t cmd[] = {0x1B, 0x21, (uint8_t)style};  /* ESC ! n */
    return escpos_write_raw(printer, cmd, sizeof(cmd));
}

esp_err_t escpos_reset_text_format(escpos_printer_t *printer)
{
    ESP_RETURN_ON_ERROR(escpos_set_align(printer, ESCPOS_ALIGN_LEFT), TAG,
                        "Failed to reset alignment");
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

esp_err_t escpos_set_printer_width(escpos_printer_t *printer, uint16_t dots, uint16_t chars)
{
    ESP_RETURN_ON_FALSE(printer && dots > 0 && chars > 0,
                        ESP_ERR_INVALID_ARG, TAG, "Invalid printer width");

    printer->width_dots = dots;
    printer->width_chars = chars;
    return ESP_OK;
}

uint16_t escpos_get_printer_width_dots(const escpos_printer_t *printer)
{
    return printer ? printer->width_dots : 0;
}

uint16_t escpos_get_printer_width_chars(const escpos_printer_t *printer)
{
    return printer ? printer->width_chars : 0;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Printer control commands
 * ───────────────────────────────────────────────────────────────────────────*/

esp_err_t escpos_init_printer(escpos_printer_t *printer)
{
    static const uint8_t cmd[] = {0x1B, 0x40};  /* ESC @ */
    return escpos_write_raw(printer, cmd, sizeof(cmd));
}

esp_err_t escpos_feed_line(escpos_printer_t *printer)
{
    static const uint8_t lf = 0x0A;
    return escpos_write_raw(printer, &lf, 1);
}

esp_err_t escpos_feed_lines(escpos_printer_t *printer, uint8_t n)
{
    ESP_RETURN_ON_FALSE(printer && n > 0 && n < 256, ESP_ERR_INVALID_ARG, TAG,
                        "Invalid feed count");

    uint8_t cmd[] = {0x1B, 0x64, n};  /* ESC d n */
    return escpos_write_raw(printer, cmd, sizeof(cmd));
}

esp_err_t escpos_cut_paper(escpos_printer_t *printer)
{
    static const uint8_t cmd[] = {0x1D, 0x56, 0x00};  /* GS V 0 */
    return escpos_write_raw(printer, cmd, sizeof(cmd));
}

esp_err_t escpos_cut_paper_partial(escpos_printer_t *printer)
{
    static const uint8_t cmd[] = {0x1D, 0x56, 0x01};  /* GS V 1 */
    return escpos_write_raw(printer, cmd, sizeof(cmd));
}

esp_err_t escpos_beep(escpos_printer_t *printer)
{
    static const uint8_t cmd[] = {0x1B, 0x42, 0x03, 0x02};  /* ESC B n t */
    return escpos_write_raw(printer, cmd, sizeof(cmd));
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Connection management
 * ───────────────────────────────────────────────────────────────────────────*/

bool escpos_is_connected(const escpos_printer_t *printer)
{
    if (!printer || !printer->usb_ctx) return false;
    return escpos_usb_is_connected(printer->usb_ctx);
}

esp_err_t escpos_wait_disconnect(escpos_printer_t *printer, uint32_t timeout_ms)
{
    ESP_RETURN_ON_FALSE(printer && printer->usb_ctx, ESP_ERR_INVALID_ARG, TAG,
                        "Invalid printer handle");
    return escpos_usb_wait_disconnect(printer->usb_ctx, timeout_ms);
}
