#include "oled.h"

#include <hardware/gpio.h>
#include <pico/time.h>

namespace {
enum Command : std::uint8_t {
  kPowerOn = 0xAF,
  kDisplayOn = 0xA5,
  kDisplayOff = 0xA4,
  kSetAddressMode = 0x20,
  kFlipHorizontally = 0xA1,
};
}  // namespace

Oled::Oled(Spi spi, Pins pins)
    : spi_(std::move(spi)),
      spi_clock_(pins.clock, {.function = GPIO_FUNC_SPI}),
      spi_data_(pins.data, {.function = GPIO_FUNC_SPI}),
      reset_(pins.reset, {.polarity = Gpio::kNegative}),
      chip_select_(pins.cs, {.polarity = Gpio::kNegative}),
      data_mode_(pins.dc) {}

void Oled::Reset() {
  reset_.Set();
  sleep_us(3);
  reset_.Clear();

  // Vertical address mode.
  const std::uint8_t address_mode = 1;
  SendCommands({{
      kPowerOn,
      kDisplayOff,
      kSetAddressMode,
      address_mode,
      kFlipHorizontally,
      kDisplayOn,
  }});
}

void Oled::SendCommands(std::span<const std::uint8_t> commands) {
  CommandMode();
  spi_.Write(commands);
}
