#pragma once

#include <pico/sync.h>

class CriticalSection {
 public:
  CriticalSection() { critical_section_init(&section_); }
  ~CriticalSection() { critical_section_deinit(&section_); }
  critical_section_t& get() { return section_; }

  void Lock() { critical_section_enter_blocking(&section_); }
  void Unlock() { critical_section_exit(&section_); }

 private:
  critical_section_t section_;
};

// C++ RAII wrapper around critical_section_enter_blocking and
// critical_section_exit.
class CriticalSectionLock {
 public:
  CriticalSectionLock(critical_section_t& section) : section_(section) {
    critical_section_enter_blocking(&section_);
  }

  CriticalSectionLock(CriticalSection& section)
      : CriticalSectionLock(section.get()) {}

  ~CriticalSectionLock() { critical_section_exit(&section_); }

  // Not copyable or moveable.
  CriticalSectionLock(const CriticalSectionLock&) = delete;

 private:
  critical_section_t& section_;
};
