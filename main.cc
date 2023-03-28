#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <hardware/pwm.h>
#include <hardware/timer.h>
#include <pico/async_context.h>
#include <pico/async_context_poll.h>
#include <pico/stdlib.h>

#include <array>
#include <cstdio>
#include <iostream>
#include <string>
#include <tuple>

#include "digital_input.h"
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

std::uint64_t time_ms() { return time_us_64() / 1000; }

void AsyncShenanigans() {
  async_context_poll_t poll_context;
  async_context_poll_init_with_defaults(&poll_context);

  async_context_t* context = &poll_context.core;

  DigitalInput::Create<15>(*context, [](bool value) {
    std::cout << "new digital input: " << int(value) << "@" << time_ms()
              << std::endl;
  });

  async_at_time_worker_t worker;
  worker.do_work = +[](async_context_t* context,
                       async_at_time_worker_t* worker) {
    std::cout << "scheduled async worker triggered @" << time_ms() << std::endl;
    async_context_add_at_time_worker_in_ms(context, worker, 1000);
  };
  worker.do_work(context, &worker);

  while (true) {
    async_context_wait_for_work_until(context, at_the_end_of_time);
    async_context_poll(context);
  }
}

int main() {
  stdio_usb_init();
  irq_set_enabled(IO_IRQ_BANK0, true);
  sleep_ms(2000);
  std::cout << "startup" << std::endl;
  AsyncShenanigans();

  std::array<RotaryEncoder, 3> encoders = {
      RotaryEncoder::Create<16, 15>(),
      RotaryEncoder::Create<5, 21>(),
      RotaryEncoder::Create<19, 18>(),
  };

  irq_set_enabled(IO_IRQ_BANK0, true);

  i2c_init(i2c0, 1'000'000);
  for (uint pin : {PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN}) {
    gpio_set_function(pin, GPIO_FUNC_I2C);
    gpio_pull_up(pin);
  }
  pico_ssd1306::SSD1306 display(i2c0, 0x3C, pico_ssd1306::Size::W128xH32);
  display.clear();
  display.sendBuffer();

  DrawCenteredValue(display, 1234);

  while (true) {
    display.clear();
    const unsigned char* font = font_12x16;
    const std::array<int, 2> positions[3] = {{0, 0}, {64, 0}, {0, 16}};
    for (int i = 0; i < 3; ++i) {
      std::array<char, 16 + 1> buffer;
      std::snprintf(buffer.data(), buffer.size(), "%+4lld", encoders[i].Read());
      auto [x, y] = positions[i];
      pico_ssd1306::drawText(&display, font, buffer.data(), x, y);
    }
    display.sendBuffer();
    sleep_ms(25);
  }
  return 0;
}
