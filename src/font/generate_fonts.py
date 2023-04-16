from math import ceil
import numpy as np
from bdfparser import Font


def serialize_font(bdf_path):
    with bdf_path.open("rt") as f:
        font = Font(f)
    width = font.headers["fbbx"]
    height = font.headers["fbby"]
    yield bytes([width, height])
    # Each image is split up into blocks where each block is a byte of 8
    # contiguous pixels vertically, where the LSB is the top-most pixel.

    # Pad char vertically so each column is a multiple of 8 pixels.
    blocks_per_column = ceil(height / 8)
    storage_height = 8 * blocks_per_column

    padding = (
        (0, storage_height - height),
        (0, 0),
    )
    as_2d_array = 2
    for code in range(ord(" "), ord("~") + 1):
        pixels = np.array(font.draw(chr(code)).todata(as_2d_array))
        pixels = np.pad(pixels, padding)
        # Pack bits over the row axis. Resulting shape will be
        # (blocks_per_column, width).
        packed = np.packbits(pixels, axis=0, bitorder="little")
        yield packed.tobytes()


def main():
    from argparse import ArgumentParser
    from pathlib import Path

    parser = ArgumentParser()
    parser.add_argument(
        "bdf_paths",
        type=Path,
        nargs="+",
        help="Path to Glyph Bitmap Distribution Format files.",
    )
    parser.add_argument(
        "--output", "-o", type=Path, help="Path to write block blob to."
    )

    args = parser.parse_args()
    num_fonts = len(args.bdf_paths)

    with args.output.open("wb") as output:
        output.write(bytes([num_fonts]))
        for bdf_path in args.bdf_paths:
            for blob in serialize_font(bdf_path):
                output.write(blob)


if __name__ == "__main__":
    main()
