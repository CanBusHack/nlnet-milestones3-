if("${IDF_TARGET}" STREQUAL linux)
  idf_component_register(SRCS "main.c" "tests.c" INCLUDE_DIRS "." REQUIRES omni_ble omni_ota omni_hello)
else()
  idf_component_register(SRCS "main.c" INCLUDE_DIRS "." REQUIRES omni_ble omni_ota omni_hello)
endif()

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -fsanitize=undefined -fanalyzer)
