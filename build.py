#!/usr/bin/env python3
"""Regenerate the listing and verify it reassembles byte-identical to the ROM.

    python3 build.py          regenerate out/btx_decoder_ii.asm and verify
    python3 build.py --check  verify without writing

A mismatch is a hard failure. This invariant holds from the bootstrap state
(only the vectors traced, everything else FCB) and must never be allowed to break.
"""

from __future__ import annotations

import argparse
import dataclasses
import hashlib
import pathlib
import sys

from dis6801.asm import assemble
from dis6801.emit import emit
from dis6801.sidecar import load_sidecar
from dis6801.trace import trace

ROOT = pathlib.Path(__file__).resolve().parent
SIDECAR_PATH = ROOT / "sidecar" / "decoder_ii.toml"
OUT_PATH = ROOT / "out" / "btx_decoder_ii.asm"


@dataclasses.dataclass
class BuildResult:
    rom: bytes
    listing: str
    assembled: bytes
    code_bytes: int
    unknown_bytes: int
    unresolved: list[int]
    bad_opcodes: list[int]

    @property
    def ok(self) -> bool:
        return self.assembled == self.rom

    @property
    def first_mismatch(self) -> int | None:
        if self.ok:
            return None
        for i, (a, b) in enumerate(zip(self.rom, self.assembled)):
            if a != b:
                return i
        return min(len(self.rom), len(self.assembled))

    @property
    def coverage_pct(self) -> float:
        return 100.0 * self.code_bytes / len(self.rom)


def run(*, write: bool = True) -> BuildResult:
    sidecar = load_sidecar(SIDECAR_PATH)
    rom = (SIDECAR_PATH.parent.parent / sidecar.rom).resolve().read_bytes()

    result = trace(rom, base=sidecar.base, entry_points=sidecar.entry_points)
    listing = emit(rom, result, sidecar)
    _, assembled = assemble(listing)

    if write:
        OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
        OUT_PATH.write_text(listing)

    cov = result.coverage()
    return BuildResult(
        rom=rom,
        listing=listing,
        assembled=assembled,
        code_bytes=cov["code"],
        unknown_bytes=cov["unknown"],
        unresolved=result.unresolved,
        bad_opcodes=result.bad_opcodes,
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="verify without writing")
    args = parser.parse_args()

    result = run(write=not args.check)
    total = len(result.rom)

    print(f"ROM        {total} bytes, sha256 {hashlib.sha256(result.rom).hexdigest()[:16]}")
    print(f"coverage   {result.code_bytes} code / {result.unknown_bytes} unknown "
          f"({result.coverage_pct:.1f}% classified)")
    print(f"unresolved {len(result.unresolved)} computed jumps")
    print(f"bad opcode {len(result.bad_opcodes)} sites")

    if not result.ok:
        offset = result.first_mismatch
        print(f"FAIL       round-trip mismatch at offset ${offset:04X} "
              f"(address ${offset + 0x8000:04X})", file=sys.stderr)
        return 1

    print("OK         reassembles byte-identical")
    if not args.check:
        print(f"wrote      {OUT_PATH.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
