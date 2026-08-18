#!/usr/bin/env bash
# Independent verification: assemble the generated listing with asl and compare
# against the ROM. dis65xx/asm.py shares an opcode table with the disassembler;
# asl does not, so this catches table errors that round-trip cleanly.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ASL="${ASL:-asl}"
P2BIN="${P2BIN:-p2bin}"
LISTING="$ROOT/out/btx_decoder_ii.asm"
ROM="$ROOT/../C64 BTX Decoder/c64_btx_decoder_ii.bin"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

if ! command -v "$ASL" >/dev/null 2>&1; then
    echo "asl not found; set ASL=/path/to/asl. Skipping cross-check." >&2
    exit 77
fi

"$ASL" -cpu 6801 -L -o "$WORK/out.p" "$LISTING"
"$P2BIN" "$WORK/out.p" "$WORK/out.bin" -r '$8000-$FFFF'

if cmp -s "$WORK/out.bin" "$ROM"; then
    echo "OK  asl output is byte-identical to the ROM"
else
    echo "FAIL  asl output differs from the ROM:" >&2
    cmp -l "$WORK/out.bin" "$ROM" | head -20 >&2
    exit 1
fi
