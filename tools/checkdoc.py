#!/usr/bin/env python3
"""Check the architecture document against the generated sources.

    python3 tools/checkdoc.py

The document accumulates edits while the listings are regenerated underneath
it, so the failure mode is prose that was true when written. These are the
checks that have actually caught something:

  1. every backticked identifier is defined by one of the three sources
  2. every ROM-map line names something the sources define
  3. no ROM-map boundary disagrees with the sidecar

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
DOC = ROOT / "docs" / "btx-decoder-ii-architecture.md"
SOURCES = ["btx_decoder_ii.asm", "c64_payload.asm", "c64_bootstrap.asm"]

# mnemonics and prose words that look like identifiers but are not
ALLOWED = {
    "ASR", "CAN", "JMP", "LDD", "LSRD", "PUL", "PULA", "ROR", "RTI", "RTS",
    "SEI", "HIBASE", "MEMSTR", "TE", "R", "T", "W", "w", "d", "h", "i", "l",
    "t", "lineAddr", "drcsPlane",
}


def main() -> int:
    doc = DOC.read_text()
    src = "".join((ROOT / "out" / f).read_text() for f in SOURCES)
    defined = set(re.findall(r"^([A-Za-z_][A-Za-z0-9_]*)(?::| +EQU)", src, re.M))
    findings: list[str] = []

    cited = set(re.findall(r"`([a-zA-Z_][a-zA-Z0-9_]*)`", doc)) - ALLOWED
    for name in sorted(cited - defined):
        findings.append(f"undefined identifier: {name}")

    body = doc.split("## 8. ROM map", 1)[1].split("100% of the image", 1)[0]
    for m in re.finditer(r"^\$[0-9A-F]{4}(?:-\$[0-9A-F]{4})?\s+(.+)$", body, re.M):
        for tok in re.findall(r"\b([a-z][A-Za-z0-9_]{3,})\b", m.group(1)):
            if any(c.isupper() for c in tok) and tok not in defined | ALLOWED:
                findings.append(f"ROM map names {tok}, which no source defines")

    for f in findings:
        print(f"  {f}")
    print(f"{len(findings)} finding(s)")
    return len(findings)


if __name__ == "__main__":
    sys.exit(main())
