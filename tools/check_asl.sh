#!/usr/bin/env bash
# Independent verification with asl (Macro Assembler AS).
#
# Mirrors the real build order: each 6502 block is assembled on its own at the
# address it runs at, then the 6801 listing pulls the results in with BINCLUDE.
# dis65xx/asm.py shares an opcode table with the disassembler; asl does not, so
# this catches table errors that round-trip cleanly.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ASL="${ASL:-asl}"
P2BIN="${P2BIN:-p2bin}"
OUT="$ROOT/out"
ROM="$ROOT/../C64 BTX Decoder/c64_btx_decoder_ii.bin"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

if ! command -v "$ASL" >/dev/null 2>&1; then
    echo "asl not found; set ASL=/path/to/asl. Skipping cross-check." >&2
    exit 77
fi

cd "$OUT"

# 1. the 6502 blocks, each at its own load address. The range is derived from
# the source's ORG and the block's size, so it cannot drift out of step with
# the sidecar the way a hardcoded one did.
for name in bootstrap payload; do
    org=$(sed -n 's/^ *ORG *\$\([0-9A-F]*\).*/\1/p' "c64_$name.asm" | head -1)
    size=$(wc -c < "c64_$name.bin")
    end=$(printf '%04X' $(( 0x$org + size - 1 )))
    "$ASL" -q -o "$WORK/$name.p" "c64_$name.asm"
    "$P2BIN" "$WORK/$name.p" "$WORK/c64_$name.bin" -r "\$$org-\$$end" >/dev/null
    if ! cmp -s "$WORK/c64_$name.bin" "c64_$name.bin"; then
        echo "FAIL  asl and dis65xx disagree on the $name block" >&2
        exit 1
    fi
done

# 2. the 6801 listing, which BINCLUDEs those binaries
"$ASL" -q -o "$WORK/rom.p" btx_decoder_ii.asm
"$P2BIN" "$WORK/rom.p" "$WORK/rom.bin" -r '$8000-$FFFF' >/dev/null

if cmp -s "$WORK/rom.bin" "$ROM"; then
    echo "OK  asl output is byte-identical to the ROM (6502 blocks assembled separately)"
else
    echo "FAIL  asl output differs from the ROM:" >&2
    cmp -l "$WORK/rom.bin" "$ROM" | head -20 >&2
    exit 1
fi
