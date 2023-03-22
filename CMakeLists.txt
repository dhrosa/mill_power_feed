cmake_minimum_required(VERSION 3.24)

set(PICO_BOARD arduino_nano_rp2040_connect)
set(PICO_SDK_FETCH_FROM_GIT on)
include(pico_sdk_import.cmake)

project(power_feed LANGUAGES C CXX)
pico_sdk_init()

add_executable(power_feed main.cc)
target_link_libraries(power_feed
  pico_stdlib
  pico_bootsel_via_double_reset
  hardware_pwm
)

pico_enable_stdio_usb(power_feed 1)
pico_enable_stdio_uart(power_feed 0)

pico_add_extra_outputs(power_feed)
