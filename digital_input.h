#pragma once

#include <cstdint>
#include <type_traits>

#include "picopp/async.h"
#include "picopp/irq.h"

// Interrupt-based GPIO input handler.
class DigitalInput {
 public:
  // Each time `pin` transitions from 1->0, `on_press` will be called within the
  // given async context.
  template <unsigned pin, typename F>
  static void SetPressHandler(async_context& context, F&& on_press);

 private:
  struct State;
};

// Internal implementation details below.

struct DigitalInput::State {
  unsigned pin;
  AsyncWorker async_worker;
  std::int64_t last_event_time_us = 0;

  void Init(irq_handler_t edge_interrupt_handler);
  void HandleInterrupt();
};

template <unsigned pin, typename F>
void DigitalInput::SetPressHandler(async_context_t& context, F&& on_press) {
  using Tag = std::integral_constant<unsigned, pin>;
  using Singleton = InterruptHandlerSingleton<Tag, State>;
  State& state = Singleton::state;
  state.pin = pin;
  state.async_worker =
      AsyncWorker::Create<Singleton>(context, std::forward<F>(on_press));
  state.Init(Singleton::interrupt_handler);
}
