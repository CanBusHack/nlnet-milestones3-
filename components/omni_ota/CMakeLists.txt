set(srcs "")

if(CONFIG_OMNITRIX_ENABLE_OTA)
  list(APPEND srcs "ota.c")
endif()

if("${IDF_TARGET}" STREQUAL linux)
  idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "include" REQUIRES omni_ble PRIV_REQUIRES omni_libnvs)
else()
  idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "include" REQUIRES app_update bootloader_support esp_partition omni_ble PRIV_REQUIRES omni_libnvs)
endif()

if(CONFIG_OMNITRIX_ENABLE_OTA)
  target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -fsanitize=undefined -fanalyzer)
endif()
