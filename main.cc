#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <hardware/pwm.h>
#include <pico/stdlib.h>

#include <array>
#include <cstdio>
#include <iostream>
#include <string>

#include "rotary_encoder.h"
#include "ssd1306.h"
#include "textRenderer/TextRenderer.h"

void DrawCenteredValue(pico_ssd1306::SSD1306& display, int value) {
  const unsigned char* font = font_16x32;
  constexpr int line_width = 128;
  constexpr int char_width = 16;
  constexpr int chars_per_line = line_width / char_width;

  std::array<char, chars_per_line + 1> buffer;
  const int chars_written =
      std::snprintf(buffer.data(), buffer.size(), "%+d", value);
  const int left_padding = (chars_per_line - chars_written) / 2;

  display.clear();
  pico_ssd1306::drawText(&display, font, buffer.data(),
                         left_padding * char_width, 0);
  display.sendBuffer();
}

int main() {
  stdio_usb_init();
  sleep_ms(2000);
  std::cout << "startup" << std::endl;

  RotaryEncoder<16, 15> encoder_a;
  RotaryEncoder<5, 21> encoder_b;
  RotaryEncoder<19, 18> encoder_c;
  irq_set_enabled(IO_IRQ_BANK0, true);

  i2c_init(i2c0, 1'000'000);
  for (uint pin : {PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN}) {
    gpio_set_function(pin, GPIO_FUNC_I2C);
    gpio_pull_up(pin);
  }
  pico_ssd1306::SSD1306 display(i2c0, 0x3C, pico_ssd1306::Size::W128xH32);
  display.clear();
  display.sendBuffer();

  DrawCenteredValue(display, 1234);

  while (true) {
    display.clear();
    const unsigned char* font = font_12x16;
    const std::array<int, 2> positions[3] = {{0, 0}, {64, 0}, {0, 16}};
    const std::int64_t values[3] = {encoder_a.Read(), encoder_b.Read(),
                                    encoder_c.Read()};
    for (int i = 0; i < 3; ++i) {
      const auto text = std::to_string(values[i]);
      auto [x, y] = positions[i];
      pico_ssd1306::drawText(&display, font, text.c_str(), x, y);
    }
    display.sendBuffer();
    sleep_ms(100);
  }
  return 0;
}
