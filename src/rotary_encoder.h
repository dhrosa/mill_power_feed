#pragma once

#include <hardware/gpio.h>
#include <pico/async_context.h>
#include <pico/sync.h>

#include <array>
#include <bitset>
#include <coroutine>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <utility>

#include "picopp/critical_section.h"
#include "picopp/irq.h"
#include "picoro/async.h"
#include "picoro/awaitable_reference.h"

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
  // Must only be called once per pin pair. Coroutines awaiting on this encoder
  // will resume execution on `context`. Not thread-safe.
  template <unsigned pin_a, unsigned pin_b>
  static RotaryEncoder Create(async_context_t& context);

  // Awaits an update to the rotary encoder dedent count (int64).
  auto operator co_await();

 private:
  class Waiter;
  struct State;

  RotaryEncoder(State* state) : state_(state) {}
  State* state_;
};

// Internal implementation details below.

// Waits for counter updates from interrupts.
class RotaryEncoder::Waiter {
 public:
  Waiter(async_context_t& context) : executor_(context) {}

  bool await_ready() {
    CriticalSectionLock lock(mutex_);
    return counter_.has_value();
  }

  bool await_suspend(std::coroutine_handle<> handle) {
    CriticalSectionLock lock(mutex);
    if (counter_.has_value()) {
      // Racing update between await_ready() and await_suspend().
      return false;
    }
    pending_ = handle;
    return true;
  }

  std::int64_t await_resume() {
    CriticalSectionLock lock(mutex_);
    const std::int64_t value = *counter_;
    counter_.reset();
    return value;
  }

  void Send(std::int64_t counter) {
    CriticalSectionLock lock(mutex_);
    if (counter_ == counter) {
      return;
    }
    counter_ = counter;
    if (std::coroutine_handle<> pending = std::exchange(pending_, nullptr)) {
      executor_.Schedule(pending);
    }
  }

 private:
  AsyncExecutor executor_;

  CriticalSection mutex_;
  std::optional<std::int64_t> counter_;
  std::coroutine_handle<> pending_;
};

// Global state per encoder.
struct RotaryEncoder::State {
  // The GPIO pin numbers for the encoder.
  std::array<unsigned, 2> pins;

  // The current encoder pins values. This assumes encoder switches shorting the
  // pins to ground on contact; i.e. 1 is disconnected, 0 is connected.
  std::bitset<2> values = 0b11;

  // Range [-3, +3] fractional detent state. When we reach ±4, we have advanced
  // to a new detent.
  int fractional_counter = 0;

  // Signed cumulative full dedents measured.
  std::int64_t counter = 0;

  // Only nullopt to allow for default construction. This is populated before
  // Init().
  std::optional<Waiter> waiter;

  // Setup pins and register the given interrupt handler for this pin pair.
  void Init(irq_handler_t edge_interrupt_handler);

  // Handle an edge transition on either signal.
  void HandleInterrupt();
};

template <unsigned pin_a, unsigned pin_b>
RotaryEncoder RotaryEncoder::Create(async_context_t& context) {
  // Unique tag type for each pin pair.
  using Tag = std::integer_sequence<unsigned, pin_a, pin_b>;
  using Singleton = InterruptHandlerSingleton<Tag, State>;

  State& state = Singleton::state;
  state.pins = {pin_a, pin_b};
  state.waiter.emplace(context);
  state.Init(Singleton::interrupt_handler);

  return RotaryEncoder(&state);
}

inline auto RotaryEncoder::operator co_await() {
  return AwaitableReference(*state_->waiter);
}
