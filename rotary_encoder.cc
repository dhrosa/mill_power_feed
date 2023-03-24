#include "rotary_encoder.h"

#include <hardware/gpio.h>
#include <hardware/timer.h>
#include <pico/sync.h>

#include <array>
#include <bitset>
#include <initializer_list>
#include <iostream>

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

// The GPIO pin numbers for the encoder.
std::array<unsigned, 2> pins;
// The current encoder pins values.
std::bitset<2> values = 0b11;

// Range [-3, +3] fractional detent state. When we reach ±4, we have advanced to
// a new detent.
int fractional_counter = 0;

critical_section_t counter_critical_section;
// Signed cumulative full dedents measured.
std::int64_t counter = 0;

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

constexpr std::uint32_t kPinChangeEvent =
    GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL;

// Triggered when either pin changes state.
void EdgeInterrupt() {
  const std::bitset<2> previous_values = values;
  const std::bitset<32> all_gpio_values = gpio_get_all();
  for (int i : {0, 1}) {
    gpio_acknowledge_irq(pins[i], kPinChangeEvent);
    values[i] = all_gpio_values[pins[i]];
  }
  const unsigned transition =
      (previous_values.to_ulong() << 2) | values.to_ulong();
  fractional_counter += kIncrementsTable[transition];
  // Pulses per full detent.
  const int divisor = 4;
  if (std::abs(fractional_counter) == divisor) {
    // Full detent completed.
    CriticalSectionLock lock(counter_critical_section);
    counter += fractional_counter / divisor;
    fractional_counter = 0;
  }
}

}  // namespace

RotaryEncoder::RotaryEncoder(unsigned pin_a, unsigned pin_b) {
  pins = {pin_a, pin_b};
  critical_section_init(&counter_critical_section);

  for (unsigned pin : pins) {
    gpio_init(pin);
    gpio_pull_up(pin);
    gpio_add_raw_irq_handler(pin, EdgeInterrupt);
    gpio_set_irq_enabled(pin, kPinChangeEvent, true);
  }
  irq_set_enabled(IO_IRQ_BANK0, true);
}

std::int64_t RotaryEncoder::Read() {
  CriticalSectionLock lock(counter_critical_section);
  return counter;
}
