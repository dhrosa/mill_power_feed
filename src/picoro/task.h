#pragma once

#include <coroutine>

#include "picopp/semaphore.h"

class Task {
 public:
  struct promise_type;

  Task(Task&& other) { *this = std::move(other); }
  Task& operator=(Task&& other);

  ~Task();

  void Wait();

 private:
  Task(std::coroutine_handle<promise_type> handle_) : handle_(handle_) {}

  std::coroutine_handle<promise_type> handle_;
};

struct Task::promise_type {
  Semaphore complete{0, 1};

  Task get_return_object() {
    return {std::coroutine_handle<promise_type>::from_promise(*this)};
  }

  std::suspend_always initial_suspend() { return {}; }

  auto final_suspend() noexcept {
    struct Awaiter : std::suspend_always {
      Semaphore& complete;
      void await_suspend(std::coroutine_handle<>) { complete.Release(); }
    };
    return Awaiter{.complete = complete};
  };
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

inline void Task::Wait() {
  if (!handle_) {
    return;
  }
  handle_.resume();
  handle_.promise().complete.Acquire();
}
