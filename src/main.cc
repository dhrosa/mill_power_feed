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
#include "picoro/async.h"
#include "picoro/task.h"
#include "rotary_encoder.h"
#include "speed_control.h"

int main() {
  stdio_usb_init();
  irq_set_enabled(IO_IRQ_BANK0, true);

  Oled oled(spi0, {.clock = 2, .data = 3, .reset = 4, .dc = 5, .cs = 6});
  auto& buffer = oled.Buffer();

  for (int i = 3; i >= 0; --i) {
    const Font& font = FontForHeight(64);
    std::cout << "Starting in " << i << " seconds" << std::endl;
    buffer.Clear();
    buffer.DrawString(font, std::to_string(i),
                      (buffer.Width() - font.width) / 2, 0);
    oled.Update();
    sleep_ms(1000);
  }
  std::cout << "Startup" << std::endl;

  buffer.Clear();
  oled.Update();

  async_context_poll_t poll_context;
  async_context_poll_init_with_defaults(&poll_context);

  async_context_t& context = poll_context.core;

  struct Input {
    std::int64_t encoder = 0;
    bool button = false;
  };
  std::array<Input, 3> inputs = {};

  const Font& font = FontForHeight(32);
  auto render_worker = AsyncWorker::Create(context, [&]() {
    buffer.Clear();
    const std::array<std::size_t, 2> positions[3] = {
        {0, 0},
        {buffer.Width() / 2, 0},
        {0, buffer.Height() / 2},
    };
    for (int i = 0; i < 3; ++i) {
      const auto text = std::to_string(inputs[i].encoder);
      const auto [x, y] = positions[i];
      buffer.DrawString(font, text, x, y);
    }
    oled.Update();
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

  // RotaryEncoder::Create<22, 26>(context, encoder_handler(0));
  // RotaryEncoder::Create<19, 20>(context, encoder_handler(1));
  // RotaryEncoder::Create<17, 16>(context, encoder_handler(2));
  RotaryEncoder encoders[] = {
      RotaryEncoder::Create<22, 26>(context),
      RotaryEncoder::Create<19, 20>(context),
      RotaryEncoder::Create<17, 16>(context),
  };

  auto encoder_task = [](auto& encoders, auto& inputs, auto& render_worker,
                         std::size_t index) -> Task {
    while (true) {
      const std::int64_t value = co_await encoders[index];
      std::cout << value << std::endl;
      inputs[index].encoder = value;
      render_worker.SetWorkPending();
    }
    co_return;
  };

  auto task = encoder_task(encoders, inputs, render_worker, 0);

  Button::Create<27>(context, button_handler(0));
  Button::Create<21>(context, button_handler(1));
  Button::Create<18>(context, button_handler(2));

  auto background_worker =
      AsyncScheduledWorker::Create(context, []() -> absolute_time_t {
        std::cout << "heartbeat @" << (time_us_64() / 1000) << "ms"
                  << std::endl;
        return make_timeout_time_ms(10'000);
      });
  background_worker.ScheduleAt(get_absolute_time());

  while (true) {
    async_context_wait_for_work_until(&context, at_the_end_of_time);
    async_context_poll(&context);
  }
  return 0;
}
