#pragma once

#include <cstdint>
#include <span>
#include <vector>

class OledBuffer {
 public:
  OledBuffer(std::size_t width, std::size_t height);

  std::uint8_t& operator()(std::size_t x, std::size_t y);

  std::size_t Width() const { return width_; }
  std::size_t Height() const { return height_; }

  void Clear();

 private:
  const std::size_t width_;
  const std::size_t height_;
  std::vector<std::uint8_t> data_;
};

inline OledBuffer::OledBuffer(std::size_t width, std::size_t height)
    : width_(width), height_(height), data_(width_ * height_) {}

inline std::uint8_t& OledBuffer::operator()(std::size_t x, std::size_t y) {
  return data_[x + y * width_];
}

inline void OledBuffer::Clear() {
  for (auto& value : data_) {
    value = 0;
  }
}
