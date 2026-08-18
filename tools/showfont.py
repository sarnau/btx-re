#!/usr/bin/env python3
"""Render the ROM character generators as ASCII art.

    python3 tools/showfont.py g0      0x41 0x49
    python3 tools/showfont.py mosaic  0x21 0x29
    python3 tools/showfont.py narrow  0x20 0x30

The $8000 area holds four 96-glyph sets. Their rows are stored little-endian
(first byte = right half), which is why a naive big-endian read splits every
glyph across two characters. Ink sits in columns 4-15 (the 12-pixel CEPT cell);
the top bits are flags, not pixels.
"""

from __future__ import annotations

import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
ROM = ROOT / "c64_btx_decoder_ii.bin"
BASE = 0x8000
ROWS = 10

FONTS = {
    # name: (address, bytes per row, glyph count, row byte order swapped)
    "g0": (0x8000, 2, 96, True),        # Latin alphanumerics
    "accents": (0x8780, 2, 96, True),   # non-spacing diacriticals (CEPT G2)
    "mosaic": (0x8F00, 2, 96, True),    # 2x3 block mosaics (CEPT G1)
    "set3": (0x9680, 2, 96, True),      # line drawing / diagonals
    "narrow": (0x9E00, 1, 104, False),  # 8px font
}


def glyph_rows(rom: bytes, font: str, code: int) -> list[str]:
    addr, wbytes, count, swap = FONTS[font]
    index = code - 0x20
    if not 0 <= index < count:
        raise SystemExit(f"chr${code:02X} is outside {font} font (0..{count - 1})")
    off = addr - BASE + index * wbytes * ROWS
    out = []
    for r in range(ROWS):
        raw = rom[off + r * wbytes: off + (r + 1) * wbytes]
        if swap:
            raw = raw[::-1]
        out.append("".join(f"{b:08b}" for b in raw).replace("0", "·").replace("1", "#"))
    return out


def main() -> int:
    if len(sys.argv) != 4:
        raise SystemExit(__doc__)
    font, first, last = sys.argv[1], int(sys.argv[2], 0), int(sys.argv[3], 0)
    if font not in FONTS:
        raise SystemExit(f"font must be one of {', '.join(FONTS)}")
    rom = ROM.read_bytes()

    codes = list(range(first, last))
    for start in range(0, len(codes), 8):
        group = codes[start:start + 8]
        rendered = [glyph_rows(rom, font, c) for c in group]
        width = len(rendered[0][0])
        print("  " + "  ".join(f"${c:02X}{'':{max(0, width - 3)}}" for c in group))
        for r in range(ROWS):
            print("  " + "  ".join(g[r] for g in rendered))
        print()
    return 0


if __name__ == "__main__":
    sys.exit(main())
