#include "font.h"

#include <pico/platform.h>

#include <iostream>
#include <vector>

extern const std::uint8_t _binary_font_data_bin_start[];

namespace {
constexpr std::size_t kNumChars = 95;

std::vector<Font> LoadFonts() {
  const std::uint8_t* font_data = _binary_font_data_bin_start;
  std::cout << "Loading font data " << std::endl;

  const auto num_fonts = *font_data++;
  std::cout << "Font count: " << int(num_fonts) << std::endl;

  std::size_t total_data_size = 0;
  std::vector<Font> fonts(num_fonts);
  for (Font& font : fonts) {
    font.width = *font_data++;
    font.height = *font_data++;
    // Round height up to multiple of 8 for pointer arithmetic.
    const std::size_t blocks_per_column = (font.height + 7) / 8;
    const std::size_t storage_height = blocks_per_column * 8;
    font.bytes_per_char = font.width * storage_height / 8;
    const std::size_t data_size = font.bytes_per_char * kNumChars;
    font.data = std::span(font_data, data_size);
    total_data_size += data_size;
    font_data += data_size;

    std::cout << "Loaded " << font.width << "x" << font.height << " font."
              << std::endl;
  }
  std::cout << "All fonts loaded: " << total_data_size << " bytes" << std::endl;
  return fonts;
}
}  // namespace

std::span<const Font> AllFonts() {
  static std::vector<Font> fonts = LoadFonts();
  return fonts;
}

const Font& FontForHeight(std::size_t height) {
  const auto fonts = AllFonts();
  for (const Font& font : fonts) {
    if (font.height == height) {
      return font;
    }
  }
  panic("Could not find font with height %d", height);
}
