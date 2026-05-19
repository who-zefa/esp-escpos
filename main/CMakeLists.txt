idf_component_register(SRCS
                        "main.c"
                        "escpos_examples.c"
                    INCLUDE_DIRS "."
                    EMBED_FILES "image.bmp"
                    REQUIRES usb_host_cdc_acm usb esp_escpos)
