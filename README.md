# ESP ESC/POS

ESP ESC/POS is an ESP-IDF component for printing to ESC/POS thermal receipt printers from ESP32 projects. It focuses on practical POS output: text, receipt columns, barcodes, QR codes, BMP images, raw commands, feed, cut, and beep.

The library does not query the printer for paper width. Configure the paper width once after connecting:

```c
ESP_ERROR_CHECK(escpos_init_printer_with_width_mm(printer, 58));
```

Use `58` for common 58 mm printers and `80` for common 80 mm printers.

## Install

Place the component in your ESP-IDF project:

```text
your_project/
  main/
  components/
    esp_escpos/
```

Then include it from your app:

```c
#include "escpos.h"
```

## Minimal Print

```c
#include "esp_err.h"
#include "escpos.h"

void app_main(void)
{
    escpos_printer_t *printer = NULL;

    ESP_ERROR_CHECK(escpos_init());
    ESP_ERROR_CHECK(escpos_new_usb(&printer));
    ESP_ERROR_CHECK(escpos_init_printer_with_width_mm(printer, 58));

    ESP_ERROR_CHECK(escpos_write_text_aligned(printer, "MY STORE", ESCPOS_ALIGN_CENTER));
    ESP_ERROR_CHECK(escpos_write_text(printer, "\n"));
    ESP_ERROR_CHECK(escpos_write_3_columns(printer, "Coffee", "2", "7.00"));
    ESP_ERROR_CHECK(escpos_write_2_columns(printer, "TOTAL", "$7.00"));

    ESP_ERROR_CHECK(escpos_feed_lines(printer, 3));
    ESP_ERROR_CHECK(escpos_cut_paper(printer));

    escpos_destroy(printer);
    ESP_ERROR_CHECK(escpos_deinit());
}
```

## Common Features

### Text

```c
escpos_set_bold(printer, true);
escpos_write_text(printer, "Bold text\n");
escpos_set_bold(printer, false);

escpos_set_text_size(printer, 2, 2);
escpos_write_text_aligned(printer, "Large", ESCPOS_ALIGN_CENTER);
escpos_reset_text_format(printer);
```

### Receipt Columns

```c
escpos_write_3_columns(printer, "ITEM", "QTY", "PRICE");
escpos_write_3_columns(printer, "Coffee", "2", "7.00");
escpos_write_2_columns(printer, "Subtotal", "$7.00");
escpos_write_2_columns(printer, "TOTAL", "$7.00");
```

### Barcode

```c
escpos_barcode_config_t barcode =
    escpos_barcode_get_default_config(ESCPOS_BARCODE_EAN13);
barcode.align = ESCPOS_ALIGN_CENTER;
escpos_print_barcode(printer, "5901234123457", &barcode);
```

### QR Code

```c
escpos_qr_config_t qr = escpos_qr_get_default_config();
qr.align = ESCPOS_ALIGN_CENTER;
qr.size = 6;
escpos_print_qr(printer, "https://example.com/qr", &qr);
```

### Image

```c
escpos_image_params_t params = escpos_image_get_default_params();
params.max_width = escpos_get_printer_width_dots(printer);
params.align = ESCPOS_ALIGN_CENTER;
params.dither_mode = ESCPOS_DITHER_FLOYD_STEINBERG;

escpos_image_t image = {0};
ESP_ERROR_CHECK(escpos_image_load_from_buffer(bmp_start, bmp_len, &params, &image));
ESP_ERROR_CHECK(escpos_print_image(printer, &image, ESCPOS_IMAGE_MODE_NORMAL));
escpos_image_free(&image);
```

## Examples

Standalone examples live in:

```text
components/esp_escpos/examples/
```

Each file contains its own `app_main`, so copy one example into your app or use it as reference. The main project also contains a combined demo in `main/`.

## Documentation

- [Getting Started](components/esp_escpos/docs/README.md)
- [API Usage Guide](components/esp_escpos/docs/API.md)
- [Receipt Printing](components/esp_escpos/docs/RECEIPTS.md)
- [Barcodes And QR Codes](components/esp_escpos/docs/BARCODES_AND_QR.md)
- [Image Printing](components/esp_escpos/docs/IMAGE_PRINTING.md)
- [Printer Compatibility](components/esp_escpos/docs/PRINTER_COMPATIBILITY.md)

## Notes

- USB is the active transport in the current component build.
- Paper width is local configuration, not printer detection.
- QR, Code128, and EAN13 are rendered locally and sent as raster data where needed.
- BMP image loading supports uncompressed BMP data.
