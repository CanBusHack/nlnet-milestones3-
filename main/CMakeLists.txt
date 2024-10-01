idf_component_register(
  SRCS
  "main.c"
  "ble.c"
  "hello.c"
  "isotp.c"
  "libnvs.c"
  "libvin.c"
  "ota.c"
  PRIV_INCLUDE_DIRS
  "include"
  PRIV_REQUIRES
  app_update
  bootloader_support
  bt
  driver
  esp_partition
  nvs_flash
  protocomm
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -fsanitize=undefined -fanalyzer)
