set(srcs "")

if(CONFIG_OMNITRIX_ENABLE_OTA)
  list(APPEND srcs "ota.c")
endif()

idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "include" REQUIRES app_update bootloader_support esp_partition omni_ble omni_libnvs)

if(CONFIG_OMNITRIX_ENABLE_OTA)
  target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -fsanitize=undefined -fanalyzer)
endif()
