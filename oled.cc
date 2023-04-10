#include "oled.h"

#include <hardware/gpio.h>
#include <pico/time.h>

namespace {
enum Command : std::uint8_t {
  kPowerOn = 0xAF,
  kDisplayOff = 0xA5,
  kDisplayOn = 0xA4,
  kSetAddressMode = 0x20,
  kFlipHorizontally = 0xA1,
  kFlipVertically = 0xC8,
};
}  // namespace

Oled::Oled(spi_inst_t* spi, Pins pins)
    : spi_(spi, 8'000'000),
      spi_clock_(pins.clock, {.function = GPIO_FUNC_SPI}),
      spi_data_(pins.data, {.function = GPIO_FUNC_SPI}),
      reset_(pins.reset, {.polarity = Gpio::kNegative}),
      chip_select_(pins.cs, {.polarity = Gpio::kNegative}),
      data_mode_(pins.dc),
      buffer_(width_, height_),
      packed_buffer_(width_ * height_ / 8) {
  chip_select_.Set();
  Reset();
}

void Oled::Reset() {
  reset_.Set();
  sleep_us(5);
  reset_.Clear();
  sleep_us(1);

  // Horizontal address mode.
  const std::uint8_t address_mode = 0;
  SendCommands({{
      kPowerOn,
      kDisplayOff,
      kSetAddressMode,
      address_mode,
      kFlipHorizontally,
      kFlipVertically,
      kDisplayOn,
  }});
}

void Oled::SendCommands(std::span<const std::uint8_t> commands) {
  CommandMode();
  spi_.Write(commands);
}

void Oled::Update() {
  const std::span<std::uint8_t> packed_buffer(packed_buffer_);
  for (std::size_t block_index = 0; block_index < packed_buffer.size();
       ++block_index) {
    std::uint8_t& packed_value = packed_buffer[block_index];
    packed_value = 0;
    const std::size_t block_column = block_index % width_;
    const std::size_t block_row = block_index / width_;
    const std::size_t x = block_column;
    for (std::size_t bit_index = 0; bit_index < 8; ++bit_index) {
      const std::size_t y = block_row * 8 + bit_index;
      packed_value |= (buffer_(x, y)) << bit_index;
    }
  }
  DataMode();
  spi_.Write(packed_buffer);
}
