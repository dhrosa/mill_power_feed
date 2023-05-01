#pragma once

#include <coroutine>

// For some reason directly returning a reference to an awaitable results in the
// await_* methods of the object being called with an incorrect 'this' pointer
// in GCC 12.2. Possibly due to
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=104872
//
// To work around this we return a new awaitable object by value that holds a
// reference to the original awaitable. In practice GCC optimizes the
// indirection away.

template <typename T>
auto AwaitableReference(T& awaitable) {
  struct Wrapper {
    T& awaitable;

    bool await_ready() { return awaitable.await_ready(); }
    auto await_suspend(std::coroutine_handle<> h) {
      return awaitable.await_suspend(h);
    }
    auto await_resume() { return awaitable.await_resume(); }
  };
  return Wrapper{.awaitable = awaitable};
}
