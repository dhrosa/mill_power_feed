#pragma once

#include <cstdint>

// Note: currently only supports a single encoder instance globally.
class RotaryEncoder {
 public:
  RotaryEncoder(unsigned pin_a, unsigned pin_b);

  std::int64_t Read();
};
