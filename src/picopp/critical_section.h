#pragma once

#include <pico/sync.h>

// C++ RAII wrapper around critical_section_enter_blocking and
// critical_section_exit.
class CriticalSectionLock {
 public:
  CriticalSectionLock(critical_section_t& section) : section_(section) {
    critical_section_enter_blocking(&section_);
  }

  ~CriticalSectionLock() { critical_section_exit(&section_); }

  // Not copyable or moveable.
  CriticalSectionLock(const CriticalSectionLock&) = delete;

 private:
  critical_section_t& section_;
};
