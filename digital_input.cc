#include "digital_input.h"

#include <cstdint>

#include "picopp/critical_section.h"

namespace {
constexpr std::uint32_t kPinEventMask = GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL;
}  // namespace

void DigitalInput::State::Init(irq_handler_t edge_interrupt_handler) {
  critical_section_init(&value_critical_section);
  async_context_add_when_pending_worker(async_context, &async_worker);
  gpio_init(pin);
  gpio_pull_up(pin);
  gpio_add_raw_irq_handler(pin, edge_interrupt_handler);
  gpio_set_irq_enabled(pin, kPinEventMask, true);
}

bool DigitalInput::State::Value() {
  CriticalSectionLock lock(value_critical_section);
  return value;
}

void DigitalInput::State::EdgeInterrupt() {
  const std::uint32_t events = gpio_get_irq_event_mask(pin);
  if ((events & kPinEventMask) == 0) {
    return;
  }
  gpio_acknowledge_irq(pin, events);
  value = events & GPIO_IRQ_EDGE_FALL;
  async_context_set_work_pending(async_context, &async_worker);
}
