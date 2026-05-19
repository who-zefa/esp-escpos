#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ─────────────────────────────────────────────────────────────────────────────
 * Transport type
 * ───────────────────────────────────────────────────────────────────────────*/
typedef enum {
    ESCPOS_TRANSPORT_USB     = 0,
    ESCPOS_TRANSPORT_UART    = 1,
    ESCPOS_TRANSPORT_NETWORK = 2,
    ESCPOS_TRANSPORT_BLE     = 3,
} escpos_transport_type_t;

/* ─────────────────────────────────────────────────────────────────────────────
 * Text formatting
 * ───────────────────────────────────────────────────────────────────────────*/
typedef enum {
    ESCPOS_ALIGN_LEFT   = 0,
    ESCPOS_ALIGN_CENTER = 1,
    ESCPOS_ALIGN_RIGHT  = 2,
} escpos_align_t;

typedef enum {
    ESCPOS_FONT_A = 0,   /* Standard font  */
    ESCPOS_FONT_B = 1,   /* Condensed font */
    ESCPOS_FONT_C = 2,   /* Font C (if supported) */
} escpos_font_t;

/* Bitmask – combine with OR */
typedef enum {
    ESCPOS_STYLE_NORMAL        = 0x00,
    ESCPOS_STYLE_BOLD          = 0x08,
    ESCPOS_STYLE_DOUBLE_HEIGHT = 0x10,
    ESCPOS_STYLE_DOUBLE_WIDTH  = 0x20,
    ESCPOS_STYLE_UNDERLINE     = 0x80,
} escpos_style_t;

typedef struct {
    const char *text;
    uint8_t width;                 /* Column width in characters */
    escpos_align_t align;
} escpos_text_column_t;

/* ─────────────────────────────────────────────────────────────────────────────
 * Barcode / QR types
 * ───────────────────────────────────────────────────────────────────────────*/
typedef enum {
    ESCPOS_BARCODE_UPC_A  = 65,
    ESCPOS_BARCODE_UPC_E  = 66,
    ESCPOS_BARCODE_EAN13  = 67,
    ESCPOS_BARCODE_EAN8   = 68,
    ESCPOS_BARCODE_CODE39 = 69,
    ESCPOS_BARCODE_I25    = 70,
    ESCPOS_BARCODE_CODEBAR= 71,
    ESCPOS_BARCODE_CODE93 = 72,
    ESCPOS_BARCODE_CODE128= 73,
} escpos_barcode_type_t;

typedef enum {
    ESCPOS_BARCODE_HRI_NONE  = 0,
    ESCPOS_BARCODE_HRI_ABOVE = 1,
    ESCPOS_BARCODE_HRI_BELOW = 2,
    ESCPOS_BARCODE_HRI_BOTH  = 3,
} escpos_barcode_hri_t;

typedef struct {
    escpos_barcode_type_t type;
    uint8_t width;                 /* Module width, usually 2..6 */
    uint8_t height;                /* Barcode height in dots, 1..255 */
    escpos_barcode_hri_t hri;
    escpos_font_t hri_font;
    escpos_align_t align;
} escpos_barcode_config_t;

typedef enum {
    ESCPOS_QR_EC_L = 48,   /* ~7%  */
    ESCPOS_QR_EC_M = 49,   /* ~15% */
    ESCPOS_QR_EC_Q = 50,   /* ~25% */
    ESCPOS_QR_EC_H = 51,   /* ~30% */
} escpos_qr_ec_t;

typedef struct {
    uint8_t size;                  /* Dot size per QR module, 1..16 */
    escpos_qr_ec_t ec_level;       /* Host QR encoder currently supports ESCPOS_QR_EC_M */
    escpos_align_t align;
} escpos_qr_config_t;

/* ─────────────────────────────────────────────────────────────────────────────
 * Paper cut
 * ───────────────────────────────────────────────────────────────────────────*/
typedef enum {
    ESCPOS_CUT_FULL    = 0,
    ESCPOS_CUT_PARTIAL = 1,
} escpos_cut_type_t;

/* ─────────────────────────────────────────────────────────────────────────────
 * Image / raster
 * ───────────────────────────────────────────────────────────────────────────*/
typedef enum {
    ESCPOS_IMAGE_MODE_NORMAL     = 0,
    ESCPOS_IMAGE_MODE_DOUBLE_W   = 1,
    ESCPOS_IMAGE_MODE_DOUBLE_H   = 2,
    ESCPOS_IMAGE_MODE_DOUBLE_WH  = 3,
} escpos_image_mode_t;

/* ─────────────────────────────────────────────────────────────────────────────
 * Generic write callback – implemented per transport
 * ───────────────────────────────────────────────────────────────────────────*/
typedef esp_err_t (*escpos_write_fn_t)(const uint8_t *data, size_t len, void *ctx);

/* ─────────────────────────────────────────────────────────────────────────────
 * Printer handle (opaque to callers)
 * ───────────────────────────────────────────────────────────────────────────*/
typedef struct escpos_printer_s *escpos_handle_t;

#ifdef __cplusplus
}
#endif
