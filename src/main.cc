#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <hardware/pwm.h>
#include <hardware/timer.h>
#include <pico/async_context_poll.h>
#include <pico/stdlib.h>

#include <array>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <span>
#include <string>
#include <tuple>
#include <vector>

#include "button.h"
#include "digital_input.h"
#include "font/font.h"
#include "oled.h"
#include "picopp/async.h"
#include "rotary_encoder.h"
#include "speed_control.h"

struct HandleSpeedEncoder {
  SpeedControl& speed_control;

  void operator()(std::int64_t encoder_value) {
    const std::int64_t speed = std::pow(2, double(encoder_value) / 8);
    speed_control.Set(speed);
  }
};

int main() {
  stdio_usb_init();
  irq_set_enabled(IO_IRQ_BANK0, true);
  sleep_ms(2000);
  std::cout << "startup" << std::endl;

  SpeedControl speed(133'000'000, 26, 27);

  async_context_poll_t poll_context;
  async_context_poll_init_with_defaults(&poll_context);

  async_context_t& context = poll_context.core;

  struct Input {
    std::int64_t encoder = 0;
    bool button = false;
  };
  std::array<Input, 3> inputs = {};

  auto render_worker = AsyncWorker::Create(context, [&]() {
    // const auto start_us = time_us_64();
    // const std::array<int, 2> positions[3] = {
    //     {0, 0},
    //     {64, 0},
    //     {0, 16},
    // };
    // for (int i = 0; i < 3; ++i) {
    //   const auto text = std::to_string(inputs[i].encoder);
    //   const auto [x, y] = positions[i];
    //   pico_ssd1306::drawText(&display, font_12x16, text.c_str(), x, y);
    //   if (inputs[i].button) {
    //     // Highlight the encoder value if the corresponding button is
    //     pressed. pico_ssd1306::fillRect(&display, x, y, x + 64, y + 16,
    //                            pico_ssd1306::WriteMode::INVERT);
    //   }
    // }
    // display.sendBuffer();
  });
  render_worker.SetWorkPending();

  auto encoder_handler = [&](std::size_t index) {
    return [&, index](std::int64_t new_value) {
      inputs[index].encoder = new_value;
      render_worker.SetWorkPending();
    };
  };

  auto button_handler = [&](std::size_t index) {
    return [&, index](bool pressed) {
      inputs[index].button = pressed;
      render_worker.SetWorkPending();
    };
  };

  // // RotaryEncoder::Create<19, 18>(context, encoder_handler(0));
  // RotaryEncoder::Create<19, 18>(context, HandleSpeedEncoder{speed});
  // RotaryEncoder::Create<5, 21>(context, encoder_handler(1));
  // RotaryEncoder::Create<16, 15>(context, encoder_handler(2));

  // Button::Create<20>(context, button_handler(0));
  // Button::Create<17>(context, button_handler(1));
  // Button::Create<25>(context, button_handler(2));

  auto background_worker =
      AsyncScheduledWorker::Create(context, []() -> absolute_time_t {
        std::cout << "heartbeat @" << (time_us_64() / 1000) << "ms"
                  << std::endl;
        return make_timeout_time_ms(1000);
      });
  background_worker.ScheduleAt(get_absolute_time());

  Oled oled(spi0, {.clock = 18, .data = 19, .reset = 25, .dc = 24, .cs = 29});
  auto& buffer = oled.Buffer();
  for (int i = 0; i < buffer.Height(); ++i) {
    buffer(i, i) = 1;
  }
  oled.Update();

  while (true) {
    async_context_wait_for_work_until(&context, at_the_end_of_time);
    async_context_poll(&context);
  }
  return 0;
}
