#pragma once

#include <cstdint>
#include <span>

struct Font {
  std::size_t width;
  std::size_t height;
  std::span<const std::uint8_t> data;
};

std::span<const Font> AllFonts();

const Font& FontForHeight(std::size_t height);
