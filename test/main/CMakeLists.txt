idf_component_register(SRCS "test.c" "ble.c" INCLUDE_DIRS "." REQUIRES bt driver protocomm nvs_flash unity WHOLE_ARCHIVE)
