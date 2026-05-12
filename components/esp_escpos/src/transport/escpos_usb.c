/**
 * @file escpos_usb.c
 * @brief USB CDC-ACM transport for esp32_escpos
 *
 * Wraps esp-idf usb_host + cdc_acm_host into the escpos_write_fn_t interface.
 * Based on the working prototype but refactored for re-entrancy, proper error
 * propagation, and clean lifecycle management.
 */

#include <string.h>

#include "esp_log.h"
#include "esp_check.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "usb/usb_host.h"
#include "usb/cdc_acm_host.h"

#include "transport/escpos_usb.h"
#include "escpos_config.h"

static const char *TAG = "escpos_usb";

/* ─────────────────────────────────────────────────────────────────────────────
 * Module-level driver state
 * Only one USB host + CDC-ACM driver instance exists per MCU.
 * ───────────────────────────────────────────────────────────────────────────*/
#define USB_LIB_TASK_STACK      4096
#define USB_LIB_TASK_PRIORITY   20

static struct {
    bool            installed;
    TaskHandle_t    lib_task_handle;
    SemaphoreHandle_t init_mutex;        /* Serialise install/uninstall calls */
} s_driver = {
    .installed       = false,
    .lib_task_handle = NULL,
    .init_mutex      = NULL,
};

/* ─────────────────────────────────────────────────────────────────────────────
 * Per-connection context
 * ───────────────────────────────────────────────────────────────────────────*/
struct escpos_usb_ctx_s {
    cdc_acm_dev_hdl_t   cdc_hdl;
    SemaphoreHandle_t   disconnect_sem;
    volatile bool       connected;
    uint32_t            tx_timeout_ms;
};

/* ─────────────────────────────────────────────────────────────────────────────
 * USB host library task
 * Pumps the USB host event loop – must run continuously.
 * ───────────────────────────────────────────────────────────────────────────*/
static void usb_lib_task(void *arg)
{
    ESP_LOGI(TAG, "USB host library task started");

    while (1) {
        uint32_t event_flags;
        esp_err_t err = usb_host_lib_handle_events(portMAX_DELAY, &event_flags);

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "usb_host_lib_handle_events error: %s", esp_err_to_name(err));
            continue;
        }

        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            ESP_LOGI(TAG, "No more USB clients – freeing devices");
            usb_host_device_free_all();
        }

        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
            ESP_LOGI(TAG, "All USB devices freed");
        }
    }

    vTaskDelete(NULL);
}

/* ─────────────────────────────────────────────────────────────────────────────
 * CDC-ACM event callback (per device)
 * ───────────────────────────────────────────────────────────────────────────*/
static void cdc_event_cb(const cdc_acm_host_dev_event_data_t *event, void *user_ctx)
{
    escpos_usb_ctx_t *ctx = (escpos_usb_ctx_t *)user_ctx;

    switch (event->type) {

    case CDC_ACM_HOST_ERROR:
        ESP_LOGE(TAG, "CDC-ACM error: %d", event->data.error);
        break;

    case CDC_ACM_HOST_DEVICE_DISCONNECTED:
        ESP_LOGW(TAG, "Printer disconnected");
        ctx->connected = false;
        cdc_acm_host_close(event->data.cdc_hdl);
        ctx->cdc_hdl = NULL;
        xSemaphoreGive(ctx->disconnect_sem);
        break;

    case CDC_ACM_HOST_SERIAL_STATE:
        ESP_LOGD(TAG, "Serial state: 0x%04X", event->data.serial_state.val);
        break;

    default:
        ESP_LOGD(TAG, "Unhandled CDC event type: %d", event->type);
        break;
    }
}

/* ─────────────────────────────────────────────────────────────────────────────
 * RX data callback – printers rarely send data; log and discard.
 * ───────────────────────────────────────────────────────────────────────────*/
static bool cdc_rx_cb(const uint8_t *data, size_t data_len, void *arg)
{
    ESP_LOGD(TAG, "RX %u bytes from printer (ignored)", (unsigned)data_len);
    (void)data;
    (void)arg;
    return true;   /* Return true = data consumed */
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Public: install driver stack
 * ───────────────────────────────────────────────────────────────────────────*/
esp_err_t escpos_usb_driver_install(void)
{
    /* Lazy-create the mutex on first call */
    if (s_driver.init_mutex == NULL) {
        s_driver.init_mutex = xSemaphoreCreateMutex();
        ESP_RETURN_ON_FALSE(s_driver.init_mutex, ESP_ERR_NO_MEM, TAG,
                            "Failed to create init mutex");
    }

    xSemaphoreTake(s_driver.init_mutex, portMAX_DELAY);

    if (s_driver.installed) {
        ESP_LOGD(TAG, "USB driver already installed");
        xSemaphoreGive(s_driver.init_mutex);
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Installing USB host driver");

    const usb_host_config_t host_cfg = {
        .skip_phy_setup = false,
        .intr_flags     = ESP_INTR_FLAG_LEVEL1,
    };

    esp_err_t err = usb_host_install(&host_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "usb_host_install failed: %s", esp_err_to_name(err));
        xSemaphoreGive(s_driver.init_mutex);
        return err;
    }

    BaseType_t created = xTaskCreate(
        usb_lib_task,
        "escpos_usb_lib",
        USB_LIB_TASK_STACK,
        NULL,
        USB_LIB_TASK_PRIORITY,
        &s_driver.lib_task_handle
    );

    if (created != pdTRUE) {
        ESP_LOGE(TAG, "Failed to create USB lib task");
        usb_host_uninstall();
        xSemaphoreGive(s_driver.init_mutex);
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Installing CDC-ACM driver");
    err = cdc_acm_host_install(NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "cdc_acm_host_install failed: %s", esp_err_to_name(err));
        vTaskDelete(s_driver.lib_task_handle);
        s_driver.lib_task_handle = NULL;
        usb_host_uninstall();
        xSemaphoreGive(s_driver.init_mutex);
        return err;
    }

    s_driver.installed = true;
    xSemaphoreGive(s_driver.init_mutex);

    ESP_LOGI(TAG, "USB driver stack installed");
    return ESP_OK;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Public: uninstall driver stack
 * ───────────────────────────────────────────────────────────────────────────*/
esp_err_t escpos_usb_driver_uninstall(void)
{
    if (!s_driver.init_mutex) {
        return ESP_OK;
    }

    xSemaphoreTake(s_driver.init_mutex, portMAX_DELAY);

    if (!s_driver.installed) {
        xSemaphoreGive(s_driver.init_mutex);
        return ESP_OK;
    }

    cdc_acm_host_uninstall();

    if (s_driver.lib_task_handle) {
        vTaskDelete(s_driver.lib_task_handle);
        s_driver.lib_task_handle = NULL;
    }

    usb_host_uninstall();
    s_driver.installed = false;

    xSemaphoreGive(s_driver.init_mutex);
    ESP_LOGI(TAG, "USB driver stack uninstalled");
    return ESP_OK;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Public: open device
 * ───────────────────────────────────────────────────────────────────────────*/
esp_err_t escpos_usb_open(const escpos_usb_config_t *cfg, escpos_usb_ctx_t **ctx_out)
{
    ESP_RETURN_ON_FALSE(cfg && ctx_out, ESP_ERR_INVALID_ARG, TAG, "NULL argument");
    ESP_RETURN_ON_FALSE(s_driver.installed, ESP_ERR_INVALID_STATE, TAG,
                        "Call escpos_usb_driver_install() first");

    escpos_usb_ctx_t *ctx = calloc(1, sizeof(escpos_usb_ctx_t));
    ESP_RETURN_ON_FALSE(ctx, ESP_ERR_NO_MEM, TAG, "Failed to allocate USB ctx");

    ctx->disconnect_sem = xSemaphoreCreateBinary();
    if (!ctx->disconnect_sem) {
        free(ctx);
        return ESP_ERR_NO_MEM;
    }

    ctx->tx_timeout_ms = ESCPOS_TX_TIMEOUT_MS;
    ctx->connected     = false;

    const cdc_acm_host_device_config_t dev_cfg = {
        .connection_timeout_ms = cfg->connect_timeout_ms,
        .out_buffer_size       = cfg->out_buffer_size,
        .in_buffer_size        = cfg->in_buffer_size,
        .user_arg              = ctx,
        .event_cb              = cdc_event_cb,
        .data_cb               = cdc_rx_cb,
    };

    /* Try primary PID */
    esp_err_t err = cdc_acm_host_open(cfg->vid, cfg->pid,
                                       cfg->interface_idx, &dev_cfg,
                                       &ctx->cdc_hdl);

    /* Fall back to alternate PID if provided */
    if (err != ESP_OK && cfg->pid_alt != 0) {
        ESP_LOGD(TAG, "Primary PID 0x%04X not found, trying alt PID 0x%04X",
                 cfg->pid, cfg->pid_alt);
        err = cdc_acm_host_open(cfg->vid, cfg->pid_alt,
                                 cfg->interface_idx, &dev_cfg,
                                 &ctx->cdc_hdl);
    }

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Printer not found (err=%s)", esp_err_to_name(err));
        vSemaphoreDelete(ctx->disconnect_sem);
        free(ctx);
        return err;
    }

    ctx->connected = true;

    /* Print device descriptor to log for debugging */
    cdc_acm_host_desc_print(ctx->cdc_hdl);

    /* Optional: set CDC line coding */
    if (cfg->set_line_coding) {
        const cdc_acm_line_coding_t lc = {
            .dwDTERate   = cfg->baud_rate,
            .bCharFormat = 0,   /* 1 stop bit */
            .bParityType = 0,   /* No parity  */
            .bDataBits   = 8,
        };
        /* Best-effort: some printers ignore SetLineCoding entirely */
        cdc_acm_host_line_coding_set(ctx->cdc_hdl, &lc);
        cdc_acm_host_set_control_line_state(ctx->cdc_hdl, true, false);
    }

    /* Brief settle delay – printer needs a moment after enumeration */
    vTaskDelay(pdMS_TO_TICKS(500));

    *ctx_out = ctx;
    ESP_LOGI(TAG, "USB printer connected");
    return ESP_OK;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Public: close device
 * ───────────────────────────────────────────────────────────────────────────*/
void escpos_usb_close(escpos_usb_ctx_t *ctx)
{
    if (!ctx) return;

    if (ctx->cdc_hdl) {
        cdc_acm_host_close(ctx->cdc_hdl);
        ctx->cdc_hdl = NULL;
    }

    if (ctx->disconnect_sem) {
        vSemaphoreDelete(ctx->disconnect_sem);
        ctx->disconnect_sem = NULL;
    }

    ctx->connected = false;
    free(ctx);
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Public: write (escpos_write_fn_t implementation)
 * ───────────────────────────────────────────────────────────────────────────*/
esp_err_t escpos_usb_write(const uint8_t *data, size_t len, void *arg)
{
    escpos_usb_ctx_t *ctx = (escpos_usb_ctx_t *)arg;

    if (!ctx || !ctx->cdc_hdl) {
        ESP_LOGE(TAG, "write called on invalid/closed context");
        return ESP_ERR_INVALID_STATE;
    }

    if (!ctx->connected) {
        ESP_LOGE(TAG, "write called but printer is disconnected");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = cdc_acm_host_data_tx_blocking(ctx->cdc_hdl, data, len,
                                                   ctx->tx_timeout_ms);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "TX failed (%u bytes): %s", (unsigned)len,
                 esp_err_to_name(err));
    }

    return err;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Public: connection status
 * ───────────────────────────────────────────────────────────────────────────*/
bool escpos_usb_is_connected(const escpos_usb_ctx_t *ctx)
{
    return ctx && ctx->connected;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Public: wait for disconnect
 * ───────────────────────────────────────────────────────────────────────────*/
esp_err_t escpos_usb_wait_disconnect(escpos_usb_ctx_t *ctx, uint32_t timeout_ms)
{
    ESP_RETURN_ON_FALSE(ctx, ESP_ERR_INVALID_ARG, TAG, "NULL ctx");

    TickType_t ticks = (timeout_ms == portMAX_DELAY)
                        ? portMAX_DELAY
                        : pdMS_TO_TICKS(timeout_ms);

    if (xSemaphoreTake(ctx->disconnect_sem, ticks) == pdTRUE) {
        return ESP_OK;
    }

    return ESP_ERR_TIMEOUT;
}