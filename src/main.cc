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
#include "picoro/async.h"
#include "picoro/event.h"
#include "picoro/task.h"
#include "rotary_encoder.h"
#include "speed_control.h"

struct Controller {
  async_context_t& context;
  Oled oled;
  OledBuffer& buffer;
  RotaryEncoder encoders[3];
  Button buttons[3];
  Button left_button;
  Button right_button;
  SpeedControl speed_control;
  Event update_event;

  Controller(async_context_t& context)
      : context(context),
        oled(spi0, {.clock = 2, .data = 3, .reset = 4, .dc = 5, .cs = 6}),
        buffer(oled.Buffer()),
        encoders{
            RotaryEncoder::Create<22, 26>(context),
            RotaryEncoder::Create<19, 20>(context),
            RotaryEncoder::Create<17, 16>(context),
        },
        buttons{
            Button::Create<27>(context),
            Button::Create<21>(context),
            Button::Create<18>(context),
        },
        left_button(Button::Create<13>(context)),
        right_button(Button::Create<14>(context)),
        speed_control(133'000'000, 1, 0),
        update_event(context) {
    const double initial_ipm = 1;
    level = fine_steps_per_octave * std::log2(ppi * initial_ipm / 60);

    Startup();
    CreateTasks();
  }

  void Startup() {
    for (int i = 3; i >= 0; --i) {
      const Font& font = FontForHeight(64);
      std::cout << "Starting in " << i << " seconds" << std::endl;
      buffer.Clear();
      buffer.DrawString(font, std::to_string(i),
                        (buffer.Width() - font.width) / 2, 0);
      oled.Update();
      sleep_ms(1000);
    }
    buffer.Clear();
    oled.Update();
    std::cout << "Startup" << std::endl;
  }

  std::vector<Task> tasks;

  void CreateTasks() {
    const auto add = [&](Task task) { tasks.push_back(std::move(task)); };
    add(BackgroundTask());
    add(EncoderTask(encoders[0], 1));
    add(EncoderTask(encoders[1], coarse_multiplier));
    add(DirectionButtonTask(left_button, -1));
    add(DirectionButtonTask(right_button, +1));
    add(UpdateTask());
  }

  Task BackgroundTask() {
    AsyncExecutor executor(context);
    while (true) {
      std::cout << "heartbeat @" << (time_us_64() / 1000) << "ms" << std::endl;
      co_await executor.SleepUntil(make_timeout_time_ms(3'000));
    }
  }

  std::int64_t level = 0;
  int direction = 0;
  const std::int64_t ppr = 8000;
  const std::int64_t tpi = 20;
  const std::int64_t ppi = ppr * tpi;
  static constexpr std::int64_t fine_steps_per_octave = 160;
  static constexpr std::int64_t coarse_multiplier = 4;

  Task EncoderTask(RotaryEncoder& encoder, std::int64_t multiplier) {
    std::int64_t previous = 0;
    while (true) {
      const std::int64_t current = co_await encoder;
      const std::int64_t delta = current - previous;
      level += delta * multiplier;
      update_event.Notify();
    }
  }

  Task DirectionButtonTask(Button& button, int button_direction) {
    while (true) {
      const bool pressed = co_await button;
      if (pressed) {
        direction = button_direction;
      } else if (direction == button_direction) {
        direction = 0;
      }
      update_event.Notify();
    }
  }

  Task UpdateTask() {
    while (true) {
      std::cout << "Level: " << level << " frequency: " << frequency()
                << " IPM: " << ipm() << " direction: " << direction
                << std::endl;
      co_await update_event;
    }
  }

  double frequency() const {
    return std::exp2(static_cast<double>(level) / fine_steps_per_octave);
  }

  double ipm(double frequency) const { return frequency * 60 / ppi; }
  double ipm() const { return frequency() * 60 / ppi; }
};

int main() {
  stdio_usb_init();
  irq_set_enabled(IO_IRQ_BANK0, true);

  async_context_poll_t poll_context;
  async_context_poll_init_with_defaults(&poll_context);

  async_context_t& context = poll_context.core;
  Controller controller(context);

  while (true) {
    async_context_wait_for_work_until(&context, at_the_end_of_time);
    async_context_poll(&context);
  }
  return 0;
}
