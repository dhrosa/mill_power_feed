#pragma once

#include <hardware/gpio.h>

#include <cstdint>

#include "picopp/async.h"

// Interrupt-based GPIO input handler.
class DigitalInput {
 public:
  // Each time `pin` transitions from 1->0, `on_press` will be called within the
  // given async context.
  template <unsigned pin, typename F>
  static void SetPressHandler(async_context& context, F&& on_press);

 private:
  struct State;

  template <unsigned pin>
  struct Singleton;
};

// Internal implementation details below.

struct DigitalInput::State {
  unsigned pin;
  AsyncWorker async_worker;
  std::int64_t last_event_time_us = 0;

  void Init(irq_handler_t edge_interrupt_handler);
  void FallInterrupt();
};

template <unsigned pin>
struct DigitalInput::Singleton {
  inline static State state{.pin = pin};

  static void FallInterrupt() { state.FallInterrupt(); }
};

template <unsigned pin, typename F>
void DigitalInput::SetPressHandler(async_context_t& context, F&& on_press) {
  using SingletonType = Singleton<pin>;
  State& state = SingletonType::state;
  state.async_worker =
      AsyncWorker::Create<SingletonType>(context, std::forward<F>(on_press));
  state.Init(&Singleton<pin>::FallInterrupt);
}
