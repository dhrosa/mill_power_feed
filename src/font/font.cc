#include "font.h"

const Font& FontForHeight(std::size_t height) {
  const auto fonts = AllFonts();
  for (const Font& font : fonts) {
    if (font.height == height) {
      return font;
    }
  }
  return fonts[0];
}
