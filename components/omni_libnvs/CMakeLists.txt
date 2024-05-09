idf_component_register(SRCS "libnvs.c" INCLUDE_DIRS "include" PRIV_REQUIRES nvs_flash)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -fsanitize=undefined -fanalyzer)
