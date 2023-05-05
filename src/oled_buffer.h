#pragma once

#include <cstdint>
#include <span>
#include <string_view>

#include "font/font.h"

// Allows addressing individual bits in a packed bitmap by (x, y) coordinate.
//
// Data internally is stored as a row-major array of 8-bit vertically-oriented
// blocks, with the LSB oriented towards the top (greatest y-value) row of the
// image.
class OledBuffer {
 public:
  class Pixel;

  OledBuffer(uint8_t* data, std::size_t width, std::size_t height);

  Pixel operator()(std::size_t x, std::size_t y);

  std::size_t Width() const { return width_; }
  std::size_t Height() const { return height_; }

  void Clear();

  std::span<const std::uint8_t> Span() const;
  std::span<std::uint8_t> Span();

  void DrawChar(const Font& font, char letter, std::size_t x0, std::size_t y0);

  void DrawString(const Font& font, std::string_view text, std::size_t x0,
                  std::size_t y0);

  void DrawLineH(std::size_t y, std::size_t x0, std::size_t x1);

  // Allows read/write access to individual bits of the image as if they were
  // boolean values.
  class Pixel {
   public:
    operator bool() const;
    void operator=(bool value);

   private:
    friend class OledBuffer;
    Pixel(std::uint8_t& block, std::uint8_t offset);

    std::uint8_t& block_;
    std::uint8_t offset_;
  };

 private:
  std::uint8_t* const data_;
  const std::size_t width_;
  const std::size_t height_;
};
