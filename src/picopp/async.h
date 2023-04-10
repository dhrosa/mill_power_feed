#pragma once

#include <pico/async_context.h>

#include <concepts>
#include <functional>
#include <optional>
#include <utility>

template <typename F>
concept IsAsyncWorkerFunctor = std::invocable<F>;

// RAII wrapper around async_when_pending_worker_t that allows use of arbitrary
// C++ functors.
class AsyncWorker {
 public:
  // Creates an async worker that calls `do_work` each time it's awoken. `Tag`
  // must be a type unique to each Create() call, but other than that the
  // properties of the `Tag` type are irrelevant.
  template <typename Tag, IsAsyncWorkerFunctor F>
  static AsyncWorker Create(async_context_t& context, F&& do_work);

  // Convenience overload for the above when the functor type can also act as
  // the unique tag type. For example, lambdas have unique types.
  template <IsAsyncWorkerFunctor F>
  static AsyncWorker Create(async_context_t& context, F&& do_work) {
    return Create<F, F>(context, std::forward<F>(do_work));
  }

  // Empty state. The only valid operations on an empty worker are destruction
  // and moving.
  AsyncWorker() = default;

  // Remove the worker from its async context if non-empty.
  ~AsyncWorker();

  AsyncWorker(AsyncWorker&& other) { *this = std::move(other); }
  AsyncWorker& operator=(AsyncWorker&& other) {
    if (&other != this) {
      state_ = std::exchange(other.state_, nullptr);
    }
    return *this;
  }

  // Mark the worke as having work pending, which will cause the worker to be
  // run from the async context at some later time.
  void SetWorkPending();

 private:
  struct State;

  template <typename Tag, typename F>
  struct Singleton;

  AsyncWorker(State& state) : state_(&state) {}
  State* state_ = nullptr;
};

struct AsyncWorker::State {
  async_context_t* context;
  async_when_pending_worker_t worker{.work_pending = false};
};

// Creates a unique static DoWork function for each F. This is needed because
// async_when_pending_worker_t doesn't provide a way to pass user data to the
// callback, so we can't share a single callback.
template <typename Tag, typename F>
struct AsyncWorker::Singleton {
  inline static State state = {};
  // Using optional to avoid needing a default-constructible functor; this
  // member is never read in its null state.
  inline static std::optional<F> do_work = std::nullopt;

  static void DoWork(async_context_t* context,
                     async_when_pending_worker_t* worker) {
    (*do_work)();
  }
};

template <typename Tag, IsAsyncWorkerFunctor F>
AsyncWorker AsyncWorker::Create(async_context_t& context, F&& do_work) {
  using SingletonType = Singleton<Tag, F>;
  SingletonType::do_work.emplace(do_work);
  State& state = SingletonType::state;
  state.context = &context;
  state.worker.do_work = &SingletonType::DoWork;
  async_context_add_when_pending_worker(state.context, &state.worker);
  return AsyncWorker(state);
}

inline AsyncWorker::~AsyncWorker() {
  if (state_ == nullptr) {
    return;
  }
  async_context_remove_when_pending_worker(state_->context, &state_->worker);
}

inline void AsyncWorker::SetWorkPending() {
  async_context_set_work_pending(state_->context, &state_->worker);
}

// RAII wrapper around async_at_time_worker_t.
class AsyncScheduledWorker {
 public:
  // Creates an async worker that calls `do_work` each time it's awoken. `Tag`
  // must be a type unique to each Create() call, but other than that the
  // properties of the `Tag` type are irrelevant.
  //
  // The functor should return an absolute_time_t indicating when it should next
  // be automatically scheduled. nil_time may be returned to indicate that we
  // should not automatically reschedule.
  template <typename Tag, typename F>
  static AsyncScheduledWorker Create(async_context_t& context, F&& do_work);

  // Convenience overload for the above when the functor type can also act as
  // the unique tag type. For example, lambdas have unique types.
  template <typename F>
  static AsyncScheduledWorker Create(async_context_t& context, F&& do_work) {
    return Create<F, F>(context, std::forward<F>(do_work));
  }

  // Empty state. The only valid operations on an empty worker are destruction
  // and moving.
  AsyncScheduledWorker() = default;

  // Remove the worker from its async context if non-empty.
  ~AsyncScheduledWorker();

  AsyncScheduledWorker(AsyncScheduledWorker&& other) {
    *this = std::move(other);
  }

  AsyncScheduledWorker& operator=(AsyncScheduledWorker&& other) {
    if (&other != this) {
      state_ = std::exchange(other.state_, nullptr);
    }
    return *this;
  }

  // Schedule the worker to execute at the given time. No-op if the worker is
  // already scheduled.
  void ScheduleAt(absolute_time_t time);

 private:
  struct State;

  template <typename Tag, typename F>
  struct Singleton;

  State* state_;
};

struct AsyncScheduledWorker::State {
  async_context_t* context;
  async_at_time_worker_t worker;

  void ScheduleAt(absolute_time_t time) {
    async_context_add_at_time_worker_at(context, &worker, time);
  }
};

template <typename Tag, typename F>
struct AsyncScheduledWorker::Singleton {
  inline static State state = {};

  // Using optional to avoid needing a default-constructible functor; this
  // member is never read in its null state.
  inline static std::optional<F> do_work = std::nullopt;

  static void DoWork(async_context_t* context, async_at_time_worker_t* worker) {
    const absolute_time_t next_time = (*do_work)();
    if (is_nil_time(next_time)) {
      return;
    }
    state.ScheduleAt(next_time);
  }
};

template <typename Tag, typename F>
AsyncScheduledWorker AsyncScheduledWorker::Create(async_context_t& context,
                                                  F&& do_work) {
  using SingletonType = Singleton<Tag, F>;
  SingletonType::do_work.emplace(do_work);
  State& state = SingletonType::state;
  state.context = &context;
  state.worker.do_work = &SingletonType::DoWork;

  AsyncScheduledWorker worker;
  worker.state_ = &state;
  return worker;
}

inline void AsyncScheduledWorker::ScheduleAt(absolute_time_t time) {
  state_->ScheduleAt(time);
}
