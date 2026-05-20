# Barcodes And QR Codes

The library exposes one API for barcodes and one API for QR codes.

## Barcode

```c
escpos_barcode_config_t barcode =
    escpos_barcode_get_default_config(ESCPOS_BARCODE_CODE128);

barcode.align = ESCPOS_ALIGN_CENTER;
barcode.width = 2;
barcode.height = 80;
barcode.hri = ESCPOS_BARCODE_HRI_BELOW;

ESP_ERROR_CHECK(escpos_print_barcode(printer, "TXN12345", &barcode));
```

## EAN13

```c
escpos_barcode_config_t ean =
    escpos_barcode_get_default_config(ESCPOS_BARCODE_EAN13);

ean.align = ESCPOS_ALIGN_CENTER;
ean.width = 3;
ean.height = 80;

ESP_ERROR_CHECK(escpos_print_barcode(printer, "5901234123457", &ean));
```

EAN13 accepts:

- 12 digits: checksum is generated
- 13 digits: checksum is validated

Code128 and EAN13 are rendered locally and sent as raster image data. This avoids relying on weak or inconsistent printer barcode command support.

The Code128 renderer currently uses Code Set B, which covers normal printable text such as order IDs and transaction IDs.

## QR Code

```c
escpos_qr_config_t qr = escpos_qr_get_default_config();
qr.align = ESCPOS_ALIGN_CENTER;
qr.size = 6;
qr.ec_level = ESCPOS_QR_EC_M;

ESP_ERROR_CHECK(escpos_print_qr(printer, "https://example.com/qr", &qr));
```

Current QR encoder limits:

- version 2
- byte mode
- EC-M
- up to 26 bytes

QR codes are rendered locally and sent as raster image data.

## Alignment

Call this before printing centered or right-aligned barcodes/QR codes:

```c
ESP_ERROR_CHECK(escpos_init_printer_with_width_mm(printer, 58));
```

The configured paper width is used to calculate placement.
