cmake_minimum_required(VERSION 3.24)
set(CMAKE_CXX_STANDARD 20)

set(PICO_BOARD adafruit_feather_rp2040)
set(PICO_SDK_FETCH_FROM_GIT on)

include(pico_sdk_import.cmake)

project(power_feed LANGUAGES C CXX)
pico_sdk_init()

include(FetchContent)
FetchContent_Declare(
  spleen-font
  GIT_REPOSITORY https://github.com/fcambus/spleen
)
FetchContent_MakeAvailable(spleen-font)
add_subdirectory(src)
