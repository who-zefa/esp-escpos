# Receipt Printing

This guide shows the usual receipt-building pattern: header, items, totals, QR or barcode, feed, cut.

## Recommended Setup

```c
escpos_printer_t *printer = NULL;

ESP_ERROR_CHECK(escpos_init());
ESP_ERROR_CHECK(escpos_new_usb(&printer));
ESP_ERROR_CHECK(escpos_init_printer_with_width_mm(printer, 58));
```

## Header

```c
escpos_set_text_size(printer, 2, 2);
escpos_write_text_aligned(printer, "MY STORE", ESCPOS_ALIGN_CENTER);
escpos_write_text(printer, "\n");
escpos_reset_text_format(printer);

escpos_write_text_aligned(printer, "123 Main Street", ESCPOS_ALIGN_CENTER);
escpos_write_text(printer, "\n\n");
```

## Items

Three-column item rows:

```c
escpos_write_3_columns(printer, "ITEM", "QTY", "PRICE");
escpos_write_3_columns(printer, "Coffee", "2", "7.00");
escpos_write_3_columns(printer, "Sandwich", "1", "6.25");
```

Four-column item rows:

```c
escpos_write_4_columns(printer, "ITEM", "QTY", "RATE", "TOTAL");
escpos_write_4_columns(printer, "Coffee", "2", "3.50", "7.00");
```

Totals:

```c
escpos_write_2_columns(printer, "Subtotal", "$13.25");
escpos_write_2_columns(printer, "Tax", "$1.06");
escpos_write_2_columns(printer, "TOTAL", "$14.31");
```

## QR Or Barcode

```c
escpos_qr_config_t qr = escpos_qr_get_default_config();
qr.align = ESCPOS_ALIGN_CENTER;
qr.size = 6;

escpos_write_text(printer, "\n");
escpos_print_qr(printer, "receipt.example/TXN12345", &qr);
```

```c
escpos_barcode_config_t barcode =
    escpos_barcode_get_default_config(ESCPOS_BARCODE_CODE128);
barcode.align = ESCPOS_ALIGN_CENTER;

escpos_print_barcode(printer, "TXN12345", &barcode);
```

## Finish

```c
escpos_write_text_aligned(printer, "Thank you!", ESCPOS_ALIGN_CENTER);
escpos_write_text(printer, "\n");
escpos_feed_lines(printer, 3);
escpos_cut_paper(printer);
```

## Full Receipt Example

```c
ESP_ERROR_CHECK(escpos_init_printer_with_width_mm(printer, 58));

escpos_set_bold(printer, true);
escpos_write_text_aligned(printer, "MY STORE", ESCPOS_ALIGN_CENTER);
escpos_write_text(printer, "\n");
escpos_set_bold(printer, false);

escpos_write_text(printer, "Transaction: TXN12345\n\n");

escpos_write_3_columns(printer, "ITEM", "QTY", "PRICE");
escpos_write_3_columns(printer, "Coffee", "2", "7.00");
escpos_write_3_columns(printer, "Sandwich", "1", "6.25");
escpos_write_text(printer, "\n");

escpos_write_2_columns(printer, "Subtotal", "$13.25");
escpos_write_2_columns(printer, "Tax", "$1.06");
escpos_write_2_columns(printer, "TOTAL", "$14.31");
escpos_write_text(printer, "\n");

escpos_qr_config_t qr = escpos_qr_get_default_config();
qr.align = ESCPOS_ALIGN_CENTER;
escpos_print_qr(printer, "receipt.example/TXN12345", &qr);

escpos_write_text(printer, "\n");
escpos_write_text_aligned(printer, "Thank you!", ESCPOS_ALIGN_CENTER);
escpos_write_text(printer, "\n");
escpos_feed_lines(printer, 3);
escpos_cut_paper(printer);
```
