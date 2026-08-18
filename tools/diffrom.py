#!/usr/bin/env python3
"""Compare the two BTX Decoder II ROM revisions.

    python3 tools/diffrom.py

Reports the identical runs, locates any shifted region, and prints the glyphs
that differ. See docs/cv30113-revision-diff.md for the conclusions.
"""

from __future__ import annotations

import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
ROMS = ROOT
A = ROMS / "c64_btx_decoder_ii.bin"
B = ROMS / "c64_BTX_decoder_CV30113 C375-B1-1 (EX).BIN"
BASE = 0x8000
FONT_SETS = {"G0": 0x8000, "accents": 0x8780, "mosaic": 0x8F00, "set3": 0x9680}


def identical_runs(a: bytes, b: bytes, minimum: int = 64) -> list[tuple[int, int]]:
    runs: list[tuple[int, int]] = []
    start = None
    for i in range(len(a)):
        if a[i] == b[i]:
            if start is None:
                start = i
        elif start is not None:
            if i - start >= minimum:
                runs.append((start, i - start))
            start = None
    if start is not None and len(a) - start >= minimum:
        runs.append((start, len(a) - start))
    return runs


def best_shift(a: bytes, b: bytes, lo: int, hi: int) -> tuple[int, float]:
    best = (0, 0.0)
    for shift in range(-16, 17):
        n = ok = 0
        for i in range(lo, hi):
            j = i + shift
            if 0 <= j < len(b):
                n += 1
                ok += a[i] == b[j]
        if n and ok / n > best[1]:
            best = (shift, ok / n)
    return best


def changed_glyphs(a: bytes, b: bytes) -> list[tuple[str, int, int]]:
    out = []
    for name, base in FONT_SETS.items():
        for g in range(96):
            o = base - BASE + g * 20
            if a[o:o + 20] != b[o:o + 20]:
                out.append((name, 0x20 + g, base + g * 20))
    return out


def main() -> int:
    a, b = A.read_bytes(), B.read_bytes()
    diff = sum(1 for i in range(len(a)) if a[i] != b[i])
    print(f"{A.name}\n{B.name}")
    print(f"\n{diff} of {len(a)} bytes differ ({100 * diff / len(a):.1f}%)")

    runs = identical_runs(a, b)
    print(f"\nidentical runs >= 64 bytes: {len(runs)}, "
          f"{sum(n for _, n in runs)} bytes ({100 * sum(n for _, n in runs) / len(a):.1f}%)")
    for s, n in runs:
        print(f"  ${s + BASE:04X}-${s + n - 1 + BASE:04X}  {n}")

    shift, frac = best_shift(a, b, 0xDB80 - BASE, 0xFFF0 - BASE)
    print(f"\ntail alignment: shift {shift:+d} matches {100 * frac:.1f}%")

    glyphs = changed_glyphs(a, b)
    print(f"\nchanged glyphs: {len(glyphs)}")
    for name, code, addr in glyphs:
        print(f"  {name} chr${code:02X} at ${addr:04X}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
