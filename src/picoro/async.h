#pragma once

#include <pico/async_context.h>
#include <pico/time.h>

#include <coroutine>
#include <cstdint>

// Allows resumption of a coroutine onto an async_context. Each executor
// instance can manage at most one suspended coroutine.
class AsyncExecutor {
 public:
  // Not thread-safe.
  AsyncExecutor(async_context_t& context);
  ~AsyncExecutor();

  void operator=(const AsyncExecutor&) = delete;

  // Schedules a suspended coroutine for resumption on the async_context. This
  // method returns immediately and does not wait for the coroutine to resume.
  void Schedule(std::coroutine_handle<> handle);

  // An awaitable object that suspends the current coroutine and resumes it on
  // the async_context.
  auto Schedule();

  // Schedules a suspended coroutine for resumption on the async_context at the
  // given time.
  void ScheduleAt(absolute_time_t time, std::coroutine_handle<> handle);

  // An awaitable that suspends the current coroutine and resumes it on the
  // async_context at the given time.
  auto SleepUntil(absolute_time_t time);

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

  static std::int64_t AlarmCallback(alarm_id_t id, void* user_data);

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

inline void AsyncExecutor::ScheduleAt(absolute_time_t time,
                                      std::coroutine_handle<> handle) {
  state_.handle = handle;
  const bool fire_if_past = true;
  add_alarm_at(time, &AlarmCallback, this, fire_if_past);
}

inline auto AsyncExecutor::SleepUntil(absolute_time_t time) {
  struct Sleeper : std::suspend_always {
    AsyncExecutor& executor;
    absolute_time_t time;

    void await_suspend(std::coroutine_handle<> handle) {
      executor.ScheduleAt(time, handle);
    }
  };
  return Sleeper{.executor = *this, .time = time};
}

inline void AsyncExecutor::ResumeInContext(
    async_context_t* context, async_when_pending_worker_t* worker) {
  auto& state = reinterpret_cast<WorkerState&>(*worker);
  state.handle.resume();
}

inline std::int64_t AsyncExecutor::AlarmCallback(alarm_id_t id,
                                                 void* user_data) {
  auto& state = *static_cast<WorkerState*>(user_data);
  async_context_set_work_pending(&state.context, &state.worker);
  // Do not reschedule alarm.
  return 0;
}
