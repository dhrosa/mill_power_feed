#include "oled_buffer.h"

OledBuffer::OledBuffer(std::uint8_t* data, std::size_t width,
                       std::size_t height)
    : data_(data), width_(width), height_(height) {}

auto OledBuffer::operator()(std::size_t x, std::size_t y) -> Pixel {
  const std::size_t block_col = x;
  const std::size_t block_row = y >> 3;
  const std::size_t offset = y & 0b111;
  return Pixel(data_[block_col + block_row * width_], offset);
}

void OledBuffer::Clear() {
  for (auto& value : Span()) {
    value = 0;
  }
}

std::span<const std::uint8_t> OledBuffer::Span() const {
  return {data_, width_ * height_ / 8};
}

std::span<std::uint8_t> OledBuffer::Span() {
  return {data_, width_ * height_ / 8};
}

void OledBuffer::DrawChar(const Font& font, char letter, std::size_t x0,
                          std::size_t y0) {
  OledBuffer font_data(const_cast<std::uint8_t*>(font[letter]), font.width,
                       font.height);

  const auto x1 = std::min(width_, x0 + font.width);
  const auto y1 = std::min(height_, y0 + font.height);

  const std::size_t width = x1 - x0;
  const std::size_t height = y1 - y0;
  for (std::size_t dx = 0; dx < width; ++dx) {
    for (std::size_t dy = 0; dy < height; ++dy) {
      if (font_data(dx, dy)) {
        (*this)(x0 + dx, y0 + dy) = 1;
      }
    }
  }
}

void OledBuffer::DrawString(const Font& font, std::string_view text,
                            std::size_t x0, std::size_t y0) {
  for (char letter : text) {
    DrawChar(font, letter, x0, y0);
    x0 += font.width;
  }
}

void OledBuffer::DrawLineH(std::size_t y, std::size_t x0, std::size_t x1) {
  for (std::size_t x = x0; x < x1; ++x) {
    (*this)(x, y) = true;
  }
}

OledBuffer::Pixel::Pixel(std::uint8_t& block, std::uint8_t offset)
    : block_(block), offset_(offset) {}

OledBuffer::Pixel::operator bool() const { return block_ & (1 << offset_); }

void OledBuffer::Pixel::operator=(bool value) {
  const std::uint8_t rhs = 1 << offset_;
  if (value) {
    block_ |= rhs;
  } else {
    block_ &= ~rhs;
  }
}
