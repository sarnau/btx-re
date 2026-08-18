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

from dis65xx.asm import assemble
from dis65xx.emit import emit
from dis65xx.emit6502 import emit_block
from dis65xx.sidecar import load_sidecar
from dis65xx.trace import trace
from tools.report import format_report

ROOT = pathlib.Path(__file__).resolve().parent
SIDECAR_PATH = ROOT / "sidecar" / "decoder_ii.toml"
OUT_PATH = ROOT / "out" / "btx_decoder_ii.asm"
OUT_DIR = ROOT / "out"


@dataclasses.dataclass
class BuildResult:
    rom: bytes
    listing: str
    assembled: bytes
    code_bytes: int
    unknown_bytes: int
    typed_bytes: int
    kind: list[str]
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

    # The 6502 blocks are separate programs. Assemble each at the address it
    # really runs at, check it against the ROM bytes it came from, and leave the
    # binary for the 6801 listing to BINCLUDE.
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    c64_sources: dict[str, str] = {}
    for block in sidecar.c64_blocks:
        src = emit_block(rom, sidecar.base, block, sidecar)
        c64_sources[block.name] = src
        _, blob = assemble(src)
        expected = rom[block.start - sidecar.base:block.end - sidecar.base]
        if blob != expected:
            raise ValueError(f"{block.name}: assembles to {len(blob)} bytes that do "
                             f"not match ROM ${block.start:04X}-${block.end - 1:04X}")
        (OUT_DIR / f"c64_{block.name}.bin").write_bytes(blob)
        if write:
            (OUT_DIR / f"c64_{block.name}.asm").write_text(src)

    listing = emit(rom, result, sidecar)
    _, assembled = assemble(listing, include_dir=OUT_DIR)

    if write:
        OUT_PATH.write_text(listing)

    cov = result.coverage()
    # Bytes that are not code but sit in a typed sidecar region are documented,
    # not unknown; counting them as unknown understates real progress.
    typed = sum(
        1 for a in range(sidecar.base, sidecar.base + len(rom))
        if result.kind[a - sidecar.base] == "unknown" and sidecar.region_at(a) is not None
    )
    return BuildResult(
        rom=rom,
        listing=listing,
        assembled=assembled,
        code_bytes=cov["code"],
        unknown_bytes=cov["unknown"],
        typed_bytes=typed,
        kind=result.kind,
        unresolved=result.unresolved,
        bad_opcodes=result.bad_opcodes,
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="verify without writing")
    parser.add_argument("--report", action="store_true",
                        help="list unresolved jumps and unreached regions")
    args = parser.parse_args()

    result = run(write=not args.check)
    total = len(result.rom)

    print(f"ROM        {total} bytes, sha256 {hashlib.sha256(result.rom).hexdigest()[:16]}")
    unclassified = result.unknown_bytes - result.typed_bytes
    print(f"coverage   {result.code_bytes} code / {result.typed_bytes} typed data / "
          f"{unclassified} unclassified")
    print(f"           {100.0 * (result.code_bytes + result.typed_bytes) / total:.1f}% "
          f"of the image is accounted for")
    print(f"unresolved {len(result.unresolved)} computed jumps")
    print(f"bad opcode {len(result.bad_opcodes)} sites")

    if args.report:
        print()
        print(format_report(base=0x8000, kind=result.kind,
                            unresolved=result.unresolved,
                            bad_opcodes=result.bad_opcodes))
        print()

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
