#pragma once

#include <pico/sync.h>

#include <cstdint>

// Single-shot event.
class Notification {
 public:
  Notification();
  void operator=(const Notification&) = delete;

  // Safe to call concurrently with Notified()/Wait().
  void Notify();

  // Notified() and Wait() must not not be called concurrently with eachother.

  [[nodiscard]] bool Notified();
  void Wait();

 private:
  semaphore_t sem_;
  bool notified_ = false;
};

inline Notification::Notification() { sem_init(&sem_, 0, 1); }

inline void Notification::Notify() { sem_release(&sem_); }

inline bool Notification::Notified() {
  if (notified_) {
    return true;
  }
  return sem_available(&sem_) == 1;
}

inline void Notification::Wait() {
  if (notified_) {
    return;
  }
  sem_acquire_blocking(&sem_);
  notified_ = true;
}
