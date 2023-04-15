#include "oled_buffer.h"

OledBuffer::Pixel::Pixel(std::uint8_t& block, std::uint8_t offset)
    : block_(block), offset_(offset) {}

void OledBuffer::Pixel::operator=(bool value) {
  const std::uint8_t rhs = 1 << offset_;
  if (value) {
    block_ |= rhs;
  } else {
    block_ &= ~rhs;
  }
}

OledBuffer::OledBuffer(std::span<std::uint8_t> data, std::size_t width,
                       std::size_t height)
    : data_(data), width_(width), height_(height) {}

auto OledBuffer::operator()(std::size_t x, std::size_t y) -> Pixel {
  const std::size_t block_col = x;
  const std::size_t block_row = y >> 3;
  const std::size_t offset = y & 0b111;
  return Pixel(data_[block_col + block_row * width_], offset);
}

void OledBuffer::Clear() {
  for (auto& value : data_) {
    value = 0;
  }
}
