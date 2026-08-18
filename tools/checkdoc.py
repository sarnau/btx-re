#!/usr/bin/env python3
"""Check the architecture document against the generated sources.

    python3 tools/checkdoc.py

The document accumulates edits while the listings are regenerated underneath
it, so the failure mode is prose that was true when written. These are the
checks that have actually caught something:

  1. every backticked identifier is defined by one of the three sources
  2. every ROM-map line names something the sources define
  3. the ROM map partitions $8000-$FFFF with no gap or overlap
  4. a row claiming "N glyphs x M bytes" spans exactly N*M

An identifier the sources no longer define is the usual finding: a rename
lands in the listing and the prose keeps the old word.

Exit code is the number of findings, so it can gate a commit.
"""
from __future__ import annotations

import pathlib
import re
import sys
import tomllib

ROOT = pathlib.Path(__file__).resolve().parent.parent
ARCH = ROOT / "docs" / "btx-decoder-ii-architecture.md"
# Every prose file that cites names from the listings. The README drifted for
# nine turns citing btxFifoWr and btxFifoRd after they were renamed, which is
# exactly what this catches.
DOCS = [ARCH, ROOT / "README.md", ROOT / "docs" / "cv30113-revision-diff.md"]
SOURCES = ["btx_decoder_ii.asm", "c64_payload.asm", "c64_bootstrap.asm"]

def _mnemonics() -> set[str]:
    """Every mnemonic of both instruction sets, plus the register letters.

    Taken from the opcode tables rather than listed here, so a mnemonic the
    document happens to mention never has to be added by hand."""
    sys.path.insert(0, str(ROOT))
    from dis65xx import opcodes, opcodes6502
    names = {m for m, _ in opcodes.TABLE.values()}
    names |= {m for m, _ in opcodes6502.TABLE.values()}
    # PUL and PSH are written bare in prose; A/B/D/X/S are registers
    names |= {"PUL", "PSH", "A", "B", "D", "X", "S"}
    return names


def _vocabulary() -> set[str]:
    """Names the prose may use that are not listing labels.

    Taken from the definitions - the sidecar's own top-level keys, the region
    kinds the loader accepts - so a schema key that is misspelled in the prose
    is still caught, while a real one never has to be listed here."""
    sys.path.insert(0, str(ROOT))
    from dis65xx.sidecar import REGION_KINDS
    sidecar = tomllib.load(open(ROOT / "sidecar" / "decoder_ii.toml", "rb"))
    return set(sidecar) | set(REGION_KINDS) | {
        # assembler directives and the package name
        "CPU", "ORG", "EQU", "END", "FCB", "FCC", "FDB", "DW", "BINCLUDE",
        "CHARSET", "dis65xx", "dis6801",
    }


# prose words that look like identifiers but are not
ALLOWED = {
    "CAN", "HIBASE", "MEMSTR", "TE", "R", "T", "W", "w", "d", "h", "i", "l",
    "t", "lineAddr", "drcsPlane",
}


def main() -> int:
    arch = ARCH.read_text()
    src = "".join((ROOT / "out" / f).read_text() for f in SOURCES)
    defined = set(re.findall(r"^([A-Za-z_][A-Za-z0-9_]*)(?::| +EQU)", src, re.M))
    findings: list[str] = []

    allowed = ALLOWED | _mnemonics() | _vocabulary()
    for path in DOCS:
        text = path.read_text()
        cited = set(re.findall(r"`([a-zA-Z_][a-zA-Z0-9_]*)`", text)) - allowed
        for name in sorted(cited - defined):
            findings.append(f"{path.name}: undefined identifier: {name}")

    body = arch.split("## 8. ROM map", 1)[1].split("The payload's own layout", 1)[0]
    rows = []
    for m in re.finditer(r"^\$([0-9A-F]{4})(?:-\$([0-9A-F]{4}))?\s+(.+)$", body, re.M):
        lo = int(m.group(1), 16)
        rows.append((lo, int(m.group(2), 16) if m.group(2) else lo, m.group(3)))
    rows.sort()
    want = 0x8000
    for lo, hi, name in rows:
        if lo != want:
            findings.append(
                f"ROM map {'gap' if lo > want else 'overlap'} at ${want:04X}"
                f"-${lo - 1:04X} before {name.split()[0]}")
        want = hi + 1
    if rows and want != 0x10000:
        findings.append(f"ROM map stops at ${want - 1:04X}, not $FFFF")
    for lo, hi, name in rows:
        g = re.search(r"(\d+) glyphs x (\d+) bytes", name)
        if g and int(g.group(1)) * int(g.group(2)) != hi - lo + 1:
            findings.append(f"{name.split()[0]}: claims "
                            f"{g.group(1)}x{g.group(2)}, spans {hi - lo + 1}")

    for m in re.finditer(r"^\$[0-9A-F]{4}(?:-\$[0-9A-F]{4})?\s+(.+)$", body, re.M):
        for tok in re.findall(r"\b([a-z][A-Za-z0-9_]{3,})\b", m.group(1)):
            if any(c.isupper() for c in tok) and tok not in defined | allowed:
                findings.append(f"ROM map names {tok}, which no source defines")

    for f in findings:
        print(f"  {f}")
    print(f"{len(findings)} finding(s)")
    return len(findings)


if __name__ == "__main__":
    sys.exit(main())
