#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"
#include "escpos_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration for escpos_printer_t - defined in escpos.h */
typedef struct escpos_printer_s escpos_printer_t;

/* ─────────────────────────────────────────────────────────────────────────────
 * Image type definitions
 * ───────────────────────────────────────────────────────────────────────────*/

/**
 * @enum escpos_image_format_t
 * @brief Supported image input formats
 */
typedef enum {
    ESCPOS_IMAGE_FORMAT_BMP     = 0,    /**< Bitmap (BMP) format */
    ESCPOS_IMAGE_FORMAT_PNG     = 1,    /**< PNG format (auto-detected) */
    ESCPOS_IMAGE_FORMAT_JPG     = 2,    /**< JPEG format (auto-detected) */
    ESCPOS_IMAGE_FORMAT_RAW     = 3,    /**< Raw bitmap data (width-aligned) */
    ESCPOS_IMAGE_FORMAT_AUTO    = 99,   /**< Auto-detect from file magic bytes */
} escpos_image_format_t;

/**
 * @enum escpos_image_dither_t
 * @brief Dithering algorithms for grayscale to monochrome conversion
 */
typedef enum {
    ESCPOS_DITHER_NONE          = 0,    /**< No dithering - simple threshold */
    ESCPOS_DITHER_THRESHOLD     = 1,    /**< Simple binary threshold (128) */
    ESCPOS_DITHER_FLOYD_STEINBERG = 2, /**< Floyd-Steinberg dithering */
} escpos_image_dither_t;

/**
 * @struct escpos_image_t
 * @brief Image data structure for printer
 */
typedef struct {
    uint16_t width;              /**< Image width in pixels */
    uint16_t height;             /**< Image height in pixels */
    uint8_t *bitmap_data;        /**< Monochrome bitmap data (1-bit per pixel) */
    size_t bitmap_size;          /**< Size of bitmap data in bytes */
    uint16_t print_area_width;    /**< Printable area used for alignment (dots) */
    escpos_align_t align;         /**< Image alignment inside print_area_width */
} escpos_image_t;

/**
 * @struct escpos_image_params_t
 * @brief Parameters for image processing and printing
 */
typedef struct {
    escpos_image_format_t input_format;   /**< Input image format (AUTO to detect) */
    escpos_image_dither_t dither_mode;    /**< Dithering algorithm */
    uint16_t max_width;                   /**< Maximum print width (dots) */
    uint16_t print_width;                 /**< Desired output width in dots (0 = original/auto) */
    escpos_align_t align;                 /**< Image alignment within max_width */
    uint8_t threshold;                    /**< Gray-to-mono threshold (0-255, higher prints darker) */
    bool auto_scale;                      /**< Auto-scale if image exceeds max_width */
    uint8_t quality;                      /**< Quality for lossy formats (1-100) */
} escpos_image_params_t;

/* ─────────────────────────────────────────────────────────────────────────────
 * Default parameters
 * ───────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Get default image parameters
 * @return Initialized escpos_image_params_t with sensible defaults
 */
escpos_image_params_t escpos_image_get_default_params(void);

/**
 * @brief Auto-detect image format from file magic bytes
 * 
 * @param[in] data         Image data or file header (at least 16 bytes)
 * @param[in] data_len     Length of data
 * 
 * @return Detected image format
 * 
 * @note Checks for BMP, PNG, JPEG magic bytes
 */
escpos_image_format_t escpos_image_detect_format(const uint8_t *data, size_t data_len);

/* ─────────────────────────────────────────────────────────────────────────────
 * Image processing and printing
 * ───────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Load and process image from file
 * 
 * @param[in] file_path       Path to image file (BMP or raw binary)
 * @param[in] params          Processing parameters
 * @param[out] image          Output processed image structure
 * 
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if parameters are invalid
 * @return ESP_ERR_NO_MEM if memory allocation fails
 * @return ESP_ERR_NOT_FOUND if file not found
 * @return ESP_FAIL if image format is unsupported or corrupted
 */
esp_err_t escpos_image_load_from_file(
    const char *file_path,
    const escpos_image_params_t *params,
    escpos_image_t *image
);

/**
 * @brief Load and process image from memory buffer
 * 
 * @param[in] data           Image data in memory
 * @param[in] data_len       Length of data
 * @param[in] params         Processing parameters
 * @param[out] image         Output processed image structure
 * 
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if parameters are invalid
 * @return ESP_ERR_NO_MEM if memory allocation fails
 * @return ESP_FAIL if image format is unsupported or corrupted
 */
esp_err_t escpos_image_load_from_buffer(
    const uint8_t *data,
    size_t data_len,
    const escpos_image_params_t *params,
    escpos_image_t *image
);

/**
 * @brief Print image to printer
 * 
 * @param[in] printer        Printer handle
 * @param[in] image          Image structure containing bitmap data
 * @param[in] image_mode     Display mode (normal, double-width, etc.)
 * 
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if parameters are invalid
 * @return ESP_FAIL if printer communication fails
 */
esp_err_t escpos_print_image(
    escpos_printer_t *printer,
    const escpos_image_t *image,
    escpos_image_mode_t image_mode
);

/**
 * @brief Free image resources
 * 
 * @param[in] image          Image structure to free
 */
void escpos_image_free(escpos_image_t *image);

/**
 * @brief Convert RGB/BGR image data to monochrome bitmap
 * 
 * @param[in] rgb_data       RGB or BGR image data (3 bytes per pixel)
 * @param[in] width          Image width in pixels
 * @param[in] height         Image height in pixels
 * @param[in] stride         Bytes per row (may include padding)
 * @param[in] dither_mode    Dithering algorithm
 * @param[out] bitmap_data   Output monochrome bitmap (must be pre-allocated)
 * 
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if parameters invalid
 * 
 * @note Bitmap layout: each byte represents 8 pixels (MSB = leftmost pixel)
 *       Allocated size must be at least (width + 7) / 8 * height bytes
 */
esp_err_t escpos_image_rgb_to_mono(
    const uint8_t *rgb_data,
    uint16_t width,
    uint16_t height,
    uint16_t stride,
    escpos_image_dither_t dither_mode,
    uint8_t *bitmap_data
);

/**
 * @brief Convert grayscale image data to monochrome bitmap
 * 
 * @param[in] gray_data      Grayscale image data (1 byte per pixel, 0-255)
 * @param[in] width          Image width in pixels
 * @param[in] height         Image height in pixels
 * @param[in] dither_mode    Dithering algorithm
 * @param[out] bitmap_data   Output monochrome bitmap (must be pre-allocated)
 * 
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if parameters invalid
 * 
 * @note Allocated size for bitmap_data must be at least (width + 7) / 8 * height bytes
 */
esp_err_t escpos_image_gray_to_mono(
    const uint8_t *gray_data,
    uint16_t width,
    uint16_t height,
    escpos_image_dither_t dither_mode,
    uint8_t *bitmap_data
);

#ifdef __cplusplus
}
#endif
