#pragma once

#include <utility>

#include "picopp/critical_section.h"
#include "picoro/async.h"

// Reusable binary event awaitable.
class Event {
 public:
  // Not thread-safe.
  Event(async_context_t& context);

  void operator=(const Event&) = delete;

  // Wake any sleeping waiters.
  void Notify();

  // Awaitable that suspends the current coroutine until notified. Execution
  // might resume immediately if the event was already notified. If not, the
  // coroutine will be resumed on the async_context provided in the constructor.
  // When the coroutine is resumed, the event is reset to its unnotified state.
  auto operator co_await();

 private:
  AsyncExecutor executor_;

  CriticalSection mutex_;
  bool notified_ = false;
  std::coroutine_handle<> waiter_;
};

inline Event::Event(async_context_t& context) : executor_(context) {}

inline void Event::Notify() {
  CriticalSectionLock lock(mutex_);
  notified_ = true;
  if (waiter_) {
    executor_.Schedule(std::exchange(waiter_, nullptr));
  }
}

inline auto Event::operator co_await() {
  struct Waiter {
    Event& event;

    bool await_ready() {
      CriticalSectionLock lock(event.mutex_);
      return event.notified_;
    }

    bool await_suspend(std::coroutine_handle<> handle) {
      CriticalSectionLock lock(event.mutex_);
      if (event.notified_) {
        return false;
      }
      event.waiter_ = handle;
      return true;
    }

    void await_resume() {
      CriticalSectionLock lock(event.mutex_);
      event.notified_ = false;
    }
  };
  return Waiter{.event = *this};
}
