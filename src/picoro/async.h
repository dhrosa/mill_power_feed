#pragma once

#include <pico/async_context.h>

#include <coroutine>

// Allows resumption of a coroutine onto an async_context. Each executor
// instance can manage at most one suspended coroutine.
class AsyncExecutor {
 public:
  // Not thread-safe.
  AsyncExecutor(async_context_t& context);
  ~AsyncExecutor();

  void operator=(const AsyncExecutor&) = delete;

  // An awaitable object that suspends the current coroutine and resumes it on
  // the async_context. Thread-safe.
  auto Schedule();

  // Schedules a suspended coroutine for resumption on the async_context. This
  // method returns immediately and does not wait for the coroutine to resume.
  // Thread-safe.
  void Schedule(std::coroutine_handle<> handle);

 private:
  struct WorkerState {
    // Note: a standard-layout object is pointer-interconvertible with its first
    // member.
    async_when_pending_worker_t worker;
    async_context_t& context;
    std::coroutine_handle<> handle;
  };

  // async_context worker callback.
  static void ResumeInContext(async_context_t* context,
                              async_when_pending_worker_t* worker);

  WorkerState state_;
};

inline AsyncExecutor::AsyncExecutor(async_context_t& context)
    : state_({.worker = {.do_work = &ResumeInContext, .work_pending = false},
              .context = context}) {
  async_context_add_when_pending_worker(&state_.context, &state_.worker);
}

inline AsyncExecutor::~AsyncExecutor() {
  async_context_remove_when_pending_worker(&state_.context, &state_.worker);
}

inline void AsyncExecutor::Schedule(std::coroutine_handle<> handle) {
  state_.handle = handle;
  async_context_set_work_pending(&state_.context, &state_.worker);
}

inline auto AsyncExecutor::Schedule() {
  struct Awaiter : std::suspend_always {
    AsyncExecutor& executor;

    void await_suspend(std::coroutine_handle<> handle) {
      executor.Schedule(handle);
    }
  };
  return Awaiter{.executor = *this};
}

inline void AsyncExecutor::ResumeInContext(
    async_context_t* context, async_when_pending_worker_t* worker) {
  auto& state = reinterpret_cast<WorkerState&>(*worker);
  state.handle.resume();
}
