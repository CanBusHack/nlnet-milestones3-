idf_component_register(SRCS "test.c" "ble/hello/handle.c" "ble/hello/uuid.c" "can/raw/read.c" "can/raw/write.c" INCLUDE_DIRS "." REQUIRES bt driver protocomm nvs_flash unity WHOLE_ARCHIVE)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -fsanitize=undefined -fanalyzer)
