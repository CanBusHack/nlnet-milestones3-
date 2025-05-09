idf_component_register(
  SRCS
  "main.c"
  "ble.c"
  "hello.c"
  "isotp.c"
  "j2534.c"
  "j2534.pb-c.c"
  "libcan.c"
  "libisotp.c"
  "libnvs.c"
  "libvin.c"
  "ota.c"
  "debug.c"
  "led.c"
  "led_ble.c"
  "heartbeat.c"
  "${CMAKE_CURRENT_BINARY_DIR}/omnitrix/uuid.gen.h"
  PRIV_INCLUDE_DIRS
  "include"
  "j2534/include"
  "${CMAKE_CURRENT_BINARY_DIR}"
  PRIV_REQUIRES
  app_update
  bootloader_support
  bt
  driver
  esp_partition
  nvs_flash
  protocomm
  protobuf-c
  esp_common
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -fsanitize=undefined -fanalyzer)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/omnitrix/uuid.gen.h
  COMMAND ${python} ${PROJECT_DIR}/generate-uuids.py ${BUILD_DIR}/config/sdkconfig.json ${CMAKE_CURRENT_BINARY_DIR}/omnitrix/uuid.gen.h
  DEPENDS ${PROJECT_DIR}/generate-uuids.py ${BUILD_DIR}/config/sdkconfig.h
)
