#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "esp_check.h"

#include "escpos_private.h"
#include "escpos_commands.h"

static const char *TAG = "escpos_barcode";

escpos_barcode_config_t escpos_barcode_get_default_config(escpos_barcode_type_t type)
{
    return (escpos_barcode_config_t){
        .type = type,
        .width = (type == ESCPOS_BARCODE_CODE128) ? 2 : 3,
        .height = 80,
        .hri = ESCPOS_BARCODE_HRI_BELOW,
        .hri_font = ESCPOS_FONT_A,
        .align = ESCPOS_ALIGN_LEFT,
    };
}

static uint16_t escpos_estimate_barcode_width_dots(escpos_barcode_type_t type,
                                                   size_t payload_len,
                                                   uint8_t module_width)
{
    uint16_t modules = 0;

    switch (type) {
        case ESCPOS_BARCODE_UPC_A:
        case ESCPOS_BARCODE_EAN13:
            modules = 95;
            break;
        case ESCPOS_BARCODE_EAN8:
            modules = 67;
            break;
        case ESCPOS_BARCODE_UPC_E:
            modules = 51;
            break;
        case ESCPOS_BARCODE_I25:
            modules = (uint16_t)(18 + ((payload_len / 2) * 14));
            break;
        case ESCPOS_BARCODE_CODE39:
            modules = (uint16_t)((payload_len + 2) * 13);
            break;
        case ESCPOS_BARCODE_CODE128: {
            size_t data_symbols = payload_len;
            if (payload_len > 2) {
                data_symbols = payload_len - 2;
            }
            modules = (uint16_t)(((data_symbols + 2) * 11) + 13);
            break;
        }
        case ESCPOS_BARCODE_CODEBAR:
        case ESCPOS_BARCODE_CODE93:
        default:
            modules = (uint16_t)(payload_len * 12 + 24);
            break;
    }

    return (uint16_t)(modules * module_width);
}

static bool escpos_is_digits(const char *data)
{
    if (!data || !*data) {
        return false;
    }

    while (*data) {
        if (!isdigit((unsigned char)*data)) {
            return false;
        }
        data++;
    }

    return true;
}

static bool escpos_is_code39_char(char ch)
{
    return isdigit((unsigned char)ch) ||
           (ch >= 'A' && ch <= 'Z') ||
           ch == ' ' || ch == '-' || ch == '.' || ch == '$' ||
           ch == '/' || ch == '+' || ch == '%';
}

static bool escpos_validate_barcode_data(escpos_barcode_type_t type, const char *data, size_t len)
{
    if (!data || len == 0 || len > 255) {
        return false;
    }

    switch (type) {
        case ESCPOS_BARCODE_UPC_A:
            return (len == 11 || len == 12) && escpos_is_digits(data);
        case ESCPOS_BARCODE_UPC_E:
            return (len >= 6 && len <= 8) && escpos_is_digits(data);
        case ESCPOS_BARCODE_EAN13:
            return (len == 12 || len == 13) && escpos_is_digits(data);
        case ESCPOS_BARCODE_EAN8:
            return (len == 7 || len == 8) && escpos_is_digits(data);
        case ESCPOS_BARCODE_I25:
            return (len % 2) == 0 && escpos_is_digits(data);
        case ESCPOS_BARCODE_CODE39:
            for (size_t i = 0; i < len; i++) {
                if (!escpos_is_code39_char(data[i])) {
                    return false;
                }
            }
            return true;
        case ESCPOS_BARCODE_CODEBAR:
        case ESCPOS_BARCODE_CODE93:
        case ESCPOS_BARCODE_CODE128:
            return true;
        default:
            return false;
    }
}

static uint8_t ean13_checksum(const char *digits12)
{
    uint16_t sum = 0;

    for (uint8_t i = 0; i < 12; i++) {
        uint8_t digit = (uint8_t)(digits12[i] - '0');
        sum += (i & 1) ? (uint16_t)(digit * 3) : digit;
    }

    return (uint8_t)((10 - (sum % 10)) % 10);
}

static bool ean13_normalize(const char *data, char out[14])
{
    size_t len = strlen(data);
    if ((len != 12 && len != 13) || !escpos_is_digits(data)) {
        return false;
    }

    memcpy(out, data, len);
    if (len == 12) {
        out[12] = (char)('0' + ean13_checksum(out));
    } else if ((uint8_t)(out[12] - '0') != ean13_checksum(out)) {
        return false;
    }
    out[13] = '\0';
    return true;
}

static const char *code128_patterns[107] = {
    "212222", "222122", "222221", "121223", "121322", "131222",
    "122213", "122312", "132212", "221213", "221312", "231212",
    "112232", "122132", "122231", "113222", "123122", "123221",
    "223211", "221132", "221231", "213212", "223112", "312131",
    "311222", "321122", "321221", "312212", "322112", "322211",
    "212123", "212321", "232121", "111323", "131123", "131321",
    "112313", "132113", "132311", "211313", "231113", "231311",
    "112133", "112331", "132131", "113123", "113321", "133121",
    "313121", "211331", "231131", "213113", "213311", "213131",
    "311123", "311321", "331121", "312113", "312311", "332111",
    "314111", "221411", "431111", "111224", "111422", "121124",
    "121421", "141122", "141221", "112214", "112412", "122114",
    "122411", "142112", "142211", "241211", "221114", "413111",
    "241112", "134111", "111242", "121142", "121241", "114212",
    "124112", "124211", "411212", "421112", "421211", "212141",
    "214121", "412121", "111143", "111341", "131141", "114113",
    "114311", "411113", "411311", "113141", "114131", "311141",
    "411131", "211412", "211214", "211232", "2331112"
};

static bool code128_normalize_code_b(const char *data,
                                     const char **payload,
                                     size_t *payload_len)
{
    size_t len = strlen(data);

    if (len >= 2 && data[0] == '{') {
        if (data[1] != 'B') {
            return false;
        }
        data += 2;
        len -= 2;
    }

    if (len == 0 || len > 80) {
        return false;
    }

    for (size_t i = 0; i < len; i++) {
        unsigned char ch = (unsigned char)data[i];
        if (ch < 32 || ch > 127) {
            return false;
        }
    }

    *payload = data;
    *payload_len = len;
    return true;
}

static uint8_t code128_checksum_b(const char *payload, size_t len)
{
    uint16_t sum = 104; /* Start Code B */

    for (size_t i = 0; i < len; i++) {
        uint8_t value = (uint8_t)((unsigned char)payload[i] - 32);
        sum = (uint16_t)(sum + (value * (i + 1)));
    }

    return (uint8_t)(sum % 103);
}

static void ean13_append_pattern(uint8_t modules[95], uint8_t *idx, const char *pattern)
{
    for (uint8_t i = 0; i < 7; i++) {
        modules[(*idx)++] = pattern[i] == '1' ? 1 : 0;
    }
}

static void ean13_append_guard(uint8_t modules[95], uint8_t *idx, const char *pattern)
{
    while (*pattern) {
        modules[(*idx)++] = *pattern++ == '1' ? 1 : 0;
    }
}

static void ean13_build_modules(const char digits[14], uint8_t modules[95])
{
    static const char *left_odd[] = {
        "0001101", "0011001", "0010011", "0111101", "0100011",
        "0110001", "0101111", "0111011", "0110111", "0001011"
    };
    static const char *left_even[] = {
        "0100111", "0110011", "0011011", "0100001", "0011101",
        "0111001", "0000101", "0010001", "0001001", "0010111"
    };
    static const char *right[] = {
        "1110010", "1100110", "1101100", "1000010", "1011100",
        "1001110", "1010000", "1000100", "1001000", "1110100"
    };
    static const char *parity[] = {
        "OOOOOO", "OOEOEE", "OOEEOE", "OOEEEO", "OEOOEE",
        "OEEOOE", "OEEEOO", "OEOEOE", "OEOEEO", "OEEOEO"
    };

    uint8_t idx = 0;
    uint8_t first = (uint8_t)(digits[0] - '0');

    ean13_append_guard(modules, &idx, "101");
    for (uint8_t i = 1; i <= 6; i++) {
        uint8_t digit = (uint8_t)(digits[i] - '0');
        ean13_append_pattern(modules, &idx,
                             parity[first][i - 1] == 'O' ? left_odd[digit] : left_even[digit]);
    }
    ean13_append_guard(modules, &idx, "01010");
    for (uint8_t i = 7; i <= 12; i++) {
        ean13_append_pattern(modules, &idx, right[(uint8_t)(digits[i] - '0')]);
    }
    ean13_append_guard(modules, &idx, "101");
}

static esp_err_t ean13_render_image(const char digits[14],
                                    const escpos_barcode_config_t *config,
                                    uint16_t print_area_width,
                                    escpos_image_t *image)
{
    uint8_t modules[95] = {0};
    uint8_t module_width = config->width;
    uint8_t quiet_modules = 11;
    uint16_t width = (uint16_t)((95 + (quiet_modules * 2)) * module_width);
    uint16_t height = config->height;
    size_t row_bytes = (width + 7) / 8;
    size_t bitmap_size = row_bytes * height;

    uint8_t *bitmap = (uint8_t *)calloc(bitmap_size, 1);
    ESP_RETURN_ON_FALSE(bitmap, ESP_ERR_NO_MEM, TAG, "Failed to allocate EAN13 bitmap");

    ean13_build_modules(digits, modules);

    for (uint16_t y = 0; y < height; y++) {
        for (uint8_t module = 0; module < 95; module++) {
            if (!modules[module]) {
                continue;
            }
            uint16_t start_x = (uint16_t)((quiet_modules + module) * module_width);
            for (uint8_t dx = 0; dx < module_width; dx++) {
                uint16_t x = (uint16_t)(start_x + dx);
                bitmap[(y * row_bytes) + (x >> 3)] |= (uint8_t)(1 << (7 - (x & 7)));
            }
        }
    }

    image->width = width;
    image->height = height;
    image->bitmap_data = bitmap;
    image->bitmap_size = bitmap_size;
    image->print_area_width = print_area_width;
    image->align = config->align;
    return ESP_OK;
}

static void code128_draw_symbol(uint8_t *bitmap,
                                uint16_t row_bytes,
                                uint16_t height,
                                uint16_t *x,
                                uint8_t module_width,
                                uint8_t symbol)
{
    const char *pattern = code128_patterns[symbol];
    bool black = true;

    for (uint8_t i = 0; pattern[i] != '\0'; i++) {
        uint8_t width = (uint8_t)(pattern[i] - '0');

        if (black) {
            for (uint8_t module = 0; module < width; module++) {
                for (uint8_t dx = 0; dx < module_width; dx++) {
                    uint16_t pixel_x = (uint16_t)(*x + (module * module_width) + dx);
                    for (uint16_t y = 0; y < height; y++) {
                        bitmap[(y * row_bytes) + (pixel_x >> 3)] |=
                            (uint8_t)(1 << (7 - (pixel_x & 7)));
                    }
                }
            }
        }

        *x = (uint16_t)(*x + (width * module_width));
        black = !black;
    }
}

static esp_err_t code128_render_image(const char *payload,
                                      size_t payload_len,
                                      const escpos_barcode_config_t *config,
                                      uint16_t print_area_width,
                                      escpos_image_t *image)
{
    uint8_t module_width = config->width;
    uint8_t quiet_modules = 10;
    uint16_t symbol_count = (uint16_t)(payload_len + 3); /* start + data + checksum + stop */
    uint16_t module_count = (uint16_t)(((symbol_count - 1) * 11) + 13 + (quiet_modules * 2));
    uint16_t width = (uint16_t)(module_count * module_width);
    uint16_t height = config->height;
    size_t row_bytes = (width + 7) / 8;
    size_t bitmap_size = row_bytes * height;

    uint8_t *bitmap = (uint8_t *)calloc(bitmap_size, 1);
    ESP_RETURN_ON_FALSE(bitmap, ESP_ERR_NO_MEM, TAG, "Failed to allocate CODE128 bitmap");

    uint16_t x = (uint16_t)(quiet_modules * module_width);
    code128_draw_symbol(bitmap, row_bytes, height, &x, module_width, 104);

    for (size_t i = 0; i < payload_len; i++) {
        code128_draw_symbol(bitmap, row_bytes, height, &x, module_width,
                            (uint8_t)((unsigned char)payload[i] - 32));
    }

    code128_draw_symbol(bitmap, row_bytes, height, &x, module_width,
                        code128_checksum_b(payload, payload_len));
    code128_draw_symbol(bitmap, row_bytes, height, &x, module_width, 106);

    image->width = width;
    image->height = height;
    image->bitmap_data = bitmap;
    image->bitmap_size = bitmap_size;
    image->print_area_width = print_area_width ? print_area_width : width;
    image->align = config->align;
    return ESP_OK;
}

static esp_err_t escpos_print_code128_raster(escpos_printer_t *printer,
                                             const char *data,
                                             const escpos_barcode_config_t *config)
{
    const char *payload = NULL;
    size_t payload_len = 0;
    ESP_RETURN_ON_FALSE(code128_normalize_code_b(data, &payload, &payload_len),
                        ESP_ERR_INVALID_ARG, TAG,
                        "CODE128 currently supports printable Code Set B data");

    if (config->align != ESCPOS_ALIGN_LEFT ||
        config->hri == ESCPOS_BARCODE_HRI_ABOVE ||
        config->hri == ESCPOS_BARCODE_HRI_BOTH ||
        config->hri == ESCPOS_BARCODE_HRI_BELOW) {
        ESP_RETURN_ON_ERROR(escpos_require_paper_width(printer),
                            TAG, "Cannot align CODE128 without paper width");
    }

    if (config->hri == ESCPOS_BARCODE_HRI_ABOVE || config->hri == ESCPOS_BARCODE_HRI_BOTH) {
        ESP_RETURN_ON_ERROR(escpos_write_text_aligned(printer, payload, config->align),
                            TAG, "Failed to print CODE128 HRI above");
        ESP_RETURN_ON_ERROR(escpos_write_text(printer, "\r\n"),
                            TAG, "Failed to print CODE128 HRI line break");
    }

    escpos_image_t image = {0};
    ESP_RETURN_ON_ERROR(code128_render_image(payload, payload_len, config,
                                             printer->width_dots, &image),
                        TAG, "Failed to render CODE128 image");

    esp_err_t ret = escpos_print_image(printer, &image, ESCPOS_IMAGE_MODE_NORMAL);
    escpos_image_free(&image);
    if (ret != ESP_OK) {
        return ret;
    }

    if (config->hri == ESCPOS_BARCODE_HRI_BELOW || config->hri == ESCPOS_BARCODE_HRI_BOTH) {
        ESP_RETURN_ON_ERROR(escpos_write_text(printer, "\r\n"),
                            TAG, "Failed to print CODE128 HRI spacing");
        return escpos_write_text_aligned(printer, payload, config->align);
    }

    return ESP_OK;
}

static esp_err_t escpos_print_ean13_raster(escpos_printer_t *printer,
                                           const char *data,
                                           const escpos_barcode_config_t *config)
{
    if (config->align != ESCPOS_ALIGN_LEFT ||
        config->hri == ESCPOS_BARCODE_HRI_ABOVE ||
        config->hri == ESCPOS_BARCODE_HRI_BOTH ||
        config->hri == ESCPOS_BARCODE_HRI_BELOW) {
        ESP_RETURN_ON_ERROR(escpos_require_paper_width(printer),
                            TAG, "Cannot align EAN13 without paper width");
    }

    char digits[14];
    ESP_RETURN_ON_FALSE(ean13_normalize(data, digits),
                        ESP_ERR_INVALID_ARG, TAG, "Invalid EAN13 data or checksum");

    if (config->hri == ESCPOS_BARCODE_HRI_ABOVE || config->hri == ESCPOS_BARCODE_HRI_BOTH) {
        ESP_RETURN_ON_ERROR(escpos_write_text_aligned(printer, digits, config->align),
                            TAG, "Failed to print EAN13 HRI above");
        ESP_RETURN_ON_ERROR(escpos_write_text(printer, "\r\n"),
                            TAG, "Failed to print EAN13 HRI line break");
    }

    escpos_image_t image = {0};
    ESP_RETURN_ON_ERROR(ean13_render_image(digits, config, printer->width_dots, &image),
                        TAG, "Failed to render EAN13 image");

    esp_err_t ret = escpos_print_image(printer, &image, ESCPOS_IMAGE_MODE_NORMAL);
    escpos_image_free(&image);
    if (ret != ESP_OK) {
        return ret;
    }

    if (config->hri == ESCPOS_BARCODE_HRI_BELOW || config->hri == ESCPOS_BARCODE_HRI_BOTH) {
        ESP_RETURN_ON_ERROR(escpos_write_text(printer, "\r\n"),
                            TAG, "Failed to print EAN13 HRI spacing");
        return escpos_write_text_aligned(printer, digits, config->align);
    }

    return ESP_OK;
}

esp_err_t escpos_print_barcode(escpos_printer_t *printer,
                               const char *data,
                               const escpos_barcode_config_t *config)
{
    ESP_RETURN_ON_FALSE(printer && data && config, ESP_ERR_INVALID_ARG, TAG,
                        "Invalid barcode argument");
    ESP_RETURN_ON_FALSE(config->type >= ESCPOS_BARCODE_UPC_A &&
                        config->type <= ESCPOS_BARCODE_CODE128,
                        ESP_ERR_INVALID_ARG, TAG, "Invalid barcode type");
    ESP_RETURN_ON_FALSE(config->width >= 2 && config->width <= 6,
                        ESP_ERR_INVALID_ARG, TAG,
                        "Barcode width must be 2..6");
    ESP_RETURN_ON_FALSE(config->height > 0,
                        ESP_ERR_INVALID_ARG, TAG,
                        "Barcode height must be 1..255");
    ESP_RETURN_ON_FALSE(config->hri <= ESCPOS_BARCODE_HRI_BOTH,
                        ESP_ERR_INVALID_ARG, TAG, "Invalid barcode HRI position");
    ESP_RETURN_ON_FALSE(config->hri_font <= ESCPOS_FONT_B,
                        ESP_ERR_INVALID_ARG, TAG, "Invalid barcode HRI font");
    ESP_RETURN_ON_FALSE(config->align <= ESCPOS_ALIGN_RIGHT,
                        ESP_ERR_INVALID_ARG, TAG, "Invalid barcode alignment");
    if (config->align != ESCPOS_ALIGN_LEFT) {
        ESP_RETURN_ON_ERROR(escpos_require_paper_width(printer),
                            TAG, "Cannot align barcode without paper width");
    }

    if (config->type == ESCPOS_BARCODE_EAN13) {
        return escpos_print_ean13_raster(printer, data, config);
    }
    if (config->type == ESCPOS_BARCODE_CODE128) {
        return escpos_print_code128_raster(printer, data, config);
    }

    const char *payload = data;
    size_t payload_len = strlen(data);

    ESP_RETURN_ON_FALSE(escpos_validate_barcode_data(config->type, payload, payload_len),
                        ESP_ERR_INVALID_ARG, TAG, "Invalid barcode data");
    ESP_RETURN_ON_FALSE(payload_len <= 255, ESP_ERR_INVALID_ARG, TAG,
                        "Barcode data too long");

    uint16_t barcode_width_dots = escpos_estimate_barcode_width_dots(config->type,
                                                                     payload_len,
                                                                     config->width);
    uint8_t height_cmd[] = {CMD_BARCODE_HEIGHT, config->height};
    uint8_t width_cmd[] = {CMD_BARCODE_WIDTH, config->width};
    uint8_t hri_pos_cmd[] = {CMD_BARCODE_HRI_POS, (uint8_t)config->hri};
    uint8_t hri_font_cmd[] = {CMD_BARCODE_HRI_FONT, (uint8_t)config->hri_font};
    uint8_t print_cmd[] = {CMD_BARCODE_PRINT, (uint8_t)config->type, (uint8_t)payload_len};

    ESP_RETURN_ON_ERROR(escpos_write_raw(printer, height_cmd, sizeof(height_cmd)),
                        TAG, "Failed to set barcode height");
    ESP_RETURN_ON_ERROR(escpos_write_raw(printer, width_cmd, sizeof(width_cmd)),
                        TAG, "Failed to set barcode width");
    ESP_RETURN_ON_ERROR(escpos_write_raw(printer, hri_pos_cmd, sizeof(hri_pos_cmd)),
                        TAG, "Failed to set barcode HRI position");
    ESP_RETURN_ON_ERROR(escpos_write_raw(printer, hri_font_cmd, sizeof(hri_font_cmd)),
                        TAG, "Failed to set barcode HRI font");
    ESP_RETURN_ON_ERROR(escpos_write_graphic_alignment(printer,
                                                       barcode_width_dots,
                                                       config->align),
                        TAG, "Failed to write barcode alignment padding");
    ESP_RETURN_ON_ERROR(escpos_write_raw(printer, print_cmd, sizeof(print_cmd)),
                        TAG, "Failed to send barcode command");
    return escpos_write_raw(printer, (const uint8_t *)payload, payload_len);
}
