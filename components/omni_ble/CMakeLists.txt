set(srcs "")

if(CONFIG_OMNITRIX_ENABLE_BLE)
  list(APPEND srcs "ble.c")
endif()

if("${IDF_TARGET}" STREQUAL linux)
  idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "include" PRIV_REQUIRES omni_libnvs)
else()
  idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "include" REQUIRES bt PRIV_REQUIRES protocomm omni_libnvs)
endif()

if(CONFIG_OMNITRIX_ENABLE_BLE)
  target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -fsanitize=undefined -fanalyzer)
endif()
