#pragma once

#include <pico/async_context.h>

#include <concepts>
#include <functional>
#include <optional>

template <typename F>
concept IsAsyncWorkerFunctor = std::invocable<F>;

// RAII wrapper around async_when_pending_worker_t that allows use of arbitrary
// C++ functors.
class AsyncWorker {
 public:
  // Creates an async worker that calls `do_work` each time it's awoken. This
  // must only be called once for each type `F`. To ensure there aren't
  // accidentally overlapping F types, the user should use a lambda for the
  // functor, as lambdas have unique types.
  template <typename F>
    requires IsAsyncWorkerFunctor<F>
  static AsyncWorker Create(async_context_t& context, F&& do_work);

  // Remove the worker from its async context.
  ~AsyncWorker();

 private:
  struct State;

  template <typename F>
  struct Singleton;

  AsyncWorker(State& state) : state_(state) {}
  State& state_;
};

struct AsyncWorker::State {
  async_context_t* context;
  async_when_pending_worker_t worker{.work_pending = false};
};

// Creates a unique static DoWork function for each F. This is needed because
// async_when_pending_worker_t doesn't provide a way to pass user data to the
// callback, so we can't share a single callback.
template <typename F>
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

template <typename F>
  requires IsAsyncWorkerFunctor<F>
AsyncWorker AsyncWorker::Create(async_context_t& context, F&& do_work) {
  Singleton<F>::do_work = std::forward<F>(do_work);
  State& state = Singleton<F>::state;
  state.context = &context;
  state.worker.do_work = &Singleton<F>::DoWork;
  async_context_add_when_pending_worker(state.context, &state.worker);
  return AsyncWorker{state};
}

AsyncWorker::~AsyncWorker() {
  async_context_remove_when_pending_worker(state_.context, &state_.worker);
}
