#pragma once

#include <coroutine>
#include <optional>
#include <utility>

#include "picopp/critical_section.h"
#include "picopp/irq.h"
#include "picoro/async.h"
#include "picoro/awaitable_reference.h"

class Button {
 public:
  template <unsigned pin>
  static Button Create(async_context_t& context);

  void operator=(const Button&) = delete;

  // Awaits an update to the button state; returns true if the button is being
  // pressed, and false if the button is released.
  auto operator co_await();

 private:
  struct State;
  class Waiter;

  Button(State* state) : state_(state) {}
  State* state_;
};

class Button::Waiter {
 public:
  Waiter(async_context_t& context) : executor_(context) {}

  bool await_ready() {
    CriticalSectionLock lock(mutex_);
    return value_.has_value();
  }

  bool await_suspend(std::coroutine_handle<> handle) {
    CriticalSectionLock lock(mutex_);
    if (value_.has_value()) {
      // Racing update between await_ready() and await_suspend().
      return false;
    }
    pending_ = handle;
    return true;
  }

  bool await_resume() {
    CriticalSectionLock lock(mutex_);
    const bool value = *value_;
    value_.reset();
    return value;
  }

  void Send(bool value) {
    CriticalSectionLock lock(mutex_);
    if (value == value_) {
      return;
    }
    value_ = value;
    if (pending_) {
      executor_.Schedule(std::exchange(pending_, nullptr));
    }
  }

 private:
  AsyncExecutor executor_;

  CriticalSection mutex_;
  std::optional<bool> value_;
  std::coroutine_handle<> pending_;
};

struct Button::State {
  unsigned pin;
  std::optional<Waiter> waiter;

  void Init(irq_handler_t edge_interrupt_handler);

  void HandleInterrupt();
};

template <unsigned pin>
Button Button::Create(async_context_t& context) {
  using Tag = std::integral_constant<unsigned, pin>;
  using Singleton = InterruptHandlerSingleton<Tag, State>;

  State& state = Singleton::state;
  state.pin = pin;
  state.waiter.emplace(context);
  state.Init(Singleton::interrupt_handler);

  return Button(&state);
}

inline auto Button::operator co_await() {
  return AwaitableReference(*state_->waiter);
}
