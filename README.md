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
| `c64_blocks` | 6502 blocks: ROM range plus the address each runs at on the C64. |
| `c64_symbols` | Names for the BTX register window as the C64 sees it. |

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

The package was originally `dis6801`, renamed to `dis65xx` once the embedded
6502 payload was identified and the toolchain grew a second instruction set.
The design spec and plan under `docs/superpowers/` were updated to match, so
their paths point at real files.

## Two instruction sets, two sources

The image holds a C64-side 6502 program as well as the 68B01 firmware. They are
kept in **separate sources**, because the two live in different address spaces
and mixing them made every operand a special case.

Each 6502 block is assembled on its own at the address it really runs at, and
the result is linked back into the 6801 listing as binary:

    out/c64_bootstrap.asm   ORG $8000   ->  c64_bootstrap.bin    121 bytes
    out/c64_payload.asm     ORG $0FFE   ->  c64_payload.bin     7523 bytes
    out/btx_decoder_ii.asm  ORG $8000   ->  BINCLUDEs both      32768 bytes

The payload starts at the PRG load-address word and is ORGed two bytes early so
its data still lands at $1000. `c64_payload.bin` is therefore a genuine C64
`.prg` - load address followed by data - which is exactly what the 6801 streams
and what the bootstrap's loader consumes. The two blocks are contiguous, so the
6801 listing carries nothing but the two BINCLUDEs.

PETSCII text uses a `petscii` region kind. The C64 lowercase charset puts
uppercase at `$C1-$DA` and lowercase at `$41-$5A`, so the emitter brackets such
a region with `CHARSET` and writes the text as ordinary letters - which is how
`C2 49 4C 44 ...` becomes `"Bildschirmtext"` rather than a hex dump.

Hardware the C64 sees is symbolic too: `btxFifoWr`, `btxFifoRd`, `btxXferEn`
and friends are declared under `c64_symbols` in the sidecar and pair with the
6801-side names at `$6000`.

`build.py` checks each block against the ROM bytes it came from before the 6801
listing pulls it in, so a failure says which block broke rather than giving one
offset in a 32 KB image.

The payoff is that neither listing carries the other's quirks. The 6801 listing
is pure 6801 with a single `CPU` directive. The 6502 sources sit at their real
addresses, so every target is an ordinary label - no EQU indirection, no
cross-reference comments - and C64 ROM entry points are named
(`dis65xx/c64kernal.py`) rather than given `L<addr>` forms that would collide
with 6801 listing labels.

Which ROM range is which block, and where it runs, is declared in the sidecar
under `c64_blocks`.

## Labels

Every branch and jump target is a label. Targets the sidecar has named keep that
name; the rest get `L<address>`. Nothing branches to a bare address in either
listing.

## Why not Ghidra

Ghidra ships no 6800/6801 processor module. Its `MC6800/data/languages/6800.ldefs`
defines only 6809 and H6309, and lists `6800`/`6801`/`6803`/`6808` merely as
IDA-Pro import aliases mapped onto the **6809** module — a different opcode map.
Ghidra would produce plausible-looking but wrong disassembly.

## Layout

    dis65xx/opcodes.py     MC6801 opcode table (pure data)
    dis65xx/decode.py      bytes -> one 6801 instruction
    dis65xx/encode.py      one 6801 instruction -> bytes
    dis65xx/opcodes6502.py NMOS 6502 opcode table (151 documented opcodes)
    dis65xx/codec6502.py   6502 decode and encode
    dis65xx/trace.py     recursive-descent code discovery
    dis65xx/emit.py      code/data map + sidecar -> listing text
    dis65xx/asm.py       listing text -> bytes (the round-trip verifier)
    dis65xx/sidecar.py   analysis metadata loading
    tools/report.py      unresolved jumps and coverage gaps
    build.py             regenerate, assemble, compare, report

`conftest.py` at the repository root is load-bearing: its presence is what puts
the repository root on `sys.path` for pytest. Deleting it breaks every test.

See `docs/superpowers/specs/2026-08-17-c64-btx-decoder-re-design.md` for the design.

## Documents

- `docs/btx-decoder-ii-architecture.md` - how the module works
- `docs/cv30113-revision-diff.md` - the two ROM revisions compared
- `docs/superpowers/specs/` - the original design spec and plan

## Current state

Bootstrap trace from the reset vector and the seven interrupt stubs classifies
25% of the image, with 12 unresolved computed jumps and no decode failures.
`$8000`–`$9FFF` is character-generator bitmap data; the `$D374`–`$D3E3` cluster
of computed jumps looks like table dispatch and is the obvious next target.

## Independent verification

`tools/check_asl.sh` reassembles the generated listing with asl (Macro Assembler
AS) and compares against the ROM. `dis65xx/asm.py` shares an opcode table with the
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
