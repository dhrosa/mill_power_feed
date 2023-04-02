#pragma once

#include <pico/time.h>
#include <hardware/gpio.h>

#include <cstdint>

#include "picopp/async.h"

class Button {
 public:
  template <unsigned pin, typename F>
  static void Create(async_context_t& context, F&& callback);
};

template <unsigned pin, typename F>
void Button::Create(async_context_t& context, F&& callback) {
  gpio_init(pin);
  gpio_pull_up(pin);
  auto do_work = [callback, last_value = gpio_get(pin)]() mutable {
    const bool current_value = gpio_get(pin);
    if (current_value != last_value) {
      last_value = current_value;
      callback(!current_value);
    }
    return make_timeout_time_ms(100);
  };
  auto* worker = new AsyncScheduledWorker(
      AsyncScheduledWorker::Create(context, std::move(do_work)));
  worker->ScheduleAt(get_absolute_time());
}
