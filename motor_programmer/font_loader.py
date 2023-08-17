import os
from adafruit_bitmap_font import bitmap_font


def load_fonts():
    fonts = {}
    for fname in os.listdir("fonts/"):
        if not fname.endswith(".bdf"):
            continue
        path = f"fonts/{fname}"
        font = bitmap_font.load_font(path)
        height = font.get_bounding_box()[1]
        fonts[height] = LazyFont(font, fname)
    return fonts


class LazyFont:
    def __init__(self, font, name):
        self._font = font
        self._name = name
        self._loaded = False

    def get_bounding_box(self):
        return self._font.get_bounding_box()

    def get_glyph(self, codepoint):
        if not self._loaded:
            print(f"Loading glyphs for {self._name}")
            self._font.load_glyphs(range(32, 127))
            self._loaded = True
            print(f"{self._name} loaded.")
        return self._font.get_glyph(codepoint)
