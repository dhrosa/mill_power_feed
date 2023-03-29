#include "digital_input.h"

#include <cstdint>

void DigitalInput::State::Init(irq_handler_t edge_interrupt_handler) {
  async_context_add_when_pending_worker(async_context, &async_worker);
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
  async_context_set_work_pending(async_context, &async_worker);
}
