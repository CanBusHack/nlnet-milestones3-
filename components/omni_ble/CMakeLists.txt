idf_component_register(SRCS "ble.c" INCLUDE_DIRS "include" REQUIRES nvs_flash bt protocomm)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -fsanitize=undefined -fanalyzer)
