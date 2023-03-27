#pragma once

#include <hardware/gpio.h>
#include <pico/sync.h>

#include <array>
#include <bitset>
#include <cstdint>
#include <initializer_list>

struct RotaryEncoderState;

// Interrupt-based incremental rotary encoder reader. Accepting the pin numbers
// as template arguments rather than runtime parameters allows us to instantiate
// a different global interrupt handler function per encoder. The alternative
// would be run-time sharing and multiplexing of a single global interrupt
// handler.
//
// This technique template is inspired by
// https://github.com/tobiglaser/RP2040-Encoder/
template <unsigned pin_a, unsigned pin_b>
class RotaryEncoder {
 public:
  // Only one encoder instnace may be created per pin pair.
  RotaryEncoder();

  // Read the current signed cumulative dedent count.
  std::int64_t Read();

 private:
  // Global interrupt handler for this pin pair.
  static void EdgeInterrupt();

  // Global state for this pin pair.
  static RotaryEncoderState state_;
};

// Internal implementation details below.

// Global state per encoder.
struct RotaryEncoderState {
  // The GPIO pin numbers for the encoder.
  const std::array<unsigned, 2> pins;
  // The current encoder pins values. This assumes encoder switches shorting the
  // pins to ground on contact; i.e. 1 is disconnected, 0 is connected.
  std::bitset<2> values = 0b11;

  // Range [-3, +3] fractional detent state. When we reach ±4, we have advanced
  // to a new detent.
  int fractional_counter = 0;

  critical_section_t counter_critical_section;
  // Signed cumulative full dedents measured.
  std::int64_t counter = 0;

  // Setup pins and register the given interrupt handler for this pin pair.
  void Init(irq_handler_t edge_interrupt_handler);

  // Handle an edge transition on either signal.
  void EdgeInterrupt();

  // The current signed cumulative dedent count.
  std::int64_t Read();
};

template <unsigned pin_a, unsigned pin_b>
RotaryEncoderState RotaryEncoder<pin_a, pin_b>::state_{.pins = {pin_a, pin_b}};

template <unsigned pin_a, unsigned pin_b>
RotaryEncoder<pin_a, pin_b>::RotaryEncoder() {
  state_.Init(&RotaryEncoder<pin_a, pin_b>::EdgeInterrupt);
}

template <unsigned pin_a, unsigned pin_b>
std::int64_t RotaryEncoder<pin_a, pin_b>::Read() {
  return state_.Read();
}

template <unsigned pin_a, unsigned pin_b>
void RotaryEncoder<pin_a, pin_b>::EdgeInterrupt() {
  state_.EdgeInterrupt();
}
