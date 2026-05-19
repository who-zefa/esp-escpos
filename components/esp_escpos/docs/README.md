# ESP ESC/POS Usage Docs

This folder explains how to use the `esp_escpos` component in an ESP-IDF app.

Start here:

1. Add `components/esp_escpos` to your ESP-IDF project.
2. Include the public API:

```c
#include "escpos.h"
```

3. Initialize the library, connect a printer, and initialize the printer with paper width:

```c
escpos_printer_t *printer = NULL;

ESP_ERROR_CHECK(escpos_init());
ESP_ERROR_CHECK(escpos_new_usb(&printer));
ESP_ERROR_CHECK(escpos_init_printer_with_width_mm(printer, 58));
```

4. Print, feed, cut, and clean up:

```c
ESP_ERROR_CHECK(escpos_write_text(printer, "Hello\n"));
ESP_ERROR_CHECK(escpos_feed_lines(printer, 3));
ESP_ERROR_CHECK(escpos_cut_paper(printer));

escpos_destroy(printer);
ESP_ERROR_CHECK(escpos_deinit());
```

## What To Read

- [API Usage Guide](API.md): common printing tasks and function usage
- [Image Printing](IMAGE_PRINTING.md): BMP image loading and printing
- [Receipt Printing](RECEIPTS.md): practical receipt layout with columns and totals
- [Barcodes And QR Codes](BARCODES_AND_QR.md): barcode and QR code printing
- [Printer Compatibility](PRINTER_COMPATIBILITY.md): printer setup and width notes

## Important Pattern

Call this once after opening the printer:

```c
ESP_ERROR_CHECK(escpos_init_printer_with_width_mm(printer, 58));
```

Use:

- `58` for common 58 mm printers
- `80` for common 80 mm printers

The library does not ask the printer for paper width. It uses the width you configure to calculate alignment, columns, image placement, QR placement, and barcode placement.
