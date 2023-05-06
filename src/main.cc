#include <hardware/i2c.h>
#include <hardware/timer.h>
#include <hardware/watchdog.h>
#include <pico/async_context_poll.h>
#include <pico/stdlib.h>

#include <cmath>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <span>
#include <sstream>
#include <string>
#include <vector>

#include "button.h"
#include "digital_input.h"
#include "font/font.h"
#include "oled.h"
#include "picopp/gpio.h"
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
  Gpio left_button;
  Gpio right_button;
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
        left_button(
            Gpio(13, {.direction = Gpio::kInput, .polarity = Gpio::kNegative})),
        right_button(
            Gpio(14, {.direction = Gpio::kInput, .polarity = Gpio::kNegative})),
        speed_control(133'000'000, 1, 0),
        update_event(context) {
    const double initial_ipm = 1;
    level =
        std::round(fine_steps_per_octave * std::log2(ppi * initial_ipm / 60));

    Startup();
    CreateTasks();
  }

  void Startup() {
    for (int i = 2; i >= 0; --i) {
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
    add(EncoderTask(encoders[2], coarse_multiplier));
    add(DirectionTask());
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
  static constexpr std::int64_t coarse_multiplier = 8;

  Task EncoderTask(RotaryEncoder& encoder, std::int64_t multiplier) {
    std::int64_t previous = 0;
    while (true) {
      const std::int64_t current = co_await encoder;
      const std::int64_t delta = current - previous;
      previous = current;
      level += delta * multiplier;
      update_event.Notify();
    }
  }

  Task DirectionTask() {
    AsyncExecutor executor(context);
    while (true) {
      int new_direction = 0;
      if (left_button) {
        new_direction = -1;
      } else if (right_button) {
        new_direction = 1;
      }
      if (new_direction != direction) {
        direction = new_direction;
        update_event.Notify();
      }
      co_await executor.SleepUntil(make_timeout_time_ms(50));
    }
  }

  Task UpdateTask() {
    const Font& value_font = FontForHeight(24);
    const Font& unit_font = FontForHeight(8);
    auto draw_speed = [&](double value, std::string_view unit, int y) {
      std::ostringstream ss;
      // Always shows 5 characters including the decimal marker.
      if (value > 1000) {
        ss << int(value) << ".";
      } else if (value > 1) {
        ss << std::setprecision(4) << value;
      } else {
        ss << std::setprecision(3) << value;
      }
      const std::string_view value_str = ss.view();

      // Center the speed text, which is 6.5 characters wide: 5 from the value
      // and 1.5 from the units.
      int x = (buffer.Width() - 13 * value_font.width / 2) / 2;
      buffer.DrawString(value_font, value_str, x, y);
      x += value_str.size() * 12;
      // unit-per-minute fraction drawn so that it takes up 1 digit height
      // vertically, 2 digit widths horizontally, and lining up with the top
      // and bottom edges of the digit text.
      buffer.DrawString(unit_font, unit, x + 7, y + 3);
      buffer.DrawLineH(y + 11, x + 4, x + 20);
      buffer.DrawString(unit_font, "min", x + 5, y + 12);
    };
    const Font& arrow_font = FontForHeight(32);
    auto draw_arrow = [&] {
      if (direction == 0) {
        return;
      }
      int x;
      const int y = value_font.height - (arrow_font.height / 2);
      char arrow_char;
      if (direction == -1) {
        arrow_char = '<';
        x = 0;
      } else {
        arrow_char = '>';
        x = buffer.Width() - arrow_font.width;
      }
      buffer.DrawChar(arrow_font, arrow_char, x, y);
    };
    auto draw_labels = [&] {
      const Font& label_font = FontForHeight(8);
      buffer.DrawString(label_font, "Fine", 0,
                        buffer.Height() - label_font.height);
      buffer.DrawString(label_font, "Coarse",
                        buffer.Width() - 6 * label_font.width,
                        buffer.Height() - label_font.height);
    };
    while (true) {
      std::cout << "Level: " << level << " frequency: " << frequency()
                << " IPM: " << ipm() << " direction: " << direction
                << std::endl;
      speed_control.Set(direction * frequency());
      // Update display.
      buffer.Clear();
      draw_speed(ipm(), "in", 0);
      draw_speed(25.4 * ipm(), "mm", 24);
      draw_arrow();
      draw_labels();
      oled.Update();

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

  if (watchdog_enable_caused_reboot()) {
    std::cout << "Last reboot triggered by watchdog." << std::endl;
  }
  const bool pause_on_debug = false;
  const std::uint32_t watchdog_timeout_ms = 5000;
  watchdog_enable(watchdog_timeout_ms, pause_on_debug);

  while (true) {
    async_context_wait_for_work_until(&context, at_the_end_of_time);
    async_context_poll(&context);
    watchdog_update();
  }
  return 0;
}
