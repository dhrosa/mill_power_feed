#pragma once

#include <coroutine>

#include "picoro/notification.h"

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
  Notification complete;

  Task get_return_object() {
    return {std::coroutine_handle<promise_type>::from_promise(*this)};
  }

  std::suspend_always initial_suspend() { return {}; }

  auto final_suspend() noexcept {
    struct Awaiter : std::suspend_always {
      Notification& complete;
      void await_suspend(std::coroutine_handle<>) { complete.Notify(); }
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
  handle_.promise().complete.Wait();
}
