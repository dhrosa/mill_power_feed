#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include <hardware/pwm.h>

#include <iostream>

const uint pulse_pin = 25;

void pwm_pulse() {
  gpio_set_function(pulse_pin, GPIO_FUNC_PWM);
  const uint slice = pwm_gpio_to_slice_num(pulse_pin);
  const uint channel = pwm_gpio_to_channel(pulse_pin);

  pwm_set_clkdiv_int_frac(slice, 125, 0);
  pwm_set_wrap(slice, 9);
  pwm_set_chan_level(slice, channel, 5);
  pwm_set_enabled(slice, true);
  
  while (true) {
    sleep_ms(1000);
  }
}

int main() {
  stdio_init_all();
  pwm_pulse();
  return 0;
}
