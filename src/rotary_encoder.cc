#include "rotary_encoder.h"

#include "picopp/critical_section.h"

namespace {
// Mapping of `abAB` values to fractional counter increments, where `ab` are the
// bits representing the previous readings of the encoder pins, and `AB` are the
// bits representing the new readings.
//
// The encoder should transition with the following cycle:
// 11 <=> 10 <=> 00 <=> 01 <=> 11
constexpr std::array<int, 16> IncrementsTable() {
  std::array<int, 16> inc = {0};

  inc[0b11'01] = +1;
  inc[0b11'10] = -1;

  inc[0b01'00] = +1;
  inc[0b01'11] = -1;

  inc[0b00'10] = +1;
  inc[0b00'01] = -1;

  inc[0b10'11] = +1;
  inc[0b10'00] = -1;
  return inc;
}

constexpr auto kIncrementsTable = IncrementsTable();

constexpr std::uint32_t kPinEventMask = GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL;

}  // namespace

void RotaryEncoder::State::Init(irq_handler_t edge_interrupt_handler) {
  critical_section_init(&counter_critical_section);
  for (unsigned pin : pins) {
    gpio_init(pin);
    gpio_pull_up(pin);
    gpio_add_raw_irq_handler(pin, edge_interrupt_handler);
    gpio_set_irq_enabled(pin, kPinEventMask, true);
  }
}

void RotaryEncoder::State::HandleInterrupt() {
  const std::bitset<2> previous_values = values;
  const std::bitset<32> all_gpio_values = gpio_get_all();
  for (int i : {0, 1}) {
    gpio_acknowledge_irq(pins[i], kPinEventMask);
    values[i] = all_gpio_values[pins[i]];
  }
  const unsigned transition =
      (previous_values.to_ulong() << 2) | values.to_ulong();
  fractional_counter += kIncrementsTable[transition];
  // Pulses per full detent.
  const int divisor = 4;
  if (std::abs(fractional_counter) == divisor) {
    {
      // Full detent completed.
      CriticalSectionLock lock(counter_critical_section);
      counter += fractional_counter / divisor;
      fractional_counter = 0;
    }
    async_worker.SetWorkPending();
  }
}

std::int64_t RotaryEncoder::State::Read() {
  CriticalSectionLock lock(counter_critical_section);
  return counter;
}
