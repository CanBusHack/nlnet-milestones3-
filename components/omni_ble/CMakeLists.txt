set(srcs "")

if(CONFIG_OMNITRIX_ENABLE_BLE)
  list(APPEND srcs "ble.c")
endif()

idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "include" REQUIRES bt PRIV_REQUIRES nvs_flash protocomm)

if(CONFIG_OMNITRIX_ENABLE_BLE)
  target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -fsanitize=undefined -fanalyzer)
endif()
