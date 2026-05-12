idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES usb_host_cdc_acm usb esp_escpos)
