add_subdirectory(font)

add_executable(power_feed main.cc rotary_encoder.cc digital_input.cc
                          speed_control.cc oled.cc oled_buffer.cc)
# We run over the default setting of 4.
target_compile_definitions(power_feed PRIVATE PICO_MAX_SHARED_IRQ_HANDLERS=16)
target_include_directories(power_feed PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_compile_options(power_feed PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fcoroutines>)

target_link_libraries(
  power_feed
  font
  pico_stdlib
  pico_async_context_poll
  pico_bootsel_via_double_reset
  hardware_pwm
  hardware_spi
  hardware_i2c)

pico_enable_stdio_usb(power_feed 1)
pico_enable_stdio_uart(power_feed 0)

pico_add_extra_outputs(power_feed)
