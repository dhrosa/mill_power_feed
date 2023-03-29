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
  SingletonType::do_work = std::forward<F>(do_work);
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
