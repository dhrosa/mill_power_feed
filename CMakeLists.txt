cmake_minimum_required(VERSION 3.24)
set(CMAKE_CXX_STANDARD 20)

set(PICO_BOARD arduino_nano_rp2040_connect)
set(PICO_SDK_FETCH_FROM_GIT on)

include(pico_sdk_import.cmake)

project(power_feed LANGUAGES C CXX)
pico_sdk_init()

include(FetchContent)
FetchContent_Declare(
  pico_oled
  GIT_REPOSITORY https://github.com/Harbys/pico-ssd1306.git
  GIT_TAG master
)

FetchContent_MakeAvailable(pico_oled)

add_executable(power_feed main.cc rotary_encoder.cc)
# We run over the default setting of 4.
target_compile_definitions(power_feed PRIVATE PICO_MAX_SHARED_IRQ_HANDLERS=16)
target_include_directories(power_feed PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(power_feed
  pico_stdlib
  pico_bootsel_via_double_reset
  hardware_pwm
  pico_ssd1306
  hardware_i2c
)

pico_enable_stdio_usb(power_feed 1)
pico_enable_stdio_uart(power_feed 0)

pico_add_extra_outputs(power_feed)
