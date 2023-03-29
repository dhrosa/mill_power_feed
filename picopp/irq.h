#pragma once

#include <hardware/irq.h>

// Type-erasure mechanism that allows usage of C++ functors as stateful
// interrupt handlers. The `Tag` type simply needs to be unique per interrupt
// handler. `State` must be default-constructible and have a HandleInterrupt()
// method.
template <typename Tag, typename State>
struct InterruptHandlerSingleton {
  inline static State state = {};

  static void HandleInterrupt() { state.HandleInterrupt(); }

  // Type-erased interrupt handler.
  static constexpr irq_handler_t interrupt_handler = &HandleInterrupt;
};
