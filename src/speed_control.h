#pragma once

#include <cstdint>

class SpeedControl {
 public:
  SpeedControl(std::int64_t sys_clock_hz, unsigned pulse_pin, unsigned dir_pin);

  void Set(std::int64_t freq_hz);

 private:
  const std::int64_t sys_clock_hz_;
  const unsigned pulse_pin_;
  const unsigned dir_pin_;

  unsigned slice_;
  unsigned channel_;
};
