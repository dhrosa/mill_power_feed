#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <hardware/pwm.h>

#include "ssd1306.h"
#include "textRenderer/TextRenderer.h"

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
  
  i2c_init(i2c0, 1'000'000);
  for (uint pin : {PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN}) {
    gpio_set_function(pin, GPIO_FUNC_I2C);
    gpio_pull_up(pin);
  }
  pico_ssd1306::SSD1306 display(i2c0, 0x3C, pico_ssd1306::Size::W128xH32);

  bool orientation = false;
  while(true) {
    for (const unsigned char* font : {font_5x8, font_8x8, font_12x16, font_16x32}) {
      display.clear();
      pico_ssd1306::drawText(&display, font, "sup world", 0, 0);
      display.sendBuffer();
      sleep_ms(1000);
    }
  }
  return 0;
}
