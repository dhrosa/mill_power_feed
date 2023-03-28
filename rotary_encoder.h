#pragma once

#include <hardware/gpio.h>
#include <pico/sync.h>

#include <array>
#include <bitset>
#include <cstdint>
#include <initializer_list>

// Interrupt-based incremental rotary encoder reader. Accepting the pin numbers
// as template arguments rather than runtime parameters allows us to instantiate
// a different global interrupt handler function per encoder. The alternative
// would be run-time sharing and multiplexing of a single global interrupt
// handler.
//
// Trivially copyable and moveable. Copied/moved values will refer to the same
// internal state.
//
// This technique template is inspired by
// https://github.com/tobiglaser/RP2040-Encoder/
class RotaryEncoder {
 public:
  // Must only be called once per pin pair. Creates the encoder instance and
  // sets up interrupt handlers for it.
  template <unsigned pin_a, unsigned pin_b>
  static RotaryEncoder Create();

  // The current signed cumulative dedent count.
  std::int64_t Read();

 private:
  struct State;
  State* state_;

  template <std::array<unsigned, 2> pins>
  struct Singleton;
};

// Internal implementation details below.

// Global state per encoder.
struct RotaryEncoder::State {
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

// Per-pin-pair singleton.
template <std::array<unsigned, 2> pins>
struct RotaryEncoder::Singleton {
  inline static RotaryEncoder::State state{.pins = pins};
  static void EdgeInterrupt() { state.EdgeInterrupt(); }
};

template <unsigned pin_a, unsigned pin_b>
RotaryEncoder RotaryEncoder::Create() {
  constexpr std::array<unsigned, 2> pins = {pin_a, pin_b};
  RotaryEncoder::State& state = Singleton<pins>::state;
  state.Init(&Singleton<pins>::EdgeInterrupt);

  RotaryEncoder encoder;
  encoder.state_ = &state;
  return encoder;
}
