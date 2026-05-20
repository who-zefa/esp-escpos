# Printer Compatibility

This component is intended for ESC/POS-compatible thermal receipt printers.
The current build uses USB through the ESP-IDF USB host stack.

## Paper Width

The library does not ask the printer for paper width. Set the width in your app:

```c
ESP_ERROR_CHECK(escpos_init_printer_with_width_mm(printer, 58));
```

Common mappings:

- `58` mm: 384 dots, 32 text columns
- `80` mm: 576 dots, 48 text columns

For unusual mechanisms, configure the exact layout width:

```c
ESP_ERROR_CHECK(escpos_init_printer(printer));
ESP_ERROR_CHECK(escpos_set_printer_width(printer, 512, 42));
```

This width is used locally for aligned text, receipt columns, image placement,
barcode placement, and QR placement.

## USB Printers

Use the default USB VID/PID from the component configuration:

```c
escpos_printer_t *printer = NULL;

ESP_ERROR_CHECK(escpos_init());
ESP_ERROR_CHECK(escpos_new_usb(&printer));
ESP_ERROR_CHECK(escpos_init_printer_with_width_mm(printer, 58));
```

Use explicit USB IDs when needed:

```c
ESP_ERROR_CHECK(escpos_new_usb_with_ids(0x0416, 0x5011, &printer));
```

The correct VID/PID depends on the printer USB adapter. Check the device with
your operating system or the printer vendor tool.

## Cut And Beep

Many ESC/POS printers accept cut and beep commands:

```c
escpos_feed_lines(printer, 3);
escpos_cut_paper(printer);
escpos_beep(printer);
```

Some low-cost printers ignore one or both commands. If text prints but cut or
beep does nothing, the printer likely does not support that command.

## Barcodes And QR Codes

Code128, EAN13, and QR codes are rendered by the library and sent as raster image data.
This is useful for printers with incomplete barcode or QR command support.

For barcode printing, start with:

```c
escpos_barcode_config_t barcode =
    escpos_barcode_get_default_config(ESCPOS_BARCODE_CODE128);
```

If a printer has trouble with command-based barcodes, prefer Code128, EAN13, or
QR because those paths do not depend on the printer's barcode firmware.

## Images

Use uncompressed BMP files. A good starting size is:

- 384 px wide or less for 58 mm paper
- 576 px wide or less for 80 mm paper

When image printing fails, first test with a small black-and-white BMP, then
increase size or enable dithering.

## Troubleshooting

Nothing prints:

- Confirm the printer has paper and power.
- Confirm the USB VID/PID.
- Confirm the ESP target supports USB host mode.
- Check that `escpos_init()` and `escpos_new_usb()` return `ESP_OK`.

Alignment is wrong:

- Call `escpos_init_printer_with_width_mm()` once after connecting.
- Use `58` or `80` to match the actual paper/mechanism.
- For nonstandard printers, use `escpos_set_printer_width()`.

Text prints but barcode, QR, or image is shifted:

- Make sure paper width is configured before printing.
- Keep image width at or below `escpos_get_printer_width_dots(printer)`.

Printer cuts too early:

- Feed a few lines before cutting:

```c
escpos_feed_lines(printer, 3);
escpos_cut_paper(printer);
```
