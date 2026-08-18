# btx-re — Commodore BTX Decoder II firmware reverse engineering

Reconstructs a commented, byte-identical assembly listing of the Commodore
Bildschirmtext Decoder II ROM (`c64_btx_decoder_ii.bin`, MC6801, 32 KB at
`$8000`–`$FFFF`).

## Usage

    python3 build.py            regenerate out/btx_decoder_ii.asm and verify
    python3 build.py --check    verify without writing
    python3 build.py --report   show unresolved jumps and unreached regions
    python3 -m pytest           run the test suite

## How to work on this

`out/btx_decoder_ii.asm` is **generated**. Never edit it. All analysis goes into
`sidecar/decoder_ii.toml`:

| Section | Purpose |
|---|---|
| `entry_points` | Code entry addresses. Add the target of every computed jump you resolve. |
| `labels` | Address → symbol name. Used for branch targets and extended operands. |
| `line_comments` | Address → trailing `;` comment on that instruction. |
| `block_comments` | Address → multi-line banner above that address. |
| `symbols` | Address → name for hardware registers and RAM locations. |
| `regions` | Typed data ranges: `bytes`, `words`, `string`, `ptr_table`, `chargen`, `code6502`. |

`entry_points` must appear **above** the `[meta]` header. TOML assigns bare keys
written after a table header to that table, so putting it below silently makes it
`meta.entry_points` and yields an empty disassembly. The loader rejects that case
explicitly.

After every change, run `python3 build.py`. It must print
`OK         reassembles byte-identical`. If it does not, stop and fix that before
doing anything else — a broken round-trip means the listing no longer describes
the ROM.

Use `--report` to find the next thing to work on: unresolved computed jumps are
where code coverage is being lost, and the largest unreached regions are either
undiscovered code or data that needs a region entry.

## Two instruction sets

The image holds a C64-side 6502 program as well as the 68B01 firmware, so the
listing carries both. A `code6502` region is linear-disassembled as 6502 and
bracketed with `CPU 6502` / `CPU 6801`, which asl and `dis6801/asm.py` both
honour. Undecodable bytes inside such a region fall back to `FCB`.

The payload is stored at one address and runs at another, so its absolute
operands are runtime addresses with no position in this ROM. They are emitted
verbatim rather than relabelled - that is what keeps the round-trip exact. The
block comment at `$B32D` records the mapping.

## Why not Ghidra

Ghidra ships no 6800/6801 processor module. Its `MC6800/data/languages/6800.ldefs`
defines only 6809 and H6309, and lists `6800`/`6801`/`6803`/`6808` merely as
IDA-Pro import aliases mapped onto the **6809** module — a different opcode map.
Ghidra would produce plausible-looking but wrong disassembly.

## Layout

    dis6801/opcodes.py     MC6801 opcode table (pure data)
    dis6801/decode.py      bytes -> one 6801 instruction
    dis6801/encode.py      one 6801 instruction -> bytes
    dis6801/opcodes6502.py NMOS 6502 opcode table (151 documented opcodes)
    dis6801/codec6502.py   6502 decode and encode
    dis6801/trace.py     recursive-descent code discovery
    dis6801/emit.py      code/data map + sidecar -> listing text
    dis6801/asm.py       listing text -> bytes (the round-trip verifier)
    dis6801/sidecar.py   analysis metadata loading
    tools/report.py      unresolved jumps and coverage gaps
    build.py             regenerate, assemble, compare, report

`conftest.py` at the repository root is load-bearing: its presence is what puts
the repository root on `sys.path` for pytest. Deleting it breaks every test.

See `docs/superpowers/specs/2026-08-17-c64-btx-decoder-re-design.md` for the design.

## Current state

Bootstrap trace from the reset vector and the seven interrupt stubs classifies
25% of the image, with 12 unresolved computed jumps and no decode failures.
`$8000`–`$9FFF` is character-generator bitmap data; the `$D374`–`$D3E3` cluster
of computed jumps looks like table dispatch and is the obvious next target.

## Independent verification

`tools/check_asl.sh` reassembles the generated listing with asl (Macro Assembler
AS) and compares against the ROM. `dis6801/asm.py` shares an opcode table with the
disassembler, so a wrong table entry round-trips cleanly through it; asl does not
share that table and will catch such an error.

asl is not packaged in Homebrew and must be built from source:
<http://john.ccac.rwth-aachen.de:8000/as/>

    curl -O http://john.ccac.rwth-aachen.de:8000/ftp/as/source/c_version/asl-current.tar.gz
    tar xzf asl-current.tar.gz && cd asl-current
    cp Makefile.def-samples/Makefile.def-arm-osx Makefile.def   # or -x86_64-osx
    make

Then:

    ASL=/path/to/asl P2BIN=/path/to/p2bin AS_MSGPATH=/path/to/asl-current \
        ./tools/check_asl.sh

Exit code 77 means asl was not found and the check was skipped.

Status: **passing** — asl assembles the listing with 0 errors and 0 warnings and
its output is byte-identical to the ROM, independently confirming the opcode table.
