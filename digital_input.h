#pragma once

#include <hardware/gpio.h>
#include <pico/async_context.h>

#include <cstdint>
#include <functional>

// Interrupt-based GPIO input handler.
class DigitalInput {
 public:
  using OnPress = std::function<void()>;

  template <unsigned pin>
  static void SetPressHandler(async_context& context, OnPress on_press);

 private:
  struct State;

  template <unsigned pin>
  struct Singleton;
};

// Internal implementation details below.

struct DigitalInput::State {
  unsigned pin;
  OnPress on_press;
  async_context_t* async_context;
  async_when_pending_worker_t async_worker{.work_pending = false};

  std::uint64_t last_event_time_us = 0;

  void Init(irq_handler_t edge_interrupt_handler);
  void FallInterrupt();
};

template <unsigned pin>
struct DigitalInput::Singleton {
  inline static State state{.pin = pin};
  static void FallInterrupt() { state.FallInterrupt(); }
  static void CallPressHandler(async_context_t* context,
                               async_when_pending_worker_t* async_worker) {
    state.on_press();
  }
};

template <unsigned pin>
void DigitalInput::SetPressHandler(async_context_t& context, OnPress on_press) {
  State& state = Singleton<pin>::state;
  state.on_press = on_press;
  state.async_context = &context;
  state.async_worker.do_work = &Singleton<pin>::CallPressHandler;
  state.Init(&Singleton<pin>::FallInterrupt);
}
