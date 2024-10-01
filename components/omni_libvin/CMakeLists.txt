idf_component_register(SRCS "libvin.c" INCLUDE_DIRS "include" REQUIRES driver)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -fsanitize=undefined -fanalyzer)
