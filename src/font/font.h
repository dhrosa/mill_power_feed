#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

struct Font {
  std::ptrdiff_t width;
  std::ptrdiff_t height;
  std::ptrdiff_t bytes_per_char;
  std::span<const std::uint8_t> data;

  // Pointer to start of data for the given character, which must be in the
  // range [' ', '~'].
  const std::uint8_t* operator[](char c) const;
};

inline const std::uint8_t* Font::operator[](char c) const {
  return &data[(c - ' ') * bytes_per_char];
}

std::span<const Font> AllFonts();

const Font& FontForHeight(std::size_t height);
