# Image Printing Feature - Documentation

## Overview

The image printing feature allows you to load, process, and print images (BMP format) on ESC/POS thermal printers. The library automatically:

- Loads BMP files from file system or memory buffers
- Converts color/grayscale images to monochrome bitmap
- Applies dithering algorithms for better print quality
- Enforces file size limits to prevent memory overflow
- Sends optimized commands to the printer

## Supported Formats

### Input Formats
- **BMP (Bitmap)**: 24-bit RGB, 8-bit grayscale
  - Uncompressed only
  - Width/height limits: up to 1024x1024 pixels (configurable)
  - File size limit: 100 KB (configurable)

### Color Conversion
- **RGB to Monochrome**: Uses luminosity formula (0.299R + 0.587G + 0.114B)
- **Grayscale to Monochrome**: Direct conversion with dithering

## Dithering Algorithms

### 1. ESCPOS_DITHER_THRESHOLD
- Simple binary threshold (128 midpoint)
- Fast, minimal memory usage
- May show banding in gradients
- Best for: logos, diagrams, text

### 2. ESCPOS_DITHER_FLOYD_STEINBERG
- Advanced error diffusion dithering
- Better image quality with gradients
- Requires temporary error buffer
- Slightly slower than threshold
- Best for: photographs, detailed images

## API Reference

### Initialization

```c
/* Get default parameters */
escpos_image_params_t params = escpos_image_get_default_params();

/* Customize parameters */
params.dither_mode = ESCPOS_DITHER_FLOYD_STEINBERG;
params.max_width = 384;      /* Print width in dots */
params.auto_scale = true;     /* Auto-resize if too large */
```

### Loading Images

#### From File
```c
escpos_image_t image = {0};
esp_err_t err = escpos_image_load_from_file(
    "/sdcard/logo.bmp",
    &params,
    &image
);

if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to load: %s", esp_err_to_name(err));
}
```

#### From Memory Buffer
```c
const uint8_t *bmp_data = /* ... */;
size_t bmp_size = /* ... */;

escpos_image_t image = {0};
esp_err_t err = escpos_image_load_from_buffer(
    bmp_data,
    bmp_size,
    &params,
    &image
);
```

### Printing Images

```c
/* Print at normal size */
escpos_print_image(printer, &image, ESCPOS_IMAGE_MODE_NORMAL);

/* Print at double width */
escpos_print_image(printer, &image, ESCPOS_IMAGE_MODE_DOUBLE_W);

/* Print at double height */
escpos_print_image(printer, &image, ESCPOS_IMAGE_MODE_DOUBLE_H);

/* Print at double width and height */
escpos_print_image(printer, &image, ESCPOS_IMAGE_MODE_DOUBLE_WH);
```

### Cleanup
```c
escpos_image_free(&image);
```

## Configuration

Edit `escpos_config.h` to customize limits:

```c
/* Maximum image file size (bytes) */
#define ESCPOS_MAX_IMAGE_SIZE_BYTES (100 * 1024)  /* 100 KB */

/* Maximum image width (pixels) - typical printer is 384 or 512 */
#define ESCPOS_MAX_IMAGE_WIDTH 384

/* Maximum image height (pixels) */
#define ESCPOS_MAX_IMAGE_HEIGHT 1024

/* Maximum bitmap buffer size (bytes) */
#define ESCPOS_MAX_BITMAP_SIZE_BYTES (48 * 1024)  /* 48 KB */
```

## Memory Requirements

### Heap Usage During Processing
1. **BMP Loading**: ~5-10 KB (file I/O buffer)
2. **RGB Conversion**: width * height bytes (grayscale intermediate)
3. **Dithering**: 
   - Threshold: ~0 KB extra
   - Floyd-Steinberg: ~width * 2 bytes (error buffer)
4. **Final Bitmap**: (width + 7) / 8 * height bytes

### Example: 384x384 Image
- Input BMP: ~450 KB (24-bit)
- Grayscale buffer: 147 KB
- Final bitmap: ~18.5 KB
- Total peak: ~600 KB

### Recommendations
- For memory-constrained ESP32 (4 MB PSRAM):
  - Max image: 400x400 pixels
  - Use THRESHOLD dithering (less memory)
  - Disable Floyd-Steinberg

- For ESP32-S3 with 8 MB PSRAM:
  - Max image: 768x768 pixels
  - Can use Floyd-Steinberg for better quality

## Error Codes

| Error | Meaning | Solution |
|-------|---------|----------|
| `ESP_ERR_INVALID_ARG` | Invalid parameters | Check image dimensions, file size |
| `ESP_ERR_NO_MEM` | Memory allocation failed | Reduce image size or dithering mode |
| `ESP_ERR_NOT_FOUND` | File not found | Verify file path, check SD card mount |
| `ESP_FAIL` | Unsupported format or corrupted file | Ensure file is valid BMP, uncompressed |

## Performance Tips

### 1. Image Optimization
- Pre-resize images to target width (384px for 58mm printer)
- Use 8-bit grayscale instead of 24-bit RGB (3x smaller)
- Compress to the lowest quality that's acceptable

### 2. Dithering Selection
- Use `THRESHOLD` for logos/icons (faster)
- Use `FLOYD_STEINBERG` for photos (better quality)

### 3. Batch Processing
```c
/* Create a buffer of commands instead of printing immediately */
uint8_t buffer[2048];
size_t len = 0;

/* Add multiple images to buffer... */
escpos_write_raw(printer, buffer, len);
```

### 4. Printing Large Documents
```c
/* For multiple images, free memory after each print */
for (int i = 0; i < num_images; i++) {
    escpos_image_load_from_file(paths[i], &params, &image);
    escpos_print_image(printer, &image, mode);
    escpos_image_free(&image);  /* Important! */
    escpos_feed_lines(printer, 2);
}
```

## Example: Complete Receipt

```c
void print_receipt(escpos_printer_t *printer) {
    escpos_init_printer(printer);
    
    /* Print header */
    escpos_write_text(printer, "COMPANY STORE\n");
    
    /* Print logo */
    escpos_image_params_t params = escpos_image_get_default_params();
    escpos_image_t logo = {0};
    
    if (escpos_image_load_from_file("/sdcard/logo.bmp", &params, &logo) == ESP_OK) {
        escpos_print_image(printer, &logo, ESCPOS_IMAGE_MODE_NORMAL);
        escpos_image_free(&logo);
        escpos_feed_lines(printer, 2);
    }
    
    /* Print items */
    escpos_write_text(printer, "Item 1.........$10.00\n");
    escpos_write_text(printer, "Item 2.........$20.00\n");
    escpos_write_text(printer, "==================\n");
    escpos_write_text(printer, "Total.........$30.00\n");
    
    /* Print QR code */
    escpos_image_t qr = {0};
    if (escpos_image_load_from_file("/sdcard/qrcode.bmp", &params, &qr) == ESP_OK) {
        escpos_print_image(printer, &qr, ESCPOS_IMAGE_MODE_NORMAL);
        escpos_image_free(&qr);
    }
    
    /* Finish receipt */
    escpos_feed_lines(printer, 3);
    escpos_cut_paper(printer);
}
```

## Limitations

1. **BMP Only**: Other formats (PNG, JPG) not currently supported
2. **Uncompressed Only**: No RLE or other compression
3. **Monochrome Output**: Always converts to 1-bit black/white
4. **File I/O**: Requires file system for file loading

## Future Enhancements

- [ ] PNG/JPG format support
- [ ] Image scaling/resizing functions
- [ ] Contrast adjustment
- [ ] Edge detection algorithms
- [ ] Image caching
- [ ] USB buffer optimization for large images

## Troubleshooting

### Image prints but looks distorted
- **Cause**: Width mismatch between image and printer
- **Solution**: Set `params.max_width` to printer's actual width (usually 384 or 512)

### "Image file too large" error
- **Cause**: File exceeds `ESCPOS_MAX_IMAGE_SIZE_BYTES`
- **Solution**: Compress image or increase limit in config

### Memory allocation failed
- **Cause**: Not enough heap during processing
- **Solution**: Reduce image size, disable Floyd-Steinberg dithering, free other resources

### Image prints as blank
- **Cause**: Image format not recognized or file path invalid
- **Solution**: Verify file is uncompressed BMP, check file path, enable logging

### Slow printing
- **Cause**: Floyd-Steinberg dithering on large image
- **Solution**: Use THRESHOLD dithering, pre-resize image

## See Also
- [ESC/POS Command Reference](./escpos_commands.h)
- [Configuration Options](./escpos_config.h)
- [Example Applications](./examples/)
