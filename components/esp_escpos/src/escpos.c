#include <string.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_check.h"

#include "escpos_private.h"
#include "escpos_config.h"

static const char *TAG = "escpos";

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

    p->usb_ctx = usb_ctx;
    p->write_fn = escpos_usb_write;
    p->transport_ctx = usb_ctx;
    p->text_width_multiplier = 1;
    p->text_height_multiplier = 1;

    *printer = p;
    ESP_LOGI(TAG, "USB printer connected");
    return ESP_OK;
}

void escpos_destroy(escpos_printer_t *printer)
{
    if (!printer) {
        return;
    }

    if (printer->usb_ctx) {
        escpos_usb_close(printer->usb_ctx);
        printer->usb_ctx = NULL;
    }

    free(printer);
}

esp_err_t escpos_write_raw(escpos_printer_t *printer, const uint8_t *data, size_t len)
{
    ESP_RETURN_ON_FALSE(printer && printer->write_fn, ESP_ERR_INVALID_ARG, TAG,
                        "Invalid printer handle");
    ESP_RETURN_ON_FALSE(data && len > 0, ESP_ERR_INVALID_ARG, TAG, "Invalid data");

    return printer->write_fn(data, len, printer->transport_ctx);
}

esp_err_t escpos_write_command(escpos_printer_t *printer, const uint8_t *data, size_t len)
{
    return escpos_write_raw(printer, data, len);
}

esp_err_t escpos_write_text(escpos_printer_t *printer, const char *text)
{
    ESP_RETURN_ON_FALSE(printer && text, ESP_ERR_INVALID_ARG, TAG, "NULL argument");
    return escpos_write_raw(printer, (const uint8_t *)text, strlen(text));
}

uint16_t escpos_get_printer_width_dots(const escpos_printer_t *printer)
{
    return printer ? printer->width_dots : 0;
}

uint16_t escpos_get_printer_width_chars(const escpos_printer_t *printer)
{
    return printer ? printer->width_chars : 0;
}

bool escpos_is_connected(const escpos_printer_t *printer)
{
    if (!printer || !printer->usb_ctx) {
        return false;
    }

    return escpos_usb_is_connected(printer->usb_ctx);
}

esp_err_t escpos_wait_disconnect(escpos_printer_t *printer, uint32_t timeout_ms)
{
    ESP_RETURN_ON_FALSE(printer && printer->usb_ctx, ESP_ERR_INVALID_ARG, TAG,
                        "Invalid printer handle");
    return escpos_usb_wait_disconnect(printer->usb_ctx, timeout_ms);
}
