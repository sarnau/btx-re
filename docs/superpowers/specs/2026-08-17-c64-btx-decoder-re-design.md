# Commodore BTX Decoder II — ROM Reverse Engineering

**Date:** 2026-08-17
**Status:** Approved

## Goal

Produce a fully commented, byte-identical re-assemblable source listing of the
Commodore Bildschirmtext Decoder II firmware, plus an architecture document
explaining how the module works.

"Done" means: `out/btx_decoder_ii.asm` assembles to a file that is byte-for-byte
identical to `c64_btx_decoder_ii.bin`, every reachable routine is named and
commented, every data region is typed, and the hardware memory map is documented.

## Target

**Primary:** `../C64 BTX Decoder/c64_btx_decoder_ii.bin`
32 KB, SHA-256 `2799910767fdb7067fb91f72aef691bf692a8f7a2f2b26acb71d3d1ca3f68926`

**Secondary:** `../C64 BTX Decoder/c64_BTX_decoder_CV30113 C375-B1-1 (EX).BIN`
32 KB, SHA-256 `2baed45ee9df97db6f8d2517db686d7f7f8598a1985c5886bb592dd7a5b1386a`

The two are revisions of the same firmware: 8549 bytes differ and the vector
targets are offset by 8, so one build has extra content earlier in the image.
The secondary ROM is out of scope until the primary is understood; the same tool
then runs over it to produce a revision diff.

### CPU

Motorola MC6801/MC68B01. Established from the vector table at the end of the
image — eight big-endian vectors at `$FFF0`–`$FFFF`, which is the 6801 layout:

| Address | Vector | decoder_ii | CV30113 |
|---|---|---|---|
| `$FFF0` | SCI (serial) | `$F129` | `$F131` |
| `$FFF2` | Timer overflow (TOF) | `$F12E` | `$F136` |
| `$FFF4` | Output compare (OCF) | `$F133` | `$F13B` |
| `$FFF6` | Input capture (ICF) | `$F138` | `$F140` |
| `$FFF8` | IRQ1 | `$F13D` | `$F145` |
| `$FFFA` | SWI | `$F142` | `$F14A` |
| `$FFFC` | NMI | `$F147` | `$F14F` |
| `$FFFE` | RESET | `$B200` | `$B200` |

The ROM therefore occupies `$8000`–`$FFFF`.

Seven of the eight point into a contiguous 5-byte-stride block. Hand-decoding it
confirms a soft-vector dispatch table — six identical stubs of the form

```
$F129:  FE 00 F0    LDX  $00F0        ; SCI
        6E 00       JMP  0,X
```

indirecting through internal RAM at `$00F0` (SCI), `$00F2` (TOF), `$00F4` (OCF),
`$00F6` (ICF), `$00F8` (IRQ1), `$00FA` (SWI). The NMI vector at `$F147` is a bare
`RTI` — also used as the null handler installed into the unused soft vectors.

This settles one of the open questions below: **internal RAM is enabled**, since
the soft vectors live at `$00F0`–`$00FB`.

### Why not Ghidra

Ghidra ships no 6800/6801 SLEIGH module. Its `MC6800/data/languages/6800.ldefs`
defines only the 6809 and H6309 languages, and lists `6800`, `6801`, `6803`,
`6808` merely as **IDA-Pro import aliases mapped onto the 6809 module**. The 6809
opcode map differs substantially from the 6801, so Ghidra would silently produce
plausible-looking but wrong disassembly. This is very likely why the existing
`BTX_6502.gpr` project is misnamed and why prior work moved to IDA.

We therefore build our own tooling.

## Hardware context

Datasheets present in the parent folder identify the board:

| Part | Role |
|---|---|
| MC68B01 | CPU (2 MHz-rated 6801) |
| MC68HCB34 | Bus/port expander |
| M41464 | 64K×4 DRAM — video framebuffer |
| MC1377 | RGB → PAL/NTSC composite encoder |
| 74HCT139 | Dual 2→4 address decoder |
| 74HC221 | Dual monostable — video timing |
| 74HCU04 | Unbuffered inverter — oscillator |

Reference specs: `ets_300072e01p.pdf` (CEPT / ETS 300 072 videotex),
`Commodore_BTX_Decoder_II.pdf` (user manual), `MC6801RM_AD2` (CPU reference),
`d65005c198.pdf`.

### Memory map (to be established, not assumed)

Known-by-architecture regions:

- `$0000`–`$001F` — 6801 internal registers (ports, timer, SCI)
- `$0080`–`$00FF` — 6801 internal RAM, *if* enabled by the operating mode
- `$F800`–`$FFFF` — 6801 internal ROM, *if* enabled

The 6801 operating mode is selected by P2 pins at reset and is not visible in the
image. Because the vectors live in the external ROM, internal ROM is almost
certainly disabled. Internal RAM **is** enabled — the interrupt soft vectors sit
at `$00F0`–`$00FB`.

Established from the reset entry at `$B200`:

```
$B200:  8E 04 00    LDS  #$0400       ; stack top → external RAM below $0400
        86 10       LDAA #$10
        97 02       STAA $02          ; Port 1 data
        7F 00 00    CLR  $0000        ; Port 1 DDR = all inputs
        86 11       LDAA #$11
        97 01       STAA $01          ; Port 2 DDR
        97 03       STAA $03          ; Port 2 data
        CC F1 47    LDD  #$F147       ; the bare RTI
        FD 00 F2    STD  $00F2        ; TOF  → null handler
        FD 00 F6    STD  $00F6        ; ICF  → null handler
        FD 00 FA    STD  $00FA        ; SWI  → null handler
        CC F1 4C    LDD  #$F14C
        FD ...                        ; installs the real timer/SCI handlers
```

So external RAM lies below `$0400`, and Port 1 is used as an input port while
Port 2 is configured with DDR `$11`. External DRAM, the 68HCB34 registers, and
the C64 interface latches are located by observing access patterns and
cross-checking against the 74HCT139 decode.

Establishing this map is the first analysis milestone; everything else depends on
it.

## Approach

**Tool + sidecar metadata, with byte-identity as a permanent invariant.**

The `.asm` listing is *always generated*, never hand-edited. All analysis lives in
a separate sidecar file. A code/data boundary discovery is therefore a one-line
sidecar change rather than a manual restructure of a 32 KB listing.

Rejected alternatives:

- *One-shot disassembly, then hand-edit the `.asm`.* Less tooling, but every
  boundary discovery becomes manual surgery and the round-trip check degrades
  into something you remember to run rather than something enforced.
- *Emulator-assisted tracing from the start.* Most thorough on computed jumps,
  but a large upfront cost before learning anything about the firmware. Held in
  reserve for when static tracing stalls.

## Components

```
btx-re/
  dis6801/
    opcodes.py     complete MC6801 table — all addressing modes, 6801-only ops
    trace.py       recursive-descent code/data discovery from entry points
    emit.py        asl-syntax listing generator
    asm.py         minimal 6801 assembler — the round-trip verifier
  sidecar/
    decoder_ii.toml
  tests/
  build.py         regenerate + verify, one command
  out/btx_decoder_ii.asm
  docs/btx-decoder-ii-architecture.md
```

Each module has one job and a narrow interface:

**`opcodes.py`** — pure data plus encode/decode helpers. No I/O, no state. Covers
all six 6801 addressing modes (inherent, immediate 8, immediate 16, direct,
indexed, extended, relative) and the 6801-only additions over the 6800:
`LDD`/`STD`/`ADDD`/`SUBD`, `LSRD`/`ASLD`, `PSHX`/`PULX`, `ABX`, `MUL`, `JSR direct`,
`CPX` flag behaviour.

**`trace.py`** — takes ROM bytes plus entry points, returns a code/data map.
Recursive descent: follow calls and branches, stop at returns and unconditional
transfers, record every byte's classification. Knows nothing about output syntax.

**`emit.py`** — takes the map plus the sidecar, returns listing text. Pure
formatting; no analysis decisions.

**`asm.py`** — takes listing text, returns bytes. Shares only the opcode table
with the disassembler.

**`sidecar/decoder_ii.toml`** — the analysis. Schema:

| Key | Contents |
|---|---|
| `entry_points` | extra code entries found manually (jump-table targets, etc.) |
| `labels` | address → symbol name |
| `comments` | address → line or block comment |
| `regions` | address range → `code` \| `bytes` \| `words` \| `string` \| `ptr_table` \| `chargen` |
| `functions` | address → name, description, calling-convention notes |
| `symbols` | hardware register names for internal registers and external I/O |

## Data flow

```
ROM bytes + sidecar
      ↓ trace
  code/data map
      ↓ emit
btx_decoder_ii.asm
      ↓ asm.py
    bytes
      ↓ compare
   ROM bytes        ← mismatch is a hard failure
```

## Verification

**The invariant.** Round-trip byte-identity runs on every regeneration, starting
from the bootstrap state where only the eight vectors are code and all remaining
32 KB is `FCB`. It is green before any analysis begins and is never allowed to
break. A red round-trip blocks all further work.

**Opcode table correctness.** Byte-identity alone is insufficient: a wrong table
entry round-trips cleanly through a disassembler and assembler that share it.
Two independent checks —

1. An encode → decode → encode fixpoint test across every opcode × addressing-mode
   combination.
2. The table cross-checked against `MC6801RM_AD2_MC6801_Reference_Manual_May84.pdf`.

**Independent assembler.** The listing targets **asl** (Alfred Arnold's Macro
Assembler AS) syntax, which has strong 6801 support. asl is not available in
Homebrew and needs a source build; `asm.py` is the zero-dependency verifier that
makes progress independent of that, and asl becomes a third opinion once built.

**Coverage reporting.** Every run reports % of the image classified as code, as
typed data, and as unknown — the progress metric for the whole project.

## Work sequence

1. **Bootstrap.** Opcode table, assembler, disassembler, emitter, `build.py`.
   Only the eight vectors are code; everything else `FCB`. Round-trip green.
2. **Grow code coverage.** Trace from the vectors and `$B200`. Resolve jump
   tables — the main source of missed code in dispatch-heavy firmware.
3. **Memory map.** Name hardware registers by usage; determine the 6801 operating
   mode and internal RAM/ROM state; locate DRAM, 68HCB34 and the C64 latches.
4. **Subsystems**, in this order:
   - C64 ↔ decoder interface (host handshake, shared latches)
   - CEPT page decoding and display (ETS 300 072 datastream, character sets,
     DRCS, attributes, framebuffer rendering)
   - Modem / SCI datacomms (DBT-03 control, 1200/75 baud, connection setup)
5. **Architecture document** written alongside, not at the end.

## Risks and open questions

| Risk | Mitigation |
|---|---|
| asl unavailable without a source build | `asm.py` is the primary verifier; asl is a cross-check |
| 6801 operating mode unknown until code is read | Treated as a milestone-3 deliverable, not an assumption |
| Computed/indirect jumps defeat static tracing | Emulator-assisted tracing held in reserve |
| Self-modifying code | Detected as a round-trip or coverage anomaly; escalates to the emulator |
| Shared opcode table hides errors | Fixpoint test plus manual cross-check against the reference manual |

## Out of scope

- The C64-side 6502 cartridge code (separate image, separate effort)
- Hardware recreation, schematic reconstruction, FPGA work
- A full module emulator
- The `CV30113` revision, until the primary ROM is understood
