#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <hardware/pwm.h>
#include <hardware/timer.h>
#include <pico/async_context_poll.h>
#include <pico/stdlib.h>

#include <array>
#include <cstdio>
#include <iostream>
#include <string>
#include <tuple>

#include "digital_input.h"
#include "picopp/async.h"
#include "rotary_encoder.h"
#include "ssd1306.h"
#include "textRenderer/TextRenderer.h"

void DrawCenteredValue(pico_ssd1306::SSD1306& display, int value) {
  const unsigned char* font = font_16x32;
  constexpr int line_width = 128;
  constexpr int char_width = 16;
  constexpr int chars_per_line = line_width / char_width;

  std::array<char, chars_per_line + 1> buffer;
  const int chars_written =
      std::snprintf(buffer.data(), buffer.size(), "%+d", value);
  const int left_padding = (chars_per_line - chars_written) / 2;

  display.clear();
  pico_ssd1306::drawText(&display, font, buffer.data(),
                         left_padding * char_width, 0);
  display.sendBuffer();
}

pico_ssd1306::SSD1306 CreateDisplay() {
  i2c_init(i2c0, 1'000'000);
  for (uint pin : {PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN}) {
    gpio_set_function(pin, GPIO_FUNC_I2C);
    gpio_pull_up(pin);
  }
  pico_ssd1306::SSD1306 display(i2c0, 0x3C, pico_ssd1306::Size::W128xH32);
  display.clear();
  display.sendBuffer();
  return display;
}

int main() {
  stdio_usb_init();
  irq_set_enabled(IO_IRQ_BANK0, true);
  sleep_ms(2000);
  std::cout << "startup" << std::endl;

  pico_ssd1306::SSD1306 display = CreateDisplay();

  async_context_poll_t poll_context;
  async_context_poll_init_with_defaults(&poll_context);

  async_context_t& context = poll_context.core;

  std::array<std::int64_t, 3> encoder_values = {};
  auto render_worker = AsyncWorker::Create(context, [&]() {
    const auto start_us = time_us_64();
    display.clear();
    const unsigned char* font = font_12x16;
    const std::array<int, 2> positions[3] = {
        {0, 0},
        {64, 0},
        {0, 16},
    };
    for (int i = 0; i < 3; ++i) {
      const auto text = std::to_string(encoder_values[i]);
      auto [x, y] = positions[i];
      pico_ssd1306::drawText(&display, font, text.c_str(), x, y);
    }
    display.sendBuffer();
  });
  render_worker.SetWorkPending();

  auto encoder_handler = [&](std::size_t index) {
    return [&, index](std::int64_t new_value) {
      encoder_values[index] = new_value;
      render_worker.SetWorkPending();
    };
  };

  RotaryEncoder::Create<5, 21>(context, encoder_handler(0));
  RotaryEncoder::Create<19, 18>(context, encoder_handler(1));
  RotaryEncoder::Create<16, 15>(context, encoder_handler(2));

  auto background_worker =
      AsyncScheduledWorker::Create(context, []() -> absolute_time_t {
        std::cout << "heartbeat @" << (time_us_64() / 1000) << "ms"
                  << std::endl;
        return make_timeout_time_ms(1000);
      });
  background_worker.ScheduleAt(get_absolute_time());

  while (true) {
    async_context_wait_for_work_until(&context, at_the_end_of_time);
    async_context_poll(&context);
  }
  return 0;
}
