#pragma once

#include <hardware/gpio.h>

// C++ wrapper around a GPIO pin. Currently only supports output.
class Gpio {
 public:
  enum Polarity {
    // true -> HIGH, false -> LOW
    kPositive,
    // true -> LOW, false -> HIGH
    kNegative,
  };

  struct Config {
    gpio_function function = GPIO_FUNC_SIO;
    Polarity polarity = kPositive;
  };

  Gpio(unsigned pin);
  Gpio(unsigned pin, Config config);

  void Set(bool value = true);
  void Clear() { Set(false); }

  void operator=(bool value) { Set(value); }

 private:
  const unsigned pin_;
  const Config config_;
};

Gpio::Gpio(unsigned pin) : Gpio(pin, Config{}) {}

Gpio::Gpio(unsigned pin, Config config) : pin_(pin), config_(config) {
  if (config_.function == GPIO_FUNC_SIO) {
    gpio_init(pin_);
    // Set pin to output.
    gpio_set_dir(pin_, true);
    Clear();
  } else {
    gpio_set_function(pin_, config_.function);
  }
}

void Gpio::Set(bool value) {
  if (config_.polarity == kNegative) {
    value = !value;
  }
  gpio_put(pin_, value);
}
