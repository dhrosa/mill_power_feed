#include "digital_input.h"

#include <cstdint>

namespace {
constexpr std::uint32_t kPinEventMask = GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL;
}  // namespace

void DigitalInput::State::Init(irq_handler_t edge_interrupt_handler) {
  gpio_init(pin);
  gpio_pull_up(pin);
  gpio_add_raw_irq_handler(pin, edge_interrupt_handler);
  gpio_set_irq_enabled(pin, kPinEventMask, true);
}

void DigitalInput::State::EdgeInterrupt() {
  const std::uint32_t events = gpio_get_irq_event_mask(pin);
  if (events & kPinEventMask == 0) {
    return;
  }
  value = events & GPIO_IRQ_EDGE_FALL;
  async_context_set_work_pending(async_context, &async_worker);
}
