#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* ─────────────────────────────────────────────────────────────────────────────
 * Control characters
 * ───────────────────────────────────────────────────────────────────────────*/
#define ESC  0x1B
#define GS   0x1D
#define FS   0x1C
#define DLE  0x10
#define EOT  0x04
#define HT   0x09
#define LF   0x0A
#define CR   0x0D
#define FF   0x0C
#define CAN  0x18
#define NUL  0x00

/* ─────────────────────────────────────────────────────────────────────────────
 * Printer initialisation
 * ───────────────────────────────────────────────────────────────────────────*/
#define CMD_INIT                ESC, 0x40        /* ESC @ */

/* ─────────────────────────────────────────────────────────────────────────────
 * Line feed / feed
 * ───────────────────────────────────────────────────────────────────────────*/
#define CMD_LF                  LF               /* 0x0A */
#define CMD_FEED_N              ESC, 0x64        /* ESC d n */
#define CMD_FEED_REVERSE_N      ESC, 0x65        /* ESC e n */

/* ─────────────────────────────────────────────────────────────────────────────
 * Text alignment
 * ───────────────────────────────────────────────────────────────────────────*/
#define CMD_ALIGN               ESC, 0x61        /* ESC a n */

/* ─────────────────────────────────────────────────────────────────────────────
 * Font selection
 * ───────────────────────────────────────────────────────────────────────────*/
#define CMD_FONT_SELECT         ESC, 0x4D        /* ESC M n */

/* ─────────────────────────────────────────────────────────────────────────────
 * Print mode (bold, double-width/height, underline)
 * ───────────────────────────────────────────────────────────────────────────*/
#define CMD_PRINT_MODE          ESC, 0x21        /* ESC ! n */
#define CMD_UNDERLINE           ESC, 0x2D        /* ESC - n  (0=off, 1=1-dot, 2=2-dot) */
#define CMD_BOLD                ESC, 0x45        /* ESC E n  (0=off, 1=on) */
#define CMD_DOUBLE_STRIKE       ESC, 0x47        /* ESC G n */
#define CMD_CHAR_SIZE           GS,  0x21        /* GS  ! n */
#define CMD_UPSIDE_DOWN         ESC, 0x7B        /* ESC { n */

/* ─────────────────────────────────────────────────────────────────────────────
 * Character spacing
 * ───────────────────────────────────────────────────────────────────────────*/
#define CMD_CHAR_SPACING        ESC, 0x20        /* ESC SP n */
#define CMD_LINE_SPACING        ESC, 0x33        /* ESC 3 n */
#define CMD_LINE_SPACING_DEFAULT ESC, 0x32       /* ESC 2   */

/* ─────────────────────────────────────────────────────────────────────────────
 * Character code table / international
 * ───────────────────────────────────────────────────────────────────────────*/
#define CMD_CODEPAGE            ESC, 0x74        /* ESC t n */
#define CMD_INTL_CHARSET        ESC, 0x52        /* ESC R n */

/* ─────────────────────────────────────────────────────────────────────────────
 * Paper cut
 * ───────────────────────────────────────────────────────────────────────────*/
#define CMD_CUT_FULL            GS,  0x56, 0x00  /* GS V 0 */
#define CMD_CUT_PARTIAL         GS,  0x56, 0x01  /* GS V 1 */
#define CMD_CUT_FEED_FULL       GS,  0x56, 0x41  /* GS V A n */
#define CMD_CUT_FEED_PARTIAL    GS,  0x56, 0x42  /* GS V B n */

/* ─────────────────────────────────────────────────────────────────────────────
 * Barcode
 * ───────────────────────────────────────────────────────────────────────────*/
#define CMD_BARCODE_HEIGHT      GS,  0x68        /* GS h n  */
#define CMD_BARCODE_WIDTH       GS,  0x77        /* GS w n  */
#define CMD_BARCODE_HRI_POS     GS,  0x48        /* GS H n  */
#define CMD_BARCODE_HRI_FONT    GS,  0x66        /* GS f n  */
#define CMD_BARCODE_PRINT       GS,  0x6B        /* GS k m  */

/* ─────────────────────────────────────────────────────────────────────────────
 * QR Code (model 2)
 * ───────────────────────────────────────────────────────────────────────────*/
#define CMD_QR_MODEL            GS,  0x28, 0x6B  /* GS ( k pL pH cn fn ... */
#define QR_FN_MODEL             49               /* fn = 49 */
#define QR_FN_SIZE              51               /* fn = 51 */
#define QR_FN_EC                50               /* fn = 50 */
#define QR_FN_STORE             80               /* fn = 80 */
#define QR_FN_PRINT             81               /* fn = 81 */
#define QR_CN                   49               /* cn = 49 */

/* ─────────────────────────────────────────────────────────────────────────────
 * Raster / bitmap image
 * ───────────────────────────────────────────────────────────────────────────*/
#define CMD_IMAGE_RASTER        GS,  0x76, 0x30  /* GS v 0 m xL xH yL yH d1..dk */
#define CMD_IMAGE_BIT           ESC, 0x2A        /* ESC *  m nL nH d1..dk */

/* ─────────────────────────────────────────────────────────────────────────────
 * Horizontal tab
 * ───────────────────────────────────────────────────────────────────────────*/
#define CMD_HT                  HT               /* 0x09 */
#define CMD_SET_HT              ESC, 0x44        /* ESC D n1...nk NUL */

/* ─────────────────────────────────────────────────────────────────────────────
 * Buzzer / beep
 * ───────────────────────────────────────────────────────────────────────────*/
#define CMD_BEEP                ESC, 0x42        /* ESC B n t */

/* ─────────────────────────────────────────────────────────────────────────────
 * Cash drawer
 * ───────────────────────────────────────────────────────────────────────────*/
#define CMD_CASH_DRAWER         ESC, 0x70        /* ESC p m t1 t2 */

/* ─────────────────────────────────────────────────────────────────────────────
 * Status / realtime
 * ───────────────────────────────────────────────────────────────────────────*/
#define CMD_DLE_EOT             DLE, EOT         /* DLE EOT n */

#ifdef __cplusplus
}
#endif