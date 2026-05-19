#pragma once

#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ─────────────────────────────────────────────────────────────────────────────
 * TX timeout (ms) – used by all blocking transports
 * ───────────────────────────────────────────────────────────────────────────*/
#ifndef CONFIG_ESCPOS_TX_TIMEOUT_MS
#define ESCPOS_TX_TIMEOUT_MS    3000
#else
#define ESCPOS_TX_TIMEOUT_MS    CONFIG_ESCPOS_TX_TIMEOUT_MS
#endif

/* ─────────────────────────────────────────────────────────────────────────────
 * Output buffer size (bytes) used by escpos_buf_t
 * ───────────────────────────────────────────────────────────────────────────*/
#ifndef CONFIG_ESCPOS_CMD_BUFFER_SIZE
#define ESCPOS_CMD_BUFFER_SIZE  512
#else
#define ESCPOS_CMD_BUFFER_SIZE  CONFIG_ESCPOS_CMD_BUFFER_SIZE
#endif

/* ─────────────────────────────────────────────────────────────────────────────
 * Default paper width (characters, font A)
 * ───────────────────────────────────────────────────────────────────────────*/
/* Default paper width (characters, font A) */
#ifndef CONFIG_ESCPOS_PAPER_WIDTH_CHARS
#define ESCPOS_PAPER_WIDTH      48
#else
#define ESCPOS_PAPER_WIDTH      CONFIG_ESCPOS_PAPER_WIDTH_CHARS
#endif

/* Default printable width in dots. Common values: 384 for 58mm, 576 for 80mm. */
#ifndef CONFIG_ESCPOS_PRINTER_WIDTH_DOTS
#define ESCPOS_PRINTER_WIDTH_DOTS 384
#else
#define ESCPOS_PRINTER_WIDTH_DOTS CONFIG_ESCPOS_PRINTER_WIDTH_DOTS
#endif

/* ─────────────────────────────────────────────────────────────────────────────
 * USB – match any VID/PID by default (0x0000)
 * ───────────────────────────────────────────────────────────────────────────*/
#ifndef CONFIG_ESCPOS_USB_VID
#define ESCPOS_USB_VID          0x0000
#else
#define ESCPOS_USB_VID          CONFIG_ESCPOS_USB_VID
#endif

#ifndef CONFIG_ESCPOS_USB_PID
#define ESCPOS_USB_PID          0x0000
#else
#define ESCPOS_USB_PID          CONFIG_ESCPOS_USB_PID
#endif

#ifndef CONFIG_ESCPOS_USB_HOST_PRIORITY
#define ESCPOS_USB_HOST_PRIORITY 20
#else
#define ESCPOS_USB_HOST_PRIORITY CONFIG_ESCPOS_USB_HOST_PRIORITY
#endif

#ifndef CONFIG_ESCPOS_USB_OUT_BUFFER_SIZE
#define ESCPOS_USB_OUT_BUFFER_SIZE 512
#else
#define ESCPOS_USB_OUT_BUFFER_SIZE CONFIG_ESCPOS_USB_OUT_BUFFER_SIZE
#endif

#ifndef CONFIG_ESCPOS_USB_IN_BUFFER_SIZE
#define ESCPOS_USB_IN_BUFFER_SIZE  64
#else
#define ESCPOS_USB_IN_BUFFER_SIZE  CONFIG_ESCPOS_USB_IN_BUFFER_SIZE
#endif

#ifndef CONFIG_ESCPOS_USB_CONNECT_TIMEOUT_MS
#define ESCPOS_USB_CONNECT_TIMEOUT_MS 2000
#else
#define ESCPOS_USB_CONNECT_TIMEOUT_MS CONFIG_ESCPOS_USB_CONNECT_TIMEOUT_MS
#endif

/* ─────────────────────────────────────────────────────────────────────────────
 * Image printing configuration
 * ───────────────────────────────────────────────────────────────────────────*/

/* Maximum image file size in bytes (400 KB default) */
#ifndef CONFIG_ESCPOS_MAX_IMAGE_SIZE_BYTES
#define ESCPOS_MAX_IMAGE_SIZE_BYTES (400 * 1024)
#else
#define ESCPOS_MAX_IMAGE_SIZE_BYTES CONFIG_ESCPOS_MAX_IMAGE_SIZE_BYTES
#endif

/* Maximum image width in pixels (printer physical width, typically 384 or 512 dots) */
#ifndef CONFIG_ESCPOS_MAX_IMAGE_WIDTH
#define ESCPOS_MAX_IMAGE_WIDTH 384
#else
#define ESCPOS_MAX_IMAGE_WIDTH CONFIG_ESCPOS_MAX_IMAGE_WIDTH
#endif

/* Maximum image height in pixels (memory constraint, 1024 pixels default) */
#ifndef CONFIG_ESCPOS_MAX_IMAGE_HEIGHT
#define ESCPOS_MAX_IMAGE_HEIGHT 1024
#else
#define ESCPOS_MAX_IMAGE_HEIGHT CONFIG_ESCPOS_MAX_IMAGE_HEIGHT
#endif

/* Maximum bitmap data size in bytes (allocated memory for processed bitmap) */
#ifndef CONFIG_ESCPOS_MAX_BITMAP_SIZE_BYTES
#define ESCPOS_MAX_BITMAP_SIZE_BYTES (48 * 1024)
#else
#define ESCPOS_MAX_BITMAP_SIZE_BYTES CONFIG_ESCPOS_MAX_BITMAP_SIZE_BYTES
#endif

#ifdef __cplusplus
}
#endif
