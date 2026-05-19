# Image Printing

The image API loads BMP data, converts it to monochrome raster data, and sends it to the printer.

Supported input today:

- BMP
- 24-bit RGB BMP
- 8-bit BMP, with palette support when present
- uncompressed BMP only

PNG and JPG are detected but not decoded by this component yet.

## Basic Flow

```c
escpos_image_params_t params = escpos_image_get_default_params();
params.max_width = escpos_get_printer_width_dots(printer);
params.align = ESCPOS_ALIGN_CENTER;
params.dither_mode = ESCPOS_DITHER_FLOYD_STEINBERG;
params.threshold = 128;

escpos_image_t image = {0};

ESP_ERROR_CHECK(escpos_image_load_from_buffer(
    image_bmp_start,
    (size_t)(image_bmp_end - image_bmp_start),
    &params,
    &image
));

ESP_ERROR_CHECK(escpos_print_image(printer, &image, ESCPOS_IMAGE_MODE_NORMAL));
escpos_image_free(&image);
```

## Embed A BMP In ESP-IDF

Add the file to your app component:

```text
main/
  main.c
  image.bmp
```

In `main/CMakeLists.txt`:

```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    EMBED_FILES "image.bmp"
)
```

In C:

```c
extern const uint8_t image_bmp_start[] asm("_binary_image_bmp_start");
extern const uint8_t image_bmp_end[] asm("_binary_image_bmp_end");

size_t image_len = (size_t)(image_bmp_end - image_bmp_start);
```

## Load From File

Use this when your app has mounted storage such as SD card or SPIFFS:

```c
escpos_image_t image = {0};
escpos_image_params_t params = escpos_image_get_default_params();

params.max_width = escpos_get_printer_width_dots(printer);
params.align = ESCPOS_ALIGN_CENTER;

ESP_ERROR_CHECK(escpos_image_load_from_file("/sdcard/logo.bmp", &params, &image));
ESP_ERROR_CHECK(escpos_print_image(printer, &image, ESCPOS_IMAGE_MODE_NORMAL));
escpos_image_free(&image);
```

## Width And Alignment

Initialize printer width before printing images:

```c
ESP_ERROR_CHECK(escpos_init_printer_with_width_mm(printer, 58));
```

Then use the configured printable width:

```c
params.max_width = escpos_get_printer_width_dots(printer);
params.align = ESCPOS_ALIGN_CENTER;
```

Common configured widths:

- 58 mm: 384 dots
- 80 mm: 576 dots

## Dithering

Simple threshold:

```c
params.dither_mode = ESCPOS_DITHER_THRESHOLD;
params.threshold = 128;
```

Floyd-Steinberg:

```c
params.dither_mode = ESCPOS_DITHER_FLOYD_STEINBERG;
params.threshold = 128;
```

Use threshold for logos and text-heavy images. Use Floyd-Steinberg for photos or gradients.

## Print Modes

```c
escpos_print_image(printer, &image, ESCPOS_IMAGE_MODE_NORMAL);
escpos_print_image(printer, &image, ESCPOS_IMAGE_MODE_DOUBLE_W);
escpos_print_image(printer, &image, ESCPOS_IMAGE_MODE_DOUBLE_H);
escpos_print_image(printer, &image, ESCPOS_IMAGE_MODE_DOUBLE_WH);
```

## Memory Tips

- Pre-resize images to the printer width.
- Use small BMPs for embedded builds.
- Always call `escpos_image_free(&image)` after printing.
- Use `ESCPOS_DITHER_THRESHOLD` when heap is tight.

## Troubleshooting

Image is too wide:

- Set `params.max_width = escpos_get_printer_width_dots(printer)`.
- Pre-resize the BMP to 384 px for 58 mm or 576 px for 80 mm.

Image is blank:

- Confirm the file is BMP and uncompressed.
- Try `params.threshold = 190` to print darker.
- Confirm the BMP data is embedded or loaded correctly.

Image alignment is wrong:

- Call `escpos_init_printer_with_width_mm()` before loading/printing.
- Set `params.align` and `params.max_width`.

Memory allocation fails:

- Reduce image dimensions.
- Use threshold dithering.
- Free other large buffers before loading the image.
