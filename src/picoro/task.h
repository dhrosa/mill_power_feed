#pragma once

#include <coroutine>
#include <cstdint>

#include "picopp/critical_section.h"

// Simple coroutine with no return value. Execution is eager: the body of the
// coroutine will start executing immediately and continue until the first
// suspension point.
class Task {
 public:
  struct promise_type;

  Task(Task&& other) { *this = std::move(other); }
  Task& operator=(Task&& other);

  ~Task();

  // Awaitable that suspends the current coroutine until this task has
  // completed.
  auto operator co_await();

 private:
  Task(std::coroutine_handle<promise_type> handle_) : handle_(handle_) {}

  std::coroutine_handle<promise_type> handle_;
};

struct Task::promise_type {
  CriticalSection mutex;
  bool done = false;
  std::coroutine_handle<> waiter;

  Task get_return_object() {
    return {std::coroutine_handle<promise_type>::from_promise(*this)};
  }

  std::suspend_never initial_suspend() { return {}; }

  auto final_suspend() noexcept {
    struct FinalSuspend : std::suspend_always {
      promise_type& promise;
      std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle) {
        CriticalSectionLock lock(promise.mutex);
        promise.done = true;
        if (promise.waiter) {
          return promise.waiter;
        }
        return std::noop_coroutine();
      }
    };

    return FinalSuspend{.promise = *this};
  }
  void return_void() {}
};

inline Task& Task::operator=(Task&& other) {
  if (this != &other) {
    handle_ = std::exchange(other.handle_, nullptr);
  }
  return *this;
}

inline Task::~Task() {
  if (handle_) {
    handle_.destroy();
  }
}

inline auto Task::operator co_await() {
  struct Awaiter {
    promise_type& promise;

    bool await_ready() {
      CriticalSectionLock lock(promise.mutex);
      return promise.done;
    }

    bool await_suspend(std::coroutine_handle<> handle) {
      CriticalSectionLock lock(promise.mutex);
      if (promise.done) {
        return false;
      }
      promise.waiter = handle;
      return true;
    }

    void await_resume() {}
  };

  return Awaiter{.promise = handle_.promise()};
}
