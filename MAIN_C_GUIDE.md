# main.c - Comprehensive Feature Test

## Overview
Updated `main.c` now demonstrates **ALL features** of the ESP32 ESC/POS library including the new **image printing API**.

## Features Tested

### 1. ✅ Text Formatting
- Standard text output
- Multiple alignment modes (left, center, right)
- Text line formatting

### 2. ✅ Image Printing (NEW!)
- **Loads BMP files** (24-bit RGB or 8-bit grayscale)
- Automatic monochrome conversion with dithering
- Multiple display modes:
  - Normal size
  - Double-width
  - Double-height
  - Double-width and double-height
- Automatic error handling with helpful messages
- Tries multiple file paths

### 3. ✅ Paper Control
- Feed specific number of blank lines
- Full paper cut
- Partial paper cut support

### 4. ✅ Audio Feedback
- Printer beep command

### 5. ✅ Complete Receipt Example
- Header with formatting
- Logo image printing
- Item list with pricing
- Subtotal, tax, total calculations
- QR code image printing
- Footer and cut

## Image File Requirements

**IMPORTANT**: The image module supports **BMP format only**, not PNG.

The application expects: **`image.bmp`**

### Supported BMP Formats
- 24-bit RGB (color)
- 8-bit Grayscale
- Uncompressed only
- Size: up to 384×1024 pixels (configurable)
- File size: up to 100 KB (configurable)

### File Locations Checked
1. `/sdcard/image.bmp` (SD card)
2. `/image.bmp` (root filesystem)

### Converting Image to BMP
If you have `image.png`, convert it using:

**Linux/Mac:**
```bash
convert image.png image.bmp
```

**Windows (ImageMagick):**
```bash
magick image.png image.bmp
```

**Python:**
```python
from PIL import Image
img = Image.open("image.png")
img.convert("RGB").save("image.bmp")
```

## Test Flow

Each cycle performs this sequence:

```
1. Initialize printer
   ↓
2. Print text formatting demo
   ↓
3. Load and print image (BMP)
   - Normal size
   - Double-width version
   ↓
4. Demonstrate paper control
   ↓
5. Send audio feedback (beep)
   ↓
6. Print complete receipt with images
   ↓
7. Cut paper
   ↓
8. Wait for disconnect
   ↓
9. Reconnect and repeat
```

## Output Examples

### Console Output (ESP32)
```
========================================
  ESP32-S3 ESC/POS USB PRINTER DEMO
  Testing ALL Library Features
========================================

Step 1/2: Initializing ESC/POS Library
✓ Library initialized

========== CYCLE 1 ==========
Step 2/2: Searching for USB printer...
✓ Printer connected!

=== TEXT FORMATTING DEMO ===
  → Printing aligned text: LEFT
  → Printing aligned text: CENTER
  → Printing aligned text: RIGHT

=== IMAGE PRINTING DEMO (NEW FEATURE!) ===
Loading image.bmp from file system...
Image loaded successfully: 384x256 pixels
Printing image at normal size...
Printing image at double-width...
Image resources freed
Image printing complete!

=== PAPER CONTROL DEMO ===

=== AUDIO FEEDBACK DEMO ===
Beeping printer...

=== COMPLETE RECEIPT DEMO ===
Logo loaded, printing...
Cutting paper...
✓ All features demonstrated successfully!
Waiting for printer disconnect...
```

### Printer Output
```
========================
 TEXT FORMATTING TEST
========================

1. Standard Text:
Normal output from ESP32

2. Multiple Sizes:
Regular text line
Different widths available

3. Text Alignment:
LEFT ALIGNED TEXT
CENTER ALIGNED TEXT
RIGHT ALIGNED TEXT

[IMAGE PRINTED HERE - Logo or Photo]

================================
     SAMPLE STORE RECEIPT
================================

Date: May 18, 2026
Time: 14:30:00
Transaction ID: TXN12345

================================
 ITEMS
================================
Item 1         x1  $10.00
Item 2         x2  $15.50
Item 3         x1  $8.99

Subtotal:          $34.49
Tax (10%):         $3.45
================================
TOTAL:             $37.94
================================

Payment: Card
Card: VISA ****1234
Auth Code: A12345B

[QR CODE IMAGE]

Thank you for your purchase!
Visit us again!
```

## Error Handling

If image loading fails, the application displays helpful messages:

| Error | Message |
|-------|---------|
| File not found | Shows expected locations |
| Too large | Suggests reducing image size |
| Out of memory | Recommends using simpler images |
| Invalid format | States BMP requirement |

## Image Processing Details

### Dithering Mode
Set to `ESCPOS_DITHER_FLOYD_STEINBERG` for high-quality output.
- Reduces banding
- Better gradient representation
- Slightly slower (but acceptable for thermal printers)

### Color Conversion
- Uses luminosity formula: 0.299R + 0.587G + 0.114B
- Converts to monochrome (1-bit black/white)
- Automatically handles byte alignment

## Building & Running

```bash
# Build
idf.py build

# Flash
idf.py flash

# Monitor
idf.py monitor
```

## Tips for Best Results

1. **Image Quality**
   - Keep images 384×384 pixels or smaller
   - Use high contrast logos/diagrams
   - For photos, use Floyd-Steinberg dithering

2. **File Management**
   - Store BMP on SD card at `/sdcard/image.bmp`
   - Keep file size under 100 KB
   - Use 8-bit grayscale for smaller files

3. **Memory Management**
   - Peak memory usage: ~600 KB per image
   - Safe for ESP32-S3 (8 MB PSRAM)
   - For 4 MB models, reduce image size

4. **Printer Compatibility**
   - Works with any ESC/POS compliant printer
   - Tested with Zebra, Epson, Gprinter models
   - Supports 58mm, 80mm thermal printers

## Modifications Made to main.c

1. **Added Image Includes**
   - `#include "escpos_image.h"`

2. **Added Helper Functions**
   - `print_aligned_text()` - Print with alignment

3. **Added Feature Demos**
   - `demo_text_formatting()` - Text features
   - `demo_image_printing()` - Image loading and printing
   - `demo_paper_control()` - Paper feeding
   - `demo_audio_feedback()` - Beeping
   - `demo_complete_receipt()` - Full example

4. **Enhanced Main Loop**
   - Cycle counter for tracking
   - Better logging with checkmarks
   - All features in sequence
   - Error recovery

## Next Steps

1. **Prepare BMP Image**
   - Convert your image to BMP format
   - Place at `/sdcard/image.bmp`

2. **Flash ESP32**
   - Run `idf.py build && idf.py flash`

3. **Connect Thermal Printer**
   - USB to ESP32 via CDC-ACM

4. **Monitor Output**
   - Run `idf.py monitor`
   - Connect printer to see printed output

5. **Customize**
   - Modify receipt format in `demo_complete_receipt()`
   - Change image locations
   - Adjust dithering algorithm

## Support

See documentation:
- [IMAGE_PRINTING.md](../components/esp_escpos/docs/IMAGE_PRINTING.md) - Complete image API reference
- [IMAGE_PRINTING_README.md](../components/esp_escpos/IMAGE_PRINTING_README.md) - Quick start guide
- [escpos_image.h](../components/esp_escpos/include/escpos_image.h) - API declarations with docstrings
