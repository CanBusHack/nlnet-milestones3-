cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(COMPONENTS main)

if("${IDF_TARGET}" STREQUAL linux)
  include_directories(linux/include)
  set(CMAKE_C_COMPILER gcc)
  set(EXTRA_COMPONENT_DIRS linux/components)
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(omnitrix)
