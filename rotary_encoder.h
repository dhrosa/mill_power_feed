#pragma once

#include <hardware/gpio.h>
#include <pico/sync.h>

#include <array>
#include <bitset>
#include <cstdint>
#include <initializer_list>
#include <utility>

#include "picopp/async.h"
#include "picopp/irq.h"

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
  template <unsigned pin_a, unsigned pin_b, typename F>
  static void Create(async_context_t& context, F&& handler);

 private:
  struct State;
  State* state_;
};

// Internal implementation details below.

// Global state per encoder.
struct RotaryEncoder::State {
  // The GPIO pin numbers for the encoder.
  std::array<unsigned, 2> pins;

  AsyncWorker async_worker;

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
  void HandleInterrupt();

  // The current signed cumulative dedent count.
  std::int64_t Read();
};

template <unsigned pin_a, unsigned pin_b, typename F>
void RotaryEncoder::Create(async_context_t& context, F&& handler) {
  // Unique tag type for each pin pair.
  using Tag = std::integer_sequence<unsigned, pin_a, pin_b>;
  using Singleton = InterruptHandlerSingleton<Tag, State>;

  State& state = Singleton::state;
  state.pins = {pin_a, pin_b};
  state.async_worker = AsyncWorker::Create(
      context, [handler]() { handler(Singleton::state.Read()); });
  state.Init(Singleton::interrupt_handler);
}
