#pragma once

#include <hardware/gpio.h>
#include <pico/async_context.h>
#include <pico/sync.h>

#include <functional>

// Interrupt-based GPIO input handler.
class DigitalInput {
 public:
  using ValueCallback = std::function<void(bool value)>;
  template <unsigned pin>
  static DigitalInput Create(async_context& context, ValueCallback callback);

 private:
  struct State;
  State* state_;

  template <unsigned pin>
  struct Singleton;
};

// Internal implementation details below.

struct DigitalInput::State {
  unsigned pin;
  critical_section_t value_critical_section;
  bool value = false;

  ValueCallback callback;
  async_context_t* async_context;
  async_when_pending_worker_t async_worker{.work_pending = false};

  void Init(irq_handler_t edge_interrupt_handler);
  void EdgeInterrupt();
  bool Value();
};

template <unsigned pin>
struct DigitalInput::Singleton {
  inline static State state{.pin = pin};
  static void EdgeInterrupt() { state.EdgeInterrupt(); }
  static void OnAsyncEvent(async_context_t* context,
                           async_when_pending_worker_t* async_worker) {
    state.callback(state.Value());
  }
};

template <unsigned pin>
DigitalInput DigitalInput::Create(async_context_t& context,
                                  ValueCallback callback) {
  State& state = Singleton<pin>::state;
  state.callback = callback;
  state.async_context = &context;
  state.async_worker.do_work = &Singleton<pin>::OnAsyncEvent;
  state.Init(&Singleton<pin>::EdgeInterrupt);

  DigitalInput input;
  input.state_ = &state;
  return input;
}
