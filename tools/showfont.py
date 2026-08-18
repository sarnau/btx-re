#!/usr/bin/env python3
"""Render the ROM character generators as ASCII art.

    python3 tools/showfont.py wide   0x41 0x49
    python3 tools/showfont.py narrow 0x20 0x30

The wide font stores each row little-endian (first byte = right half), which is
why a naive big-endian read splits every glyph in two.
"""

from __future__ import annotations

import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
ROM = ROOT.parent / "C64 BTX Decoder" / "c64_btx_decoder_ii.bin"
BASE = 0x8000
ROWS = 10

FONTS = {
    # name: (address, bytes per row, glyph count, row byte order swapped)
    "wide": (0x8000, 2, 384, True),
    "narrow": (0x9E00, 1, 104, False),
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
