#include "speed_control.h"

#include <hardware/gpio.h>
#include <hardware/pwm.h>

#include <cmath>
#include <iostream>
#include <limits>

SpeedControl::SpeedControl(std::int64_t sys_clock_hz, unsigned pulse_pin,
                           unsigned dir_pin)
    : sys_clock_hz_(sys_clock_hz), direction_(Gpio(dir_pin)) {
  gpio_set_function(pulse_pin, GPIO_FUNC_PWM);

  slice_ = pwm_gpio_to_slice_num(pulse_pin);
  channel_ = pwm_gpio_to_channel(pulse_pin);

  pwm_set_clkdiv(slice_, 125);
  pwm_set_phase_correct(slice_, true);
}

void SpeedControl::Set(double freq_hz) {
  pwm_set_enabled(slice_, false);
  if (freq_hz == 0) {
    std::cout << "requested speeed of 0; stopping." << std::endl;
    return;
  }
  direction_ = freq_hz > 0;
  const double magnitude = std::abs(freq_hz);
  // 1MHz base clock.
  constexpr double max_wrap = std::numeric_limits<std::uint16_t>::max();
  const std::uint16_t wrap = std::min(max_wrap, 1'000'000 / (2 * magnitude));
  std::cout << "setting wrap to " << wrap
            << "; requested frequency: " << freq_hz
            << "; actual frequency will be : " << (1'000'000 / double(2 * wrap))
            << std::endl;
  pwm_set_wrap(slice_, wrap - 1);
  pwm_set_chan_level(slice_, channel_, wrap / 2);
  pwm_set_counter(slice_, 0);
  pwm_set_enabled(slice_, true);
}
