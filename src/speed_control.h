#pragma once

#include <cstdint>

#include "picopp/gpio.h"

class SpeedControl {
 public:
  SpeedControl(std::int64_t sys_clock_hz, unsigned pulse_pin, unsigned dir_pin);

  void Set(double freq_hz);

 private:
  const std::int64_t sys_clock_hz_;
  Gpio direction_;

  unsigned slice_;
  unsigned channel_;
};
