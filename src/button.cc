#include "button.h"

#include <hardware/gpio.h>

#include <cstdint>

namespace {
constexpr std::uint32_t kPinEventMask = GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL;
};

void Button::State::Init(irq_handler_t edge_interrupt_handler) {
  gpio_init(pin);
  gpio_pull_up(pin);
  gpio_add_raw_irq_handler(pin, edge_interrupt_handler);
  gpio_set_irq_enabled(pin, kPinEventMask, true);
}

void Button::State::HandleInterrupt() {
  auto events = gpio_get_irq_event_mask(pin);
  if ((events & kPinEventMask) == 0) {
    return;
  }
  gpio_acknowledge_irq(pin, kPinEventMask);
  const bool value = gpio_get(pin);
  waiter->Send(value);
}
