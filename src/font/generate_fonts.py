from pathlib import Path
from collections import namedtuple
from math import ceil
from itertools import product
import numpy as np

Font = namedtuple("Font", ("char_data", "width", "height"))

char_count = 95


def load_font_data(image_path):
    from PIL import Image

    with Image.open(image_path) as image:
        # Convert to bitmap
        image.convert("1")
        image_width = image.width
        height = image.height
        # Numpy array of 0's and 1's
        data = np.array(image.getdata()).reshape(height, image_width)

    width = image_width // char_count
    # Array indexed by (char_num, row, col)
    char_data = np.array(np.hsplit(data, char_count))

    return Font(char_data, width, height)


def font_to_blocks(font):
    width = font.width
    # Each image is split up into blocks where each block is a byte of 8 contiguous
    # pixels vertically, where the LSB is the top-most pixel.

    # Pad char vertically so each column is a multiple of 8 pixels.
    blocks_per_column = ceil(font.height / 8)
    storage_height = 8 * blocks_per_column
    padding = (
        (0, 0),
        (0, storage_height - font.height),
        (0, 0),
    )
    pixels = np.pad(font.char_data, padding)

    # Pack bits over the row axis. Resulting shape will be (char_count,
    # blocks_per_column, width).
    packed = np.packbits(pixels, axis=1, bitorder="little")
    return packed.tobytes()


def serialize_font(font):
    return bytes([font.width, font.height]) + font_to_blocks(font)


def main():
    from argparse import ArgumentParser
    from sys import stdout

    parser = ArgumentParser()
    parser.add_argument(
        "image_paths",
        type=Path,
        nargs="+",
        help="Path to image of monospace font consisting of the horizontal concatenation of the ASCII range [' ', '~'].",
    )
    parser.add_argument(
        "--output", "-o", type=Path, help="Path to write block blob to."
    )

    args = parser.parse_args()
    num_fonts = len(args.image_paths)

    with args.output.open("wb") as output:
        output.write(bytes([num_fonts]))
        for path in args.image_paths:
            font = load_font_data(path)
            output.write(serialize_font(font))


if __name__ == "__main__":
    main()
