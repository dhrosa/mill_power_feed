#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <hardware/pwm.h>

#include "ssd1306.h"
#include "textRenderer/TextRenderer.h"
#include "rotary_encoder.h"

#include <atomic>
#include <iostream>

int main() {
  stdio_usb_init();

  i2c_init(i2c0, 1'000'000);
  for (uint pin : {PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN}) {
    gpio_set_function(pin, GPIO_FUNC_I2C);
    gpio_pull_up(pin);
  }
  pico_ssd1306::SSD1306 display(i2c0, 0x3C, pico_ssd1306::Size::W128xH32);
  pico_ssd1306::drawText(&display, font_12x16, "test", 0, 0);
  display.sendBuffer();

  RotaryEncoder encoder(15, 25);
  for (std::int64_t i = 0;; ++i) {
    std::cout << i << ": " << encoder.Read() << std::endl;
    sleep_ms(100);
  }

  return 0;
}
