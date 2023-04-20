#pragma once

#include <pico/async_context.h>

#include <coroutine>

class AsyncExecutor {
 public:
  AsyncExecutor(async_context_t& context);

  auto Schedule();

  void Schedule(std::coroutine_handle<> handle);

 private:
  struct WorkerState {
    // Note: a standard-layout object is pointer-interconvertible with its first
    // member.
    async_at_time_worker_t worker;
    async_context_t& context;
    std::coroutine_handle<> handle;
  };

  // async_at_time_worker callback.
  static void ResumeInContext(async_context_t* context,
                              async_at_time_worker_t* worker);

  WorkerState state_;
};

inline AsyncExecutor::AsyncExecutor(async_context_t& context)
    : state_({.worker = {.do_work = &ResumeInContext}, .context = context}) {}

inline void AsyncExecutor::Schedule(std::coroutine_handle<> handle) {
  state_.handle = handle;
  async_context_add_at_time_worker_in_ms(&state_.context, &state_.worker, 0);
}

auto AsyncExecutor::Schedule() {
  struct Awaiter : std::suspend_always {
    AsyncExecutor& executor;

    void await_suspend(std::coroutine_handle<> handle) {
      executor.Schedule(handle);
    }
  };
  return Awaiter{.executor = *this};
}

void AsyncExecutor::ResumeInContext(async_context_t* context,
                                    async_at_time_worker_t* worker) {
  auto* state = (WorkerState*)(worker);
  state->handle.resume();
}
