#pragma once

#include <hardware/gpio.h>

// C++ wrapper around a GPIO pin.
class Gpio {
 public:
  enum Polarity {
    // true -> HIGH, false -> LOW
    kPositive,
    // true -> LOW, false -> HIGH
    kNegative,
  };

  enum Direction {
    kInput,
    kOutput,
  };

  struct Config {
    gpio_function function = GPIO_FUNC_SIO;
    Direction direction = kOutput;
    Polarity polarity = kPositive;
  };

  Gpio(unsigned pin);
  Gpio(unsigned pin, Config config);

  bool Get();

  void Set(bool value = true);
  void Clear() { Set(false); }

  operator bool() { return Get(); }
  void operator=(bool value) { Set(value); }

 private:
  const unsigned pin_;
  const Config config_;
};

inline Gpio::Gpio(unsigned pin) : Gpio(pin, Config{}) {}

inline Gpio::Gpio(unsigned pin, Config config) : pin_(pin), config_(config) {
  if (config_.function == GPIO_FUNC_SIO) {
    gpio_init(pin_);
    if (config_.direction == kOutput) {
      gpio_set_dir(pin_, true);
      Clear();
    } else {
      gpio_pull_up(pin_);
    }
    Clear();
  } else {
    gpio_set_function(pin_, config_.function);
  }
}

inline bool Gpio::Get() {
  const bool value = gpio_get(pin_);
  if (config_.polarity == kNegative) {
    return !value;
  }
  return value;
}

inline void Gpio::Set(bool value) {
  if (config_.polarity == kNegative) {
    value = !value;
  }
  gpio_put(pin_, value);
}
