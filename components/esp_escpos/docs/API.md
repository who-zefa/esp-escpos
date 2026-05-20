# API Usage Guide

This guide shows the normal ways to use the public `escpos.h` API.

## Basic Lifecycle

```c
#include "esp_err.h"
#include "escpos.h"

void app_main(void)
{
    escpos_printer_t *printer = NULL;

    ESP_ERROR_CHECK(escpos_init());
    ESP_ERROR_CHECK(escpos_new_usb(&printer));
    ESP_ERROR_CHECK(escpos_init_printer_with_width_mm(printer, 58));

    ESP_ERROR_CHECK(escpos_write_text(printer, "Hello\n"));

    ESP_ERROR_CHECK(escpos_feed_lines(printer, 3));
    ESP_ERROR_CHECK(escpos_cut_paper(printer));

    escpos_destroy(printer);
    ESP_ERROR_CHECK(escpos_deinit());
}
```

## Paper Width

Set paper width once when initializing the printer:

```c
ESP_ERROR_CHECK(escpos_init_printer_with_width_mm(printer, 58));
```

Common values:

- `58`: configures 384 dots and 32 text columns
- `80`: configures 576 dots and 48 text columns

For unusual printers, use the lower-level override:

```c
ESP_ERROR_CHECK(escpos_init_printer(printer));
ESP_ERROR_CHECK(escpos_set_printer_width(printer, 512, 42));
```

Width is local configuration. The library does not query the printer.

## Text

Plain text:

```c
escpos_write_text(printer, "Normal text\n");
```

Host-side alignment:

```c
escpos_write_text_aligned(printer, "CENTER", ESCPOS_ALIGN_CENTER);
escpos_write_text(printer, "\n");
```

Styles:

```c
escpos_set_bold(printer, true);
escpos_write_text(printer, "Bold\n");
escpos_set_bold(printer, false);

escpos_set_underline(printer, 1);
escpos_write_text(printer, "Underline\n");
escpos_set_underline(printer, 0);

escpos_set_text_size(printer, 2, 2);
escpos_write_text(printer, "Large\n");
escpos_reset_text_format(printer);
```

## Receipt Columns

Use the helper functions for common POS layouts:

```c
escpos_write_3_columns(printer, "ITEM", "QTY", "PRICE");
escpos_write_3_columns(printer, "Coffee", "2", "7.00");
escpos_write_3_columns(printer, "Sandwich", "1", "6.25");

escpos_write_2_columns(printer, "Subtotal", "$13.25");
escpos_write_2_columns(printer, "Tax", "$1.06");
escpos_write_2_columns(printer, "TOTAL", "$14.31");
```

For custom layouts:

```c
escpos_text_column_t columns[] = {
    {.text = "Item", .width = 20, .align = ESCPOS_ALIGN_LEFT},
    {.text = "Qty", .width = 4, .align = ESCPOS_ALIGN_RIGHT},
    {.text = "Total", .width = 8, .align = ESCPOS_ALIGN_RIGHT},
};

escpos_write_columns(printer, columns, 3);
```

## Barcode

```c
escpos_barcode_config_t barcode =
    escpos_barcode_get_default_config(ESCPOS_BARCODE_CODE128);

barcode.align = ESCPOS_ALIGN_CENTER;
barcode.height = 80;
barcode.hri = ESCPOS_BARCODE_HRI_BELOW;

ESP_ERROR_CHECK(escpos_print_barcode(printer, "TXN12345", &barcode));
```

EAN13:

```c
escpos_barcode_config_t ean =
    escpos_barcode_get_default_config(ESCPOS_BARCODE_EAN13);

ean.align = ESCPOS_ALIGN_CENTER;
ESP_ERROR_CHECK(escpos_print_barcode(printer, "5901234123457", &ean));
```

EAN13 accepts 12 or 13 digits. With 12 digits, the checksum is generated. With 13 digits, the checksum is validated.

Code128 and EAN13 are rendered locally and sent as raster image data, so they do not rely on the printer barcode command implementation.

## QR Code

```c
escpos_qr_config_t qr = escpos_qr_get_default_config();
qr.align = ESCPOS_ALIGN_CENTER;
qr.size = 6;
qr.ec_level = ESCPOS_QR_EC_M;

ESP_ERROR_CHECK(escpos_print_qr(printer, "https://example.com/qr", &qr));
```

The current local QR encoder supports version 2, byte mode, EC-M, up to 26 bytes of data.

## Images

See `IMAGE_PRINTING.md` for a full image guide. Minimal flow:

```c
escpos_image_params_t params = escpos_image_get_default_params();
params.max_width = escpos_get_printer_width_dots(printer);
params.align = ESCPOS_ALIGN_CENTER;

escpos_image_t image = {0};
ESP_ERROR_CHECK(escpos_image_load_from_buffer(bmp_start, bmp_len, &params, &image));
ESP_ERROR_CHECK(escpos_print_image(printer, &image, ESCPOS_IMAGE_MODE_NORMAL));
escpos_image_free(&image);
```

## Raw Commands

Use raw commands when the library does not expose a helper:

```c
const uint8_t bold_on[] = {0x1B, 0x45, 0x01};
ESP_ERROR_CHECK(escpos_write_command(printer, bold_on, sizeof(bold_on)));
```

`escpos_write_command()` is an alias for `escpos_write_raw()` and exists to make direct ESC/POS command usage readable.

## Feed, Cut, Beep

```c
escpos_feed_line(printer);
escpos_feed_lines(printer, 3);
escpos_cut_paper(printer);
escpos_cut_paper_partial(printer);
escpos_beep(printer);
```

Some printers do not support cut or beep. In that case the command may be ignored by the printer.

## Connection

```c
if (escpos_is_connected(printer)) {
    escpos_write_text(printer, "Connected\n");
}

escpos_wait_disconnect(printer, portMAX_DELAY);
```

Always destroy the handle when finished:

```c
escpos_destroy(printer);
```
