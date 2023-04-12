#pragma once

#include <cstdint>
#include <span>
#include <vector>

class OledBuffer {
 public:
  class Pixel;

  OledBuffer(std::size_t width, std::size_t height);

  Pixel operator()(std::size_t x, std::size_t y);

  std::size_t Width() const { return width_; }
  std::size_t Height() const { return height_; }

  void Clear();

  std::span<const std::uint8_t> PackedData() const { return data_; }

  class Pixel {
   public:
    void operator=(bool value);

   private:
    friend class OledBuffer;
    Pixel(std::uint8_t& block, std::uint8_t offset);

    std::uint8_t& block_;
    std::uint8_t offset_;
  };

 private:
  const std::size_t width_;
  const std::size_t height_;
  std::vector<std::uint8_t> data_;
};
