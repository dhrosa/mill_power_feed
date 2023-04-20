#pragma once

#include <pico/sync.h>

#include <cstdint>

class Semaphore {
 public:
  Semaphore(std::int16_t initial, std::int16_t max);
  void operator=(const Semaphore&) = delete;

  semaphore_t& get() { return sem_; }

  void Acquire() { sem_acquire_blocking(&sem_); }
  void Release() { sem_release(&sem_); }

 private:
  semaphore_t sem_;
};

inline Semaphore::Semaphore(std::int16_t initial, std::int16_t max) {
  sem_init(&sem_, initial, max);
}
