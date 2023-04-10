#pragma once

#include <cstdint>
#include <vector>

#include "picopp/gpio.h"
#include "picopp/spi.h"

#include "oled_buffer.h"

// SSD1309 display driver.
class Oled {
 public:
  struct Pins {
    unsigned clock;
    unsigned data;
    unsigned reset;
    unsigned dc;
    unsigned cs;
  };

  Oled(spi_inst_t* spi, Pins pins);

  void Reset();

  std::size_t Width() const { return width_; }
  std::size_t Height() const { return height_; }

  OledBuffer& Buffer() { return buffer_; }

  void Update();

 private:
  void DataMode() { data_mode_.Set(); }
  void CommandMode() { data_mode_.Clear(); }

  void SendCommands(std::span<const std::uint8_t> commands);

  static constexpr std::size_t width_ = 128;
  static constexpr std::size_t height_ = 64;
  Spi spi_;
  Gpio spi_clock_;
  Gpio spi_data_;
  Gpio reset_;
  Gpio data_mode_;
  Gpio chip_select_;

  OledBuffer buffer_;
  std::vector<std::uint8_t> packed_buffer_;
};
