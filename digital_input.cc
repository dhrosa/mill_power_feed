#include "digital_input.h"

#include <hardware/timer.h>

void DigitalInput::State::Init(irq_handler_t edge_interrupt_handler) {
  gpio_init(pin);
  gpio_pull_up(pin);
  gpio_add_raw_irq_handler(pin, edge_interrupt_handler);
  gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_FALL, true);
}

void DigitalInput::State::FallInterrupt() {
  const std::uint32_t events = gpio_get_irq_event_mask(pin);
  if ((events & GPIO_IRQ_EDGE_FALL) == 0) {
    return;
  }
  gpio_acknowledge_irq(pin, events);
  // Debounce fall events within 100ms of eachother.
  const std::uint64_t time_us = time_us_64();
  if (time_us - last_event_time_us < 100'000) {
    return;
  }
  last_event_time_us = time_us;
  async_worker.SetWorkPending();
}
