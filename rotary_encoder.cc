#include "rotary_encoder.h"

#include <hardware/gpio.h>
#include <hardware/timer.h>
#include <pico/sync.h>

#include <atomic>
#include <initializer_list>

namespace {
// RAII wrapper around critical_section_t
class CriticalSectionLock {
 public:
  CriticalSectionLock(critical_section_t& section) : section_(section) {
    critical_section_enter_blocking(&section_);
  }

  ~CriticalSectionLock() { critical_section_exit(&section_); }

  CriticalSectionLock(const CriticalSectionLock&) = delete;

 private:
  critical_section_t& section_;
};

unsigned clock_pin;
unsigned direction_pin;

// 100ms debounce time.
constexpr std::int64_t kDebouncePeriodUs = 100'000;

std::int64_t last_clock_edge_time_us = 0;
std::int64_t counter = 0;
critical_section_t counter_critical_section;

void ClockInterrupt() {
  const unsigned events = gpio_get_irq_event_mask(clock_pin);
  if (events & GPIO_IRQ_EDGE_FALL == 0) {
    return;
  }
  gpio_acknowledge_irq(clock_pin, GPIO_IRQ_EDGE_FALL);
  const std::int64_t current_time_us = time_us_64();
  if (current_time_us - last_clock_edge_time_us < kDebouncePeriodUs) {
    return;
  }
  last_clock_edge_time_us = current_time_us;
  const uint direction = gpio_get(direction_pin);
  CriticalSectionLock lock(counter_critical_section);
  if (direction) {
    ++counter;
  } else {
    --counter;
  }
}

}  // namespace

RotaryEncoder::RotaryEncoder(unsigned pin_a, unsigned pin_b) {
  critical_section_init(&counter_critical_section);
  clock_pin = pin_a;
  direction_pin = pin_b;

  for (unsigned pin : {clock_pin, direction_pin}) {
    gpio_init(pin);
    gpio_pull_up(pin);
  }

  irq_set_enabled(IO_IRQ_BANK0, true);
  gpio_add_raw_irq_handler(clock_pin, ClockInterrupt);
  gpio_set_irq_enabled(clock_pin, GPIO_IRQ_EDGE_FALL, true);
}

std::int64_t RotaryEncoder::Read() {
  CriticalSectionLock lock(counter_critical_section);
  return counter;
}
