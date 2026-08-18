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

# asl is not packaged anywhere and its source URL is unversioned and was
# unreachable for minutes at a time, so a copy lives in third_party/. Build it
# there on demand rather than making the cross-check depend on the network.
if ! command -v "$ASL" >/dev/null 2>&1; then
    BUILT="$ROOT/third_party/build/asl-current"
    if [ ! -x "$BUILT/asl" ]; then
        TARBALL="$ROOT/third_party/asl-current.tar.gz"
        if [ ! -f "$TARBALL" ]; then
            echo "asl not found and $TARBALL is missing." >&2
            echo "Set ASL=/path/to/asl. Skipping cross-check." >&2
            exit 77
        fi
        case "$(uname -s)/$(uname -m)" in
            Darwin/arm64) DEF=Makefile.def-arm-osx ;;
            Darwin/*)     DEF=Makefile.def-x86_64-osx ;;
            *)            DEF=Makefile.def-x86_64-unknown-linux ;;
        esac
        echo "building asl from third_party (one time, ~1 min)..." >&2
        mkdir -p "$ROOT/third_party/build"
        tar xzf "$TARBALL" -C "$ROOT/third_party/build"
        ( cd "$BUILT" \
          && cp "Makefile.def-samples/$DEF" Makefile.def \
          && make -j"$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)" ) >/dev/null 2>&1
        if [ ! -x "$BUILT/asl" ]; then
            echo "asl build failed; see $BUILT. Skipping cross-check." >&2
            exit 77
        fi
    fi
    ASL="$BUILT/asl"
    P2BIN="$BUILT/p2bin"
    export AS_MSGPATH="${AS_MSGPATH:-$BUILT}"
    PATH="$BUILT:$PATH"
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
