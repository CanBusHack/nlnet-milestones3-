idf_component_register(SRCS "main.c" INCLUDE_DIRS "." REQUIRES omni_ble omni_ota)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -fsanitize=undefined -fanalyzer)
