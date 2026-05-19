# ESP ESC/POS Examples

Each file is a small standalone example. Copy one file into your app's `main`
component as `main.c`, or use it as a reference for the feature you need.

Do not compile all of these files together because each one contains `app_main`.

Initialize the printer with the paper width before printing aligned text,
images, QR codes, barcodes, or columns:

```c
ESP_ERROR_CHECK(escpos_init_printer_with_width_mm(printer, 58));
```

## Files

- `text_example.c`: text styles, sizes, host-side alignment, and receipt columns
- `columns_example.c`: 2-column totals and 3/4-column item rows
- `barcode_example.c`: Code128 and EAN13 barcode printing
- `qr_example.c`: QR code rendered by the library and printed as image data
- `image_example.c`: BMP image loading, dithering, alignment, and printing
- `raw_commands_example.c`: direct ESC/POS byte commands
- `beeps_example.c`: printer buzzer
- `cut_example.c`: paper feed and cut

For `image_example.c`, add a BMP file to your app and embed it:

```cmake
idf_component_register(SRCS "main.c" EMBED_FILES "image.bmp")
```
