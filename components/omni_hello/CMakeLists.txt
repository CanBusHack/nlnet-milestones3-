set(srcs "")

if(CONFIG_OMNITRIX_ENABLE_HELLO)
  list(APPEND srcs "hello.c" "isotp.c")
endif()

idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "include" REQUIRES driver omni_ble PRIV_REQUIRES omni_libvin)

if(CONFIG_OMNITRIX_ENABLE_HELLO)
  target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -fsanitize=undefined -fanalyzer)
endif()
