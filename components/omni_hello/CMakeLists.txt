set(srcs "")

if(CONFIG_OMNITRIX_ENABLE_HELLO)
  list(APPEND srcs "hello.c" "isotp.c")
endif()

if("${IDF_TARGET}" STREQUAL linux)
  idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "include" REQUIRES omni_ble PRIV_REQUIRES omni_libvin)
else()
  idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "include" REQUIRES driver omni_ble PRIV_REQUIRES omni_libvin)
endif()

if(CONFIG_OMNITRIX_ENABLE_HELLO)
  target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -fsanitize=undefined -fanalyzer)
endif()
