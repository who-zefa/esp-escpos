/**
 * @file escpos_image.c
 * @brief ESC/POS image printing implementation
 * 
 * Handles image loading, conversion to monochrome bitmap, and printing to ESC/POS printer.
 * Supports BMP format and raw image data with various dithering algorithms.
 */

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"

#include "escpos.h"
#include "escpos_image.h"
#include "escpos_config.h"
#include "escpos_commands.h"

static const char *TAG = "escpos_image";

/* ─────────────────────────────────────────────────────────────────────────────
 * BMP Header Structures (for BMP file parsing)
 * ───────────────────────────────────────────────────────────────────────────*/

#pragma pack(1)
typedef struct {
    uint16_t magic;         /* "BM" = 0x4D42 */
    uint32_t file_size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t pixel_offset;  /* Offset to pixel data */
} bmp_file_header_t;

typedef struct {
    uint32_t header_size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t image_size;
    int32_t x_pixels_per_meter;
    int32_t y_pixels_per_meter;
    uint32_t colors_used;
    uint32_t important_colors;
} bmp_info_header_t;
#pragma pack()

/* ─────────────────────────────────────────────────────────────────────────────
 * Default Parameters
 * ───────────────────────────────────────────────────────────────────────────*/

escpos_image_params_t escpos_image_get_default_params(void)
{
    return (escpos_image_params_t){
        .input_format = ESCPOS_IMAGE_FORMAT_AUTO,  /* Auto-detect from magic bytes */
        .dither_mode = ESCPOS_DITHER_THRESHOLD,
        .max_width = ESCPOS_MAX_IMAGE_WIDTH,
        .print_width = 0,
        .align = ESCPOS_ALIGN_LEFT,
        .threshold = 128,
        .auto_scale = true,
        .quality = 80,  /* 80% quality for lossy formats */
    };
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Image Format Detection
 * ───────────────────────────────────────────────────────────────────────────*/

escpos_image_format_t escpos_image_detect_format(const uint8_t *data, size_t data_len)
{
    if (!data || data_len < 4) {
        return ESCPOS_IMAGE_FORMAT_BMP;  /* Default to BMP on error */
    }

    /* Check BMP magic bytes: "BM" (0x42 0x4D) */
    if (data[0] == 0x42 && data[1] == 0x4D) {
        return ESCPOS_IMAGE_FORMAT_BMP;
    }

    /* Check PNG magic bytes: 0x89 0x50 0x4E 0x47 */
    if (data_len >= 4 && data[0] == 0x89 && data[1] == 0x50 && 
        data[2] == 0x4E && data[3] == 0x47) {
        return ESCPOS_IMAGE_FORMAT_PNG;
    }

    /* Check JPEG magic bytes: 0xFF 0xD8 0xFF */
    if (data_len >= 3 && data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF) {
        return ESCPOS_IMAGE_FORMAT_JPG;
    }

    /* Default to BMP if unknown */
    return ESCPOS_IMAGE_FORMAT_BMP;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Color conversion utilities
 * ───────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Convert RGB888 to grayscale using luminosity method
 * @param r Red channel (0-255)
 * @param g Green channel (0-255)
 * @param b Blue channel (0-255)
 * @return Grayscale value (0-255)
 */
static uint8_t rgb_to_gray(uint8_t r, uint8_t g, uint8_t b)
{
    /* Standard luminosity formula: 0.299*R + 0.587*G + 0.114*B */
    return (uint8_t)((r * 77 + g * 150 + b * 29) >> 8);
}

static int16_t clamp_gray_value(int16_t value)
{
    if (value < 0) {
        return 0;
    }
    if (value > 255) {
        return 255;
    }
    return value;
}

static uint16_t get_target_width(uint16_t source_width, const escpos_image_params_t *params)
{
    uint16_t max_width = params->max_width ? params->max_width : ESCPOS_MAX_IMAGE_WIDTH;
    uint16_t target_width = source_width;

    if (params->print_width) {
        target_width = params->print_width;
    } else if (params->auto_scale && source_width > max_width) {
        target_width = max_width;
    }

    if (target_width == 0) {
        target_width = 1;
    }

    return target_width;
}

static esp_err_t scale_gray_nearest(
    const uint8_t *src,
    uint16_t src_width,
    uint16_t src_height,
    uint16_t dst_width,
    uint8_t **dst_out,
    uint16_t *dst_height_out
)
{
    ESP_RETURN_ON_FALSE(src && dst_out && dst_height_out, ESP_ERR_INVALID_ARG,
                        TAG, "NULL scale argument");
    ESP_RETURN_ON_FALSE(src_width > 0 && src_height > 0 && dst_width > 0,
                        ESP_ERR_INVALID_ARG, TAG, "Invalid scale dimensions");

    uint32_t dst_height = ((uint32_t)src_height * dst_width + (src_width / 2)) / src_width;
    if (dst_height == 0) {
        dst_height = 1;
    }

    ESP_RETURN_ON_FALSE(dst_height <= ESCPOS_MAX_IMAGE_HEIGHT,
                        ESP_ERR_INVALID_ARG, TAG,
                        "Scaled image too tall: %lu px (max %d)",
                        dst_height, ESCPOS_MAX_IMAGE_HEIGHT);

    uint8_t *dst = (uint8_t *)malloc((size_t)dst_width * dst_height);
    ESP_RETURN_ON_FALSE(dst, ESP_ERR_NO_MEM, TAG,
                        "Failed to allocate scaled grayscale buffer");

    for (uint32_t y = 0; y < dst_height; y++) {
        uint32_t src_y = (y * src_height) / dst_height;
        for (uint32_t x = 0; x < dst_width; x++) {
            uint32_t src_x = (x * src_width) / dst_width;
            dst[y * dst_width + x] = src[src_y * src_width + src_x];
        }
    }

    *dst_out = dst;
    *dst_height_out = (uint16_t)dst_height;
    return ESP_OK;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Dithering Algorithms
 * ───────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Simple threshold dithering (128 threshold)
 */
static void dither_threshold(
    const uint8_t *gray_data,
    uint16_t width,
    uint16_t height,
    uint8_t threshold,
    uint8_t *bitmap_data
)
{
    size_t bitmap_row_bytes = (width + 7) / 8;
    
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint8_t gray = gray_data[y * width + x];
            uint8_t bit = (gray < threshold) ? 1 : 0;
            
            /* Pack 8 pixels per byte (MSB = leftmost pixel) */
            uint32_t byte_idx = y * bitmap_row_bytes + (x >> 3);
            uint8_t bit_pos = 7 - (x & 0x07);
            
            if (bit) {
                bitmap_data[byte_idx] |= (1 << bit_pos);
            } else {
                bitmap_data[byte_idx] &= ~(1 << bit_pos);
            }
        }
    }
}

/**
 * @brief Floyd-Steinberg dithering for better quality
 */
static void dither_floyd_steinberg(
    const uint8_t *gray_data,
    uint16_t width,
    uint16_t height,
    uint8_t threshold,
    uint8_t *bitmap_data
)
{
    /* Allocate error buffers (need int16_t to handle negative values) */
    int16_t *current_errors = (int16_t *)calloc(width + 2, sizeof(int16_t));
    int16_t *next_errors = (int16_t *)calloc(width + 2, sizeof(int16_t));
    if (!current_errors || !next_errors) {
        ESP_LOGW(TAG, "Failed to allocate error buffer for Floyd-Steinberg, falling back to threshold");
        free(current_errors);
        free(next_errors);
        dither_threshold(gray_data, width, height, threshold, bitmap_data);
        return;
    }

    size_t bitmap_row_bytes = (width + 7) / 8;
    int16_t threshold_bias = 128 - (int16_t)threshold;
    memset(bitmap_data, 0, bitmap_row_bytes * height);

    for (uint32_t y = 0; y < height; y++) {
        memset(next_errors, 0, (width + 2) * sizeof(int16_t));

        for (uint32_t x = 0; x < width; x++) {
            uint8_t gray = gray_data[y * width + x];
            int16_t error = current_errors[x + 1];
            int16_t value = clamp_gray_value((int16_t)gray + error + threshold_bias);
            
            uint8_t bit = (value < 128) ? 1 : 0;
            int16_t quant_error = value - (bit ? 0 : 255);

            /* Set bit in bitmap */
            uint32_t byte_idx = y * bitmap_row_bytes + (x >> 3);
            uint8_t bit_pos = 7 - (x & 0x07);
            if (bit) {
                bitmap_data[byte_idx] |= (1 << bit_pos);
            }

            /* Distribute error to neighbors (Floyd-Steinberg coefficients) */
            if (x + 1 < width) {
                current_errors[x + 2] += (quant_error * 7) >> 4;  /* Right */
            }
            if (y + 1 < height) {
                if (x > 0) {
                    next_errors[x] += (quant_error * 3) >> 4;     /* Below-left */
                }
                next_errors[x + 1] += (quant_error * 5) >> 4;     /* Below */
                if (x + 1 < width) {
                    next_errors[x + 2] += (quant_error * 1) >> 4; /* Below-right */
                }
            }
        }
        
        /* Prepare next error line */
        int16_t *tmp = current_errors;
        current_errors = next_errors;
        next_errors = tmp;
    }

    free(current_errors);
    free(next_errors);
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Image conversion functions
 * ───────────────────────────────────────────────────────────────────────────*/

esp_err_t escpos_image_rgb_to_mono(
    const uint8_t *rgb_data,
    uint16_t width,
    uint16_t height,
    uint16_t stride,
    escpos_image_dither_t dither_mode,
    uint8_t *bitmap_data
)
{
    ESP_RETURN_ON_FALSE(rgb_data && bitmap_data, ESP_ERR_INVALID_ARG, TAG,
                        "NULL rgb_data or bitmap_data");
    ESP_RETURN_ON_FALSE(width > 0 && height > 0, ESP_ERR_INVALID_ARG, TAG,
                        "Invalid image dimensions");

    /* Convert RGB to grayscale first */
    uint8_t *gray_buffer = (uint8_t *)malloc(width * height);
    ESP_RETURN_ON_FALSE(gray_buffer, ESP_ERR_NO_MEM, TAG,
                        "Failed to allocate grayscale buffer");

    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t src_idx = y * stride + x * 3;
            uint8_t r = rgb_data[src_idx + 0];
            uint8_t g = rgb_data[src_idx + 1];
            uint8_t b = rgb_data[src_idx + 2];
            gray_buffer[y * width + x] = rgb_to_gray(r, g, b);
        }
    }

    /* Apply dithering */
    esp_err_t ret = escpos_image_gray_to_mono(gray_buffer, width, height, 
                                              dither_mode, bitmap_data);
    free(gray_buffer);
    return ret;
}

static esp_err_t gray_to_mono_with_threshold(
    const uint8_t *gray_data,
    uint16_t width,
    uint16_t height,
    escpos_image_dither_t dither_mode,
    uint8_t threshold,
    uint8_t *bitmap_data
)
{
    ESP_RETURN_ON_FALSE(gray_data && bitmap_data, ESP_ERR_INVALID_ARG, TAG,
                        "NULL gray_data or bitmap_data");
    ESP_RETURN_ON_FALSE(width > 0 && height > 0, ESP_ERR_INVALID_ARG, TAG,
                        "Invalid image dimensions");

    /* Clear bitmap buffer */
    size_t bitmap_size = ((width + 7) / 8) * height;
    memset(bitmap_data, 0, bitmap_size);

    /* Apply selected dithering algorithm */
    switch (dither_mode) {
        case ESCPOS_DITHER_FLOYD_STEINBERG:
            dither_floyd_steinberg(gray_data, width, height, threshold, bitmap_data);
            break;
        
        case ESCPOS_DITHER_THRESHOLD:
        case ESCPOS_DITHER_NONE:
        default:
            dither_threshold(gray_data, width, height, threshold, bitmap_data);
            break;
    }

    // /* TEMPORARY FIX: Invert all bits - some printers use inverted polarity */
    // for (size_t i = 0; i < bitmap_size; i++) {
    //     bitmap_data[i] = ~bitmap_data[i];
    // }

    return ESP_OK;
}

esp_err_t escpos_image_gray_to_mono(
    const uint8_t *gray_data,
    uint16_t width,
    uint16_t height,
    escpos_image_dither_t dither_mode,
    uint8_t *bitmap_data
)
{
    return gray_to_mono_with_threshold(gray_data, width, height,
                                       dither_mode, 190, bitmap_data);
}

/* ─────────────────────────────────────────────────────────────────────────────
 * BMP file parsing
 * ───────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Parse BMP header and extract image information
 */
static esp_err_t parse_bmp_header(
    const uint8_t *data,
    size_t data_len,
    bmp_file_header_t *file_hdr,
    bmp_info_header_t *info_hdr,
    size_t *pixel_offset
)
{
    ESP_RETURN_ON_FALSE(data_len >= sizeof(bmp_file_header_t) + sizeof(bmp_info_header_t),
                        ESP_FAIL, TAG, "BMP file too small");

    memcpy(file_hdr, data, sizeof(bmp_file_header_t));
    memcpy(info_hdr, data + sizeof(bmp_file_header_t), sizeof(bmp_info_header_t));

    /* Check BMP magic number "BM" */
    if (file_hdr->magic != 0x4D42) {
        ESP_LOGE(TAG, "Invalid BMP magic: 0x%04X", file_hdr->magic);
        return ESP_FAIL;
    }

    /* Only support uncompressed formats */
    if (info_hdr->compression != 0) {
        ESP_LOGE(TAG, "Compressed BMP not supported (compression=%ld)", 
                 info_hdr->compression);
        return ESP_FAIL;
    }

    /* Check supported bit depths */
    if (info_hdr->bits_per_pixel != 8 && info_hdr->bits_per_pixel != 24 && 
        info_hdr->bits_per_pixel != 32) {
        ESP_LOGE(TAG, "Unsupported BMP bit depth: %d", info_hdr->bits_per_pixel);
        return ESP_FAIL;
    }

    *pixel_offset = file_hdr->pixel_offset;
    return ESP_OK;
}

static esp_err_t process_gray_image(
    uint8_t *gray_data,
    uint16_t width,
    uint16_t height,
    const escpos_image_params_t *params,
    escpos_image_t *image
)
{
    uint16_t out_width = get_target_width(width, params);
    uint16_t out_height = height;
    uint8_t *out_gray = gray_data;
    esp_err_t ret = ESP_OK;
    uint16_t max_width = params->max_width ? params->max_width : ESCPOS_MAX_IMAGE_WIDTH;

    if (out_width > max_width) {
        free(gray_data);
        ESP_LOGE(TAG, "Image width too large: %u dots (max %u)",
                 out_width, max_width);
        return ESP_ERR_INVALID_ARG;
    }

    if (out_width != width) {
        ret = scale_gray_nearest(gray_data, width, height, out_width,
                                 &out_gray, &out_height);
        free(gray_data);
        if (ret != ESP_OK) {
            return ret;
        }
    }

    size_t bitmap_row_bytes = (out_width + 7) / 8;
    size_t bitmap_size = bitmap_row_bytes * out_height;

    if (bitmap_size > ESCPOS_MAX_BITMAP_SIZE_BYTES) {
        free(out_gray);
        ESP_LOGE(TAG, "Image too large: %zu bytes (max %d)",
                 bitmap_size, ESCPOS_MAX_BITMAP_SIZE_BYTES);
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t *bitmap_data = (uint8_t *)malloc(bitmap_size);
    if (!bitmap_data) {
        free(out_gray);
        ESP_LOGE(TAG, "Failed to allocate bitmap buffer");
        return ESP_ERR_NO_MEM;
    }

    ret = gray_to_mono_with_threshold(out_gray, out_width, out_height,
                                      params->dither_mode, params->threshold,
                                      bitmap_data);
    free(out_gray);

    if (ret != ESP_OK) {
        free(bitmap_data);
        return ret;
    }

    image->width = out_width;
    image->height = out_height;
    image->bitmap_data = bitmap_data;
    image->bitmap_size = bitmap_size;
    image->print_area_width = params->max_width;
    image->align = params->align;

    return ESP_OK;
}

/**
 * @brief Load and convert 24-bit BMP to monochrome bitmap
 */
static esp_err_t process_bmp_24bit(
    const uint8_t *data,
    size_t data_len,
    const bmp_info_header_t *info_hdr,
    size_t pixel_offset,
    const escpos_image_params_t *params,
    escpos_image_t *image
)
{
    uint16_t width = (uint16_t)info_hdr->width;
    uint16_t height = (uint16_t)info_hdr->height;
    
    /* Row stride (padded to 4-byte boundary) */
    uint16_t row_bytes = ((width * 3 + 3) / 4) * 4;
    size_t expected_data_size = pixel_offset + row_bytes * height;

    ESP_RETURN_ON_FALSE(data_len >= expected_data_size, ESP_FAIL, TAG,
                        "BMP file truncated or invalid");

    uint8_t *gray_data = (uint8_t *)malloc(width * height);
    ESP_RETURN_ON_FALSE(gray_data, ESP_ERR_NO_MEM, TAG,
                        "Failed to allocate grayscale buffer");

    for (uint32_t y = 0; y < height; y++) {
        uint32_t src_y = height - 1 - y;  /* BMP rows are stored bottom-up */
        const uint8_t *src_row = data + pixel_offset + (src_y * row_bytes);

        for (uint32_t x = 0; x < width; x++) {
            uint8_t b = src_row[x * 3 + 0];
            uint8_t g = src_row[x * 3 + 1];
            uint8_t r = src_row[x * 3 + 2];
            gray_data[y * width + x] = rgb_to_gray(r, g, b);
        }
    }

    esp_err_t ret = process_gray_image(gray_data, width, height, params, image);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "BMP converted: %dx%d -> %dx%d bitmap %zu bytes",
                 width, height, image->width, image->height, image->bitmap_size);
    }
    return ret;
}

/**
 * @brief Load and convert 8-bit BMP (grayscale) to monochrome bitmap
 */
static esp_err_t process_bmp_8bit(
    const uint8_t *data,
    size_t data_len,
    const bmp_info_header_t *info_hdr,
    size_t pixel_offset,
    const escpos_image_params_t *params,
    escpos_image_t *image
)
{
    uint16_t width = (uint16_t)info_hdr->width;
    uint16_t height = (uint16_t)info_hdr->height;
    
    /* Row stride (padded to 4-byte boundary) */
    uint16_t row_bytes = ((width + 3) / 4) * 4;
    size_t expected_data_size = pixel_offset + row_bytes * height;

    ESP_RETURN_ON_FALSE(data_len >= expected_data_size, ESP_FAIL, TAG,
                        "BMP file truncated or invalid");

    uint8_t *gray_data = (uint8_t *)malloc(width * height);
    ESP_RETURN_ON_FALSE(gray_data, ESP_ERR_NO_MEM, TAG,
                        "Failed to allocate grayscale buffer");

    size_t palette_offset = sizeof(bmp_file_header_t) + info_hdr->header_size;
    uint32_t palette_colors = info_hdr->colors_used ? info_hdr->colors_used : 256;
    bool has_palette = palette_offset + (palette_colors * 4) <= pixel_offset &&
                       palette_offset + 4 <= data_len;

    for (uint32_t y = 0; y < height; y++) {
        uint32_t src_y = height - 1 - y;  /* BMP rows are stored bottom-up */
        const uint8_t *src_row = data + pixel_offset + (src_y * row_bytes);

        for (uint32_t x = 0; x < width; x++) {
            uint8_t idx = src_row[x];
            uint8_t gray = idx;

            if (has_palette && idx < palette_colors) {
                const uint8_t *entry = data + palette_offset + (idx * 4);
                uint8_t b = entry[0];
                uint8_t g = entry[1];
                uint8_t r = entry[2];
                gray = rgb_to_gray(r, g, b);
            }

            gray_data[y * width + x] = gray;
        }
    }

    esp_err_t ret = process_gray_image(gray_data, width, height, params, image);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "BMP (8-bit) converted: %dx%d -> %dx%d bitmap %zu bytes",
                 width, height, image->width, image->height, image->bitmap_size);
    }
    return ret;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Public Image Loading Functions
 * ───────────────────────────────────────────────────────────────────────────*/

esp_err_t escpos_image_load_from_buffer(
    const uint8_t *data,
    size_t data_len,
    const escpos_image_params_t *params,
    escpos_image_t *image
)
{
    ESP_RETURN_ON_FALSE(data && image, ESP_ERR_INVALID_ARG, TAG,
                        "NULL data or image pointer");
    ESP_RETURN_ON_FALSE(params, ESP_ERR_INVALID_ARG, TAG,
                        "NULL image params");
    ESP_RETURN_ON_FALSE(params->align <= ESCPOS_ALIGN_RIGHT,
                        ESP_ERR_INVALID_ARG, TAG,
                        "Invalid image alignment");
    ESP_RETURN_ON_FALSE(data_len > 0, ESP_ERR_INVALID_ARG, TAG,
                        "Invalid data length");
    ESP_RETURN_ON_FALSE(data_len <= ESCPOS_MAX_IMAGE_SIZE_BYTES,
                        ESP_ERR_INVALID_ARG, TAG,
                        "Image file too large: %zu bytes (max %d)", 
                        data_len, ESCPOS_MAX_IMAGE_SIZE_BYTES);

    memset(image, 0, sizeof(escpos_image_t));

    /* Determine image format */
    escpos_image_format_t format = params->input_format;
    if (format == ESCPOS_IMAGE_FORMAT_AUTO) {
        format = escpos_image_detect_format(data, data_len);
        ESP_LOGI(TAG, "Auto-detected image format: %d", format);
    }

    switch (format) {
        case ESCPOS_IMAGE_FORMAT_BMP: {
            bmp_file_header_t file_hdr;
            bmp_info_header_t info_hdr;
            size_t pixel_offset;

            esp_err_t ret = parse_bmp_header(data, data_len, &file_hdr, 
                                             &info_hdr, &pixel_offset);
            if (ret != ESP_OK) {
                return ret;
            }

            /* Check image dimensions */
            if (info_hdr.width <= 0 || info_hdr.height <= 0) {
                ESP_LOGE(TAG, "Invalid BMP dimensions: %ldx%ld", 
                         info_hdr.width, info_hdr.height);
                return ESP_FAIL;
            }

            /* Process based on bit depth */
            switch (info_hdr.bits_per_pixel) {
                case 24:
                    return process_bmp_24bit(data, data_len, &info_hdr, 
                                            pixel_offset, params, image);
                case 8:
                    return process_bmp_8bit(data, data_len, &info_hdr, 
                                           pixel_offset, params, image);
                default:
                    ESP_LOGE(TAG, "Unsupported bit depth: %d", 
                             info_hdr.bits_per_pixel);
                    return ESP_FAIL;
            }
        }

        case ESCPOS_IMAGE_FORMAT_PNG:
        case ESCPOS_IMAGE_FORMAT_JPG:
            ESP_LOGE(TAG, "Format %d requires external decoder library", format);
            ESP_LOGE(TAG, "Supported formats: BMP (24-bit, 8-bit uncompressed)");
            ESP_LOGE(TAG, "To add PNG/JPG support, integrate:");
            ESP_LOGE(TAG, "  - esp-idf: image/png component");
            ESP_LOGE(TAG, "  - stb_image.h (single header)");
            ESP_LOGE(TAG, "  - libjpeg-turbo");
            return ESP_FAIL;

        case ESCPOS_IMAGE_FORMAT_RAW:
            ESP_LOGE(TAG, "RAW format not yet implemented");
            return ESP_FAIL;

        default:
            ESP_LOGE(TAG, "Unknown image format: %d", format);
            return ESP_FAIL;
    }
}

esp_err_t escpos_image_load_from_file(
    const char *file_path,
    const escpos_image_params_t *params,
    escpos_image_t *image
)
{
    ESP_RETURN_ON_FALSE(file_path && image, ESP_ERR_INVALID_ARG, TAG,
                        "NULL file_path or image pointer");

    /* Open file */
    FILE *fp = fopen(file_path, "rb");
    if (!fp) {
        ESP_LOGE(TAG, "Failed to open file: %s", file_path);
        return ESP_ERR_NOT_FOUND;
    }

    /* Get file size */
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size <= 0 || file_size > (long)ESCPOS_MAX_IMAGE_SIZE_BYTES) {
        ESP_LOGE(TAG, "Invalid file size: %ld bytes (max %d)", 
                 file_size, ESCPOS_MAX_IMAGE_SIZE_BYTES);
        fclose(fp);
        return ESP_FAIL;
    }

    /* Read file into buffer */
    uint8_t *buffer = (uint8_t *)malloc(file_size);
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate file buffer: %ld bytes", file_size);
        fclose(fp);
        return ESP_ERR_NO_MEM;
    }

    size_t bytes_read = fread(buffer, 1, file_size, fp);
    fclose(fp);

    if (bytes_read != (size_t)file_size) {
        ESP_LOGE(TAG, "Failed to read complete file");
        free(buffer);
        return ESP_FAIL;
    }

    /* Process image buffer */
    esp_err_t ret = escpos_image_load_from_buffer(buffer, file_size, params, image);
    free(buffer);
    return ret;
}

void escpos_image_free(escpos_image_t *image)
{
    if (!image) return;

    if (image->bitmap_data) {
        free(image->bitmap_data);
        image->bitmap_data = NULL;
    }

    image->bitmap_size = 0;
    image->width = 0;
    image->height = 0;
    image->print_area_width = 0;
    image->align = ESCPOS_ALIGN_LEFT;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Image Printing to Printer
 * ───────────────────────────────────────────────────────────────────────────*/

/**
 * @brief Send bitmap image to printer using GS v 0 command (raster image)
 * 
 * ESC/POS command: GS v 0 m xL xH yL yH [image data]
 * - GS (0x1D), v (0x76), 0 (0x30): Image raster command
 * - m: Display mode (0=normal, 1=double-width, 2=double-height, 3=both)
 * - xL, xH: Image width in bytes (little-endian)
 * - yL, yH: Image height in dots (little-endian)
 * - image data: Bitmap data (8 pixels per byte, MSB=leftmost)
 */
static esp_err_t send_image_raster(
    escpos_printer_t *printer,
    const escpos_image_t *image,
    uint8_t display_mode
)
{
    size_t bitmap_row_bytes = (image->width + 7) / 8;
    size_t expected_bitmap_size = bitmap_row_bytes * image->height;
    uint8_t header[8];

    ESP_RETURN_ON_FALSE(image->bitmap_size >= expected_bitmap_size,
                        ESP_ERR_INVALID_ARG, TAG,
                        "Bitmap too small: %zu bytes (expected %zu)",
                        image->bitmap_size, expected_bitmap_size);
    ESP_RETURN_ON_FALSE(image->align <= ESCPOS_ALIGN_RIGHT,
                        ESP_ERR_INVALID_ARG, TAG, "Invalid image alignment");

    uint16_t area_width = image->print_area_width;
    if (area_width == 0) {
        area_width = escpos_get_printer_width_dots(printer);
    }
    ESP_RETURN_ON_FALSE(area_width > 0 || image->align == ESCPOS_ALIGN_LEFT,
                        ESP_ERR_INVALID_STATE, TAG,
                        "Paper width not configured; call escpos_init_printer_with_width_mm()");

    uint16_t effective_width = image->width;
    if ((display_mode & ESCPOS_IMAGE_MODE_DOUBLE_W) != 0) {
        effective_width = (uint16_t)(effective_width * 2);
    }

    /* Calculate padding in dots (pixels) */
    uint16_t left_offset_dots = 0;
    uint16_t right_offset_dots = 0;

    if (area_width > effective_width) {
        if (image->align == ESCPOS_ALIGN_CENTER) {
            left_offset_dots = (uint16_t)((area_width - effective_width) / 2);
            right_offset_dots = (uint16_t)(area_width - effective_width - left_offset_dots);
        } else if (image->align == ESCPOS_ALIGN_RIGHT) {
            left_offset_dots = (uint16_t)(area_width - effective_width);
            right_offset_dots = 0;
        }
    }

    /* Convert dot offsets to byte offsets */
    uint16_t left_offset_bytes = (left_offset_dots + 7) / 8;
    uint16_t right_offset_bytes = (right_offset_dots + 7) / 8;

    /* Total width in bytes (left padding + image + right padding) */
    uint16_t padded_width_bytes = left_offset_bytes + (uint16_t)bitmap_row_bytes + right_offset_bytes;

    ESP_LOGI(TAG, "Image alignment: area=%u dots, image=%u dots, left_pad=%u bytes, right_pad=%u bytes, total=%u bytes",
             area_width, effective_width, left_offset_bytes, right_offset_bytes, padded_width_bytes);

    /* GS v 0 expects image width in bytes (x-byte-count) */
    uint32_t idx = 0;
    header[idx++] = 0x1D;  /* GS */
    header[idx++] = 0x76;  /* v */
    header[idx++] = 0x30;  /* 0 */
    header[idx++] = display_mode & 0x03;  /* m: display mode */

    /* Use padded width so alignment is embedded in image data */
    header[idx++] = (padded_width_bytes & 0xFF);        /* xL */
    header[idx++] = ((padded_width_bytes >> 8) & 0xFF); /* xH */

    /* Height in dots (little-endian) */
    uint16_t height = image->height;
    header[idx++] = (height & 0xFF);        /* yL */
    header[idx++] = ((height >> 8) & 0xFF); /* yH */

    esp_err_t ret = escpos_write_raw(printer, header, sizeof(header));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send image header: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Allocate buffer for padded row */
    uint8_t *padded_row = (uint8_t *)malloc(padded_width_bytes);
    if (!padded_row) {
        ESP_LOGE(TAG, "Failed to allocate padded row buffer");
        return ESP_ERR_NO_MEM;
    }

    /* Send each row with padding */
    for (uint16_t row = 0; row < image->height; row++) {
        /* Clear padded row buffer */
        memset(padded_row, 0, padded_width_bytes);

        /* Copy image data into the middle */
        const uint8_t *row_data = image->bitmap_data + (row * bitmap_row_bytes);
        memcpy(padded_row + left_offset_bytes, row_data, bitmap_row_bytes);

        /* Send padded row */
        ret = escpos_write_raw(printer, padded_row, padded_width_bytes);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send image row %u: %s",
                     row, esp_err_to_name(ret));
            free(padded_row);
            return ret;
        }
    }

    free(padded_row);

    ESP_LOGI(TAG, "Image sent to printer (%dx%d, align=%d, left_pad=%u bytes, right_pad=%u bytes)",
             image->width, image->height, image->align, left_offset_bytes, right_offset_bytes);

    return ret;

    return ret;
}

esp_err_t escpos_print_image(
    escpos_printer_t *printer,
    const escpos_image_t *image,
    escpos_image_mode_t image_mode
)
{
    ESP_RETURN_ON_FALSE(printer && image, ESP_ERR_INVALID_ARG, TAG,
                        "NULL printer or image pointer");
    ESP_RETURN_ON_FALSE(image->bitmap_data && image->bitmap_size > 0,
                        ESP_ERR_INVALID_ARG, TAG, "Invalid image data");
    ESP_RETURN_ON_FALSE(image->width > 0 && image->height > 0,
                        ESP_ERR_INVALID_ARG, TAG, "Invalid image dimensions");

    ESP_LOGI(TAG, "Printing image: %dx%d", image->width, image->height);

    /* Send image to printer */
    return send_image_raster(printer, image, (uint8_t)image_mode);
}
