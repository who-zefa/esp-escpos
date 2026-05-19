#include <string.h>
#include <stdlib.h>

#include "esp_check.h"

#include "escpos_private.h"

static const char *TAG = "escpos_qr";

escpos_qr_config_t escpos_qr_get_default_config(void)
{
    return (escpos_qr_config_t){
        .size = 6,
        .ec_level = ESCPOS_QR_EC_M,
        .align = ESCPOS_ALIGN_LEFT,
    };
}

#define QR_VERSION_2_SIZE 25
#define QR_VERSION_2_M_DATA_CODEWORDS 28
#define QR_VERSION_2_M_EC_CODEWORDS 16
#define QR_VERSION_2_M_BYTE_CAPACITY 26
#define QR_QUIET_ZONE_MODULES 4

static void qr_set_module(uint8_t modules[QR_VERSION_2_SIZE][QR_VERSION_2_SIZE],
                          uint8_t reserved[QR_VERSION_2_SIZE][QR_VERSION_2_SIZE],
                          int x,
                          int y,
                          bool black,
                          bool is_reserved)
{
    if (x < 0 || y < 0 || x >= QR_VERSION_2_SIZE || y >= QR_VERSION_2_SIZE) {
        return;
    }

    modules[y][x] = black ? 1 : 0;
    if (is_reserved) {
        reserved[y][x] = 1;
    }
}

static void qr_add_finder(uint8_t modules[QR_VERSION_2_SIZE][QR_VERSION_2_SIZE],
                          uint8_t reserved[QR_VERSION_2_SIZE][QR_VERSION_2_SIZE],
                          int x,
                          int y)
{
    for (int dy = -1; dy <= 7; dy++) {
        for (int dx = -1; dx <= 7; dx++) {
            bool black = false;
            if (dx >= 0 && dx <= 6 && dy >= 0 && dy <= 6) {
                black = dx == 0 || dx == 6 || dy == 0 || dy == 6 ||
                        (dx >= 2 && dx <= 4 && dy >= 2 && dy <= 4);
            }
            qr_set_module(modules, reserved, x + dx, y + dy, black, true);
        }
    }
}

static void qr_add_function_patterns(uint8_t modules[QR_VERSION_2_SIZE][QR_VERSION_2_SIZE],
                                     uint8_t reserved[QR_VERSION_2_SIZE][QR_VERSION_2_SIZE])
{
    qr_add_finder(modules, reserved, 0, 0);
    qr_add_finder(modules, reserved, QR_VERSION_2_SIZE - 7, 0);
    qr_add_finder(modules, reserved, 0, QR_VERSION_2_SIZE - 7);

    for (int i = 8; i < QR_VERSION_2_SIZE - 8; i++) {
        qr_set_module(modules, reserved, i, 6, (i % 2) == 0, true);
        qr_set_module(modules, reserved, 6, i, (i % 2) == 0, true);
    }

    for (int dy = -2; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
            int dist = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
            qr_set_module(modules, reserved, 18 + dx, 18 + dy, dist != 1, true);
        }
    }

    qr_set_module(modules, reserved, 8, QR_VERSION_2_SIZE - 8, true, true);

    for (int i = 0; i < 9; i++) {
        if (i != 6) {
            qr_set_module(modules, reserved, 8, i, false, true);
            qr_set_module(modules, reserved, i, 8, false, true);
        }
    }
    for (int i = QR_VERSION_2_SIZE - 8; i < QR_VERSION_2_SIZE; i++) {
        qr_set_module(modules, reserved, 8, i, false, true);
        qr_set_module(modules, reserved, i, 8, false, true);
    }
}

static void qr_append_bits(uint8_t *data,
                           size_t data_len,
                           uint16_t *bit_len,
                           uint32_t value,
                           uint8_t count)
{
    for (int i = count - 1; i >= 0; i--) {
        if (*bit_len >= data_len * 8) {
            return;
        }
        if (((value >> i) & 1) != 0) {
            data[*bit_len >> 3] |= (uint8_t)(1 << (7 - (*bit_len & 7)));
        }
        (*bit_len)++;
    }
}

static void qr_build_data_codewords(const char *text,
                                    size_t text_len,
                                    uint8_t data_codewords[QR_VERSION_2_M_DATA_CODEWORDS])
{
    uint16_t bit_len = 0;
    memset(data_codewords, 0, QR_VERSION_2_M_DATA_CODEWORDS);

    qr_append_bits(data_codewords, QR_VERSION_2_M_DATA_CODEWORDS, &bit_len, 0x4, 4);
    qr_append_bits(data_codewords, QR_VERSION_2_M_DATA_CODEWORDS, &bit_len, (uint32_t)text_len, 8);
    for (size_t i = 0; i < text_len; i++) {
        qr_append_bits(data_codewords, QR_VERSION_2_M_DATA_CODEWORDS, &bit_len, (uint8_t)text[i], 8);
    }

    uint16_t remaining = (uint16_t)((QR_VERSION_2_M_DATA_CODEWORDS * 8) - bit_len);
    qr_append_bits(data_codewords, QR_VERSION_2_M_DATA_CODEWORDS, &bit_len, 0,
                   remaining < 4 ? remaining : 4);
    while ((bit_len & 7) != 0) {
        qr_append_bits(data_codewords, QR_VERSION_2_M_DATA_CODEWORDS, &bit_len, 0, 1);
    }

    static const uint8_t pads[] = {0xEC, 0x11};
    uint8_t pad_idx = 0;
    while ((bit_len / 8) < QR_VERSION_2_M_DATA_CODEWORDS) {
        data_codewords[bit_len / 8] = pads[pad_idx & 1];
        bit_len += 8;
        pad_idx++;
    }
}

static uint8_t qr_gf_mul(uint8_t x, uint8_t y)
{
    uint16_t z = 0;
    for (int i = 7; i >= 0; i--) {
        z = (uint16_t)((z << 1) ^ (((z >> 7) & 1) ? 0x11D : 0));
        if (((y >> i) & 1) != 0) {
            z ^= x;
        }
    }
    return (uint8_t)z;
}

static uint8_t qr_gf_pow2(uint8_t exp)
{
    uint8_t result = 1;
    while (exp-- > 0) {
        result = qr_gf_mul(result, 2);
    }
    return result;
}

static void qr_compute_generator(uint8_t generator[QR_VERSION_2_M_EC_CODEWORDS + 1])
{
    memset(generator, 0, QR_VERSION_2_M_EC_CODEWORDS + 1);
    generator[0] = 1;
    uint8_t degree = 0;

    for (uint8_t i = 0; i < QR_VERSION_2_M_EC_CODEWORDS; i++) {
        uint8_t root = qr_gf_pow2(i);
        generator[degree + 1] = 0;
        for (int j = degree; j >= 0; j--) {
            generator[j + 1] ^= generator[j];
            generator[j] = qr_gf_mul(generator[j], root);
        }
        degree++;
    }
}

static void qr_compute_ecc(const uint8_t data[QR_VERSION_2_M_DATA_CODEWORDS],
                           uint8_t ecc[QR_VERSION_2_M_EC_CODEWORDS])
{
    uint8_t generator[QR_VERSION_2_M_EC_CODEWORDS + 1];
    qr_compute_generator(generator);
    memset(ecc, 0, QR_VERSION_2_M_EC_CODEWORDS);

    for (uint8_t i = 0; i < QR_VERSION_2_M_DATA_CODEWORDS; i++) {
        uint8_t factor = data[i] ^ ecc[0];
        memmove(ecc, ecc + 1, QR_VERSION_2_M_EC_CODEWORDS - 1);
        ecc[QR_VERSION_2_M_EC_CODEWORDS - 1] = 0;
        for (uint8_t j = 0; j < QR_VERSION_2_M_EC_CODEWORDS; j++) {
            ecc[j] ^= qr_gf_mul(generator[j], factor);
        }
    }
}

static bool qr_get_codeword_bit(const uint8_t *codewords, uint16_t bit_idx)
{
    return ((codewords[bit_idx >> 3] >> (7 - (bit_idx & 7))) & 1) != 0;
}

static void qr_place_data(uint8_t modules[QR_VERSION_2_SIZE][QR_VERSION_2_SIZE],
                          uint8_t reserved[QR_VERSION_2_SIZE][QR_VERSION_2_SIZE],
                          const uint8_t *codewords,
                          uint16_t total_bits)
{
    uint16_t bit_idx = 0;
    int direction = -1;

    for (int right = QR_VERSION_2_SIZE - 1; right >= 1; right -= 2) {
        if (right == 6) {
            right--;
        }
        for (int vert = 0; vert < QR_VERSION_2_SIZE; vert++) {
            int y = direction == -1 ? QR_VERSION_2_SIZE - 1 - vert : vert;
            for (int j = 0; j < 2; j++) {
                int x = right - j;
                if (reserved[y][x]) {
                    continue;
                }
                bool bit = bit_idx < total_bits && qr_get_codeword_bit(codewords, bit_idx);
                bool mask = ((x + y) & 1) == 0;
                modules[y][x] = (bit ^ mask) ? 1 : 0;
                bit_idx++;
            }
        }
        direction = -direction;
    }
}

static void qr_add_format_bits(uint8_t modules[QR_VERSION_2_SIZE][QR_VERSION_2_SIZE])
{
    const uint16_t format = 0x5412; /* EC-M, mask 0 */

    for (int i = 0; i <= 5; i++) modules[8][i] = (uint8_t)((format >> i) & 1);
    modules[8][7] = (uint8_t)((format >> 6) & 1);
    modules[8][8] = (uint8_t)((format >> 7) & 1);
    modules[7][8] = (uint8_t)((format >> 8) & 1);
    for (int i = 9; i < 15; i++) modules[14 - i][8] = (uint8_t)((format >> i) & 1);
    for (int i = 0; i < 8; i++) modules[QR_VERSION_2_SIZE - 1 - i][8] = (uint8_t)((format >> i) & 1);
    for (int i = 8; i < 15; i++) modules[8][QR_VERSION_2_SIZE - 15 + i] = (uint8_t)((format >> i) & 1);
    modules[8][QR_VERSION_2_SIZE - 8] = 1;
}

static esp_err_t qr_render_image(const uint8_t modules[QR_VERSION_2_SIZE][QR_VERSION_2_SIZE],
                                 uint8_t scale,
                                 escpos_align_t align,
                                 escpos_image_t *image)
{
    uint16_t module_count = QR_VERSION_2_SIZE + (QR_QUIET_ZONE_MODULES * 2);
    uint16_t pixel_size = (uint16_t)(module_count * scale);
    size_t row_bytes = (pixel_size + 7) / 8;
    size_t bitmap_size = row_bytes * pixel_size;

    uint8_t *bitmap = (uint8_t *)calloc(bitmap_size, 1);
    ESP_RETURN_ON_FALSE(bitmap, ESP_ERR_NO_MEM, TAG, "Failed to allocate QR bitmap");

    for (uint16_t y = 0; y < pixel_size; y++) {
        uint16_t module_y = (uint16_t)(y / scale);
        for (uint16_t x = 0; x < pixel_size; x++) {
            uint16_t module_x = (uint16_t)(x / scale);
            bool black = false;

            if (module_x >= QR_QUIET_ZONE_MODULES &&
                module_x < QR_QUIET_ZONE_MODULES + QR_VERSION_2_SIZE &&
                module_y >= QR_QUIET_ZONE_MODULES &&
                module_y < QR_QUIET_ZONE_MODULES + QR_VERSION_2_SIZE) {
                black = modules[module_y - QR_QUIET_ZONE_MODULES]
                               [module_x - QR_QUIET_ZONE_MODULES] != 0;
            }
            if (black) {
                bitmap[(y * row_bytes) + (x >> 3)] |= (uint8_t)(1 << (7 - (x & 7)));
            }
        }
    }

    image->width = pixel_size;
    image->height = pixel_size;
    image->bitmap_data = bitmap;
    image->bitmap_size = bitmap_size;
    image->print_area_width = 0;
    image->align = align;
    return ESP_OK;
}

esp_err_t escpos_print_qr(escpos_printer_t *printer,
                          const char *data,
                          const escpos_qr_config_t *config)
{
    ESP_RETURN_ON_FALSE(printer && data && config, ESP_ERR_INVALID_ARG, TAG,
                        "Invalid QR argument");
    ESP_RETURN_ON_FALSE(config->size >= 1 && config->size <= 16,
                        ESP_ERR_INVALID_ARG, TAG, "QR size must be 1..16");
    ESP_RETURN_ON_FALSE(config->ec_level == ESCPOS_QR_EC_M,
                        ESP_ERR_NOT_SUPPORTED, TAG,
                        "Host QR encoder currently supports EC-M only");
    ESP_RETURN_ON_FALSE(config->align <= ESCPOS_ALIGN_RIGHT,
                        ESP_ERR_INVALID_ARG, TAG, "Invalid QR alignment");
    if (config->align != ESCPOS_ALIGN_LEFT) {
        ESP_RETURN_ON_ERROR(escpos_require_paper_width(printer),
                            TAG, "Cannot align QR without paper width");
    }

    size_t data_len = strlen(data);
    ESP_RETURN_ON_FALSE(data_len > 0 && data_len <= QR_VERSION_2_M_BYTE_CAPACITY,
                        ESP_ERR_INVALID_ARG, TAG,
                        "Host QR encoder supports up to %d bytes",
                        QR_VERSION_2_M_BYTE_CAPACITY);

    uint8_t modules[QR_VERSION_2_SIZE][QR_VERSION_2_SIZE] = {0};
    uint8_t reserved[QR_VERSION_2_SIZE][QR_VERSION_2_SIZE] = {0};
    uint8_t data_codewords[QR_VERSION_2_M_DATA_CODEWORDS];
    uint8_t ecc[QR_VERSION_2_M_EC_CODEWORDS];
    uint8_t all_codewords[QR_VERSION_2_M_DATA_CODEWORDS + QR_VERSION_2_M_EC_CODEWORDS];

    qr_add_function_patterns(modules, reserved);
    qr_build_data_codewords(data, data_len, data_codewords);
    qr_compute_ecc(data_codewords, ecc);
    memcpy(all_codewords, data_codewords, sizeof(data_codewords));
    memcpy(all_codewords + sizeof(data_codewords), ecc, sizeof(ecc));
    qr_place_data(modules, reserved, all_codewords, sizeof(all_codewords) * 8);
    qr_add_format_bits(modules);

    escpos_image_t image = {0};
    ESP_RETURN_ON_ERROR(qr_render_image(modules, config->size, config->align, &image),
                        TAG, "Failed to render QR image");

    image.print_area_width = printer->width_dots;
    esp_err_t ret = escpos_print_image(printer, &image, ESCPOS_IMAGE_MODE_NORMAL);
    escpos_image_free(&image);
    return ret;
}
