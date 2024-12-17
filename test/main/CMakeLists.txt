idf_component_register(
  SRCS
  "test.c"
  "ble/hello/handle.c"
  "ble/hello/uuid.c"
  "can/isotp/read.c"
  "can/isotp/write-multi.c"
  "can/isotp/write-single.c"
  "can/raw/read.c"
  "can/raw/write.c"
  INCLUDE_DIRS
  "."
  REQUIRES
  bt
  driver
  nvs_flash
  protocomm
  unity
  WHOLE_ARCHIVE
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -fsanitize=undefined -fanalyzer)
