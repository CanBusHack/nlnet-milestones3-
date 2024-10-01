idf_component_register(
  SRCS
  "main.c"
  "ble.c"
  "hello.c"
  "isotp.c"
  "j2534.c"
  "j2534.pb-c.c"
  "libcan.c"
  "libnvs.c"
  "libvin.c"
  "ota.c"
  "${CMAKE_CURRENT_BINARY_DIR}/include/omnitrix/uuid.gen.h"
  PRIV_INCLUDE_DIRS
  "include"
  "${CMAKE_CURRENT_BINARY_DIR}/include"
  PRIV_REQUIRES
  app_update
  bootloader_support
  bt
  driver
  esp_partition
  nvs_flash
  protocomm
  protobuf-c
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -fsanitize=undefined -fanalyzer)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include/omnitrix/uuid.gen.h
  COMMAND ${python} ${PROJECT_DIR}/generate-uuids.py ${BUILD_DIR}/config/sdkconfig.json ${CMAKE_CURRENT_BINARY_DIR}/include/omnitrix/uuid.gen.h
  DEPENDS ${PROJECT_DIR}/generate-uuids.py ${BUILD_DIR}/config/sdkconfig.h
)
