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
#ifndef CONFIG_ESCPOS_PAPER_WIDTH
#define ESCPOS_PAPER_WIDTH      48
#else
#define ESCPOS_PAPER_WIDTH      CONFIG_ESCPOS_PAPER_WIDTH
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

#ifdef __cplusplus
}
#endif