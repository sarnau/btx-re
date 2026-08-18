# Commodore BTX Decoder II — architecture

How the Commodore Bildschirmtext Decoder II works, reconstructed from the
firmware. Every claim here is traceable to the annotated listing; the few
inferences that are not proven from code are marked as such.

Companion material: `sidecar/decoder_ii.toml` holds the annotations,
`out/btx_decoder_ii.asm` the generated listing, and
`docs/cv30113-revision-diff.md` the comparison against the other ROM revision.

---

## 1. What the module is

A cartridge that turns a Commodore 64 into a terminal for **Bildschirmtext**,
the German videotex service. It is not a passive ROM: the cartridge carries its
own processor, its own screen memory and its own video output, and drives the
modem itself. The C64 supplies the keyboard, the display and mass storage.

Two processors are involved, and the single 32 KB ROM serves both:

| | |
|---|---|
| **MC68B01** | on the cartridge. Runs the decoder firmware: modem, CEPT interpretation, screen generation |
| **C64's 6502** | runs a terminal application that the cartridge feeds to it at power-on |

The firmware also implements PRESTEL, ANTIOPE and a plain ASCII terminal mode,
named in the mode strings at `$EF67`.

---

## 2. The 68B01 side

### CPU and vectors

MC6801/MC68B01, big-endian, ROM mapped at `$8000`–`$FFFF`. The eight hardware
vectors sit at `$FFF0`–`$FFFF` in the 6801 layout. Seven of them point into a
block of identical five-byte stubs:

```
$F129:  LDX  softVecSci        ; FE 00 F0
        JMP  0,X               ; 6E 00
```

so every interrupt is revectorable at runtime through internal RAM at
`$00F0`–`$00FB`. `$F147` is a bare `RTI`, used as the null handler that reset
installs into the vectors it does not need. That the soft vectors live in
`$00F0`–`$00FB` establishes that **internal RAM is enabled**.

### Memory map

```
$0000-$001F   6801 internal register file
$0000-$03FF   stack (LDS #$0400 at reset; it grows down)
$0080-$00FF   6801 internal RAM; interrupt soft vectors at $00F0-$00FB
$0400-$07FF   external RAM, ~110 distinct variables
$1B00-$1B2D   display and cursor state
$4000-$5BC7   display memory, four planes
$6009-$6011   C64 dual-port control and status
$6080-$608F   16-byte FIFO shared with the C64
$61F9-$61FD   write-only control registers
$8000-$FFFF   this ROM
```

### Reset

`$B200` sets the stack, configures ports 1 and 2, installs the interrupt soft
vectors, then initialises the subsystems and hands the C64 its software.

---

## 3. The C64 relationship

### The cartridge announces itself

`$B32D` in ROM is what the C64 sees at `$8000`, proved by the autostart header
sitting there:

```
$B32D  13 80              cold start vector  $8013
$B32F  72 FE              warm start vector  $FE72
$B331  C3 C2 CD 38 30     "CBM80"
```

`$8013` resolves to ROM `$B340`, exactly where the cold-start code begins.

### The C64 pulls its own software across

The cold-start routine runs from cartridge ROM. It initialises the KERNAL and
VIC, then fetches a two-byte destination pointer and copies a byte stream into
it:

```
JSR $FDA3 / $FD90 / $FD15 / $FF5B    KERNAL init, restore vectors, CINT
LDX #$00 / STX $D016                 VIC setup
JSR $804D / STA $61                  destination low byte
JSR $804D / STA $62                  destination high byte
LDY #$00
loop: JSR $804D / BCS done / STA ($61),Y
      INY / BNE loop / INC $62 / JMP loop
```

The two bytes it reads first are the `$00 $10` at ROM `$B3A6` — **load address
`$1000`**. The payload proper is `$B3A8`–`$D108`, 7521 bytes, landing at
`$1000`–`$2D60`, and it opens with a 61-entry `JMP` dispatch table at `$1000`.

Because the payload runs at a different address than it is stored, its absolute
operands name runtime locations with no position in this ROM.

### The dual-port interface

The same hardware window appears at different addresses on each side — the 6801
at `$6000`, the C64 at `$8000`, with matching low offsets:

| 6801 | C64 | Role |
|---|---|---|
| `$6080-$608F` | — | 16-byte ring FIFO |
| `$6009` | `$8009` | write index (6801 → C64) |
| `$600A` | `$800A` | read index (C64 → 6801) |
| `$600B` | `$800B` | transfer enable |
| `$600C` | — | status |
| `$6010` | — | completion flag |

`sendPayloadToC64` (`$B2E2`) drives it:

```
LDAA #$FF / STAA c64XferEn
TST c64Status (twice)              wait for the C64
loop: LDX $0406 / INX
      CPX #$D109 / BEQ done        payload ends at $D108
      LDAA 0,X
      LDAB c64FifoWr
      LDX #c64Fifo / ABX           slot = $6080 + index
      INCB / ANDB #$0F             16-slot ring
      CMPB c64FifoRd (twice)       spin while full
      STAA 0,X / STAB c64FifoWr
      BRA loop
```

Both sides guard against metastability identically: read a shared index twice
and act only when the two reads agree — `CMPB c64FifoRd` here, `LDA $8009 /
CMP $8009 / BNE` in the C64 bootstrap.

---

## 4. The line: 1200 down, 75 up

BTX is asymmetric, and the two directions use entirely different hardware.

**Receive — the 6801 SCI.** `RMCR = $0C` sets CC1:CC0 = 11, an **external
clock**: the modem supplies the 1200-baud bit clock, so the firmware never
picks a divisor. `TRCSR = $08` enables the receiver only; `$F5B4` later raises
it to `$18` for RE + RIE, switching from polled to interrupt-driven.

**Transmit — a software UART.** `TE` is never set and `TDR` is never written
anywhere in the image by any addressing mode; the SCI transmitter is unused.
`txBitTick` (`$F14C`) runs on the timer output compare instead:

- each tick reloads OCR with `+$3415` — 13333 cycles, **75.002 Hz** at a 1 MHz E clock
- each bit is written to `TCSR` bit 0, which is **OLVL**, so the compare
  hardware places the level on P21 at the exact compare instant
- 9-bit frame shifted out of `txShift`, fed from a ring buffer at `$04EE`

Letting the timer drive the pin rather than the ISR keeps the bit timing free
of interrupt-latency jitter — which matters when the CPU is simultaneously
decoding a 1200-baud stream.

---

## 5. CEPT interpretation

### Dispatch

Five tables, all reached through one idiom:

```
PSHA / ANDA #mask / ASLA / TAB / LDX #table / ABX / LDX 0,X / PULA / JMP 0,X
```

| Table | Entries | Decodes |
|---|---|---|
| `$D109` `ctrlTableC0` | 32 | C0 controls, `A < $20` |
| `$D149` `ctrlTableC1a` | 32 | C1 `$80-$9F`, **serial** attribute set |
| `$D189` `ctrlTableC1b` | 32 | C1 `$80-$9F`, **parallel** attribute set |
| `$D1C9` `escTable` | 96 | ESC (`$1B`) sequences, `$20-$7F` |
| `$D289` `csiTable` | 96 | CSI (`$9B`) sequences, `$20-$7F` |

`$0497` chooses between the two C1 sets. `escSelectC1Set` (ESC `$22` — CEPT's
serial/parallel mode select) is what writes it, which is how the two tables are
identified as the serial and parallel sets.

The tables are sparse by design: `ctrlIgnored` fills 14 of 32 C0 slots and
`seqIgnored` 85 of 96 in *both* the ESC and CSI tables, so only 11 escape
sequences each are actually implemented.

### Character sets and shifts

The escape assignments are ISO 2022: `$28`–`$2B` designate G0–G3, `$6E`/`$6F`
are the locking shifts LS2/LS3, `$7C`–`$7E` the single shifts. The distinction
shows in the code — `escLS2` sets both the current and the saved G-set
(`$049D`, `$04A2`), while `ctlSS2` sets only `$049F`.

### A second interpreter

`$F9FA` is a 32-entry C0 table with its own dispatcher at `$FBA9`, serving the
ASCII terminal mode. Nine handlers cover its 32 slots.

---

## 6. Display

### Character generators

`$8000`–`$9DFF` holds **four 96-glyph sets** of 20 bytes each:

| Address | Set |
|---|---|
| `$8000` | G0 Latin alphanumerics |
| `$8780` | non-spacing diacriticals and symbols (G2) |
| `$8F00` | 2×3 block mosaics (G1) |
| `$9680` | line drawing and diagonals |

Each glyph is 10 rows of 2 bytes, and **rows are stored little-endian** — the
first byte of a row is its *right* half. Read in the 6801's usual big-endian
order every glyph splits across two characters, which is what makes this data
look like noise at first sight.

Ink occupies columns 4–15 of the swapped word: the 12-pixel CEPT cell. Bit 15
is not ink but a per-glyph flag marking **contiguous graphics** — glyphs drawn
edge to edge with no inter-character gap. *(Inference: the firmware never reads
the font, so the video hardware consumes this bit. Across 349 live glyphs the
flag agrees with set membership 345 times, and all four exceptions are
individually sensible — G0 `$7C` is the full-height vertical bar, mosaic `$4E`
and `$5E` are the only mosaics leaving rows 0 and 9 blank, and set3 `$20` is
the space.)*

`$9E00`–`$A20F` holds a second font: 104 glyphs, 8 pixels wide, one byte per
row, no byte-order surprise.

### Display memory

Four planes, addressed through line tables at `$FC45`–`$FD0C` — 25 rows of 40
columns, the CEPT page plus a status line:

| Plane | Per cell | Role |
|---|---|---|
| `$4000` | 1 byte | render byte derived from attribute bits at `$E860` |
| `$4400` | 4 bytes | attributes, written only by the C1 handlers |
| `$5400` | 1 byte | **character codes** |
| `$5800` | 1 byte | combining-accent overlay |

`$E9A5` reloads all four row pointers whenever the cursor row changes.

What separates attributes from content is decisive: `$04B1`–`$04B4` are never
assigned a raw byte — only ever bit-manipulated by the C1 handlers — whereas
the incoming character lands in `$04B5` and only that value reaches `$5400`.

### Combining accents

The `$5800` plane is not a duplicate character plane. When a character comes
from G2 and its code lies in `$40`–`$4F` — where CEPT puts the non-spacing
diacriticals — `$E972` stores it there and returns **without advancing the
cursor**, which is what makes it combining rather than spacing. The next
character picks up an attribute bit from `accentPending` and goes to `$5400`
normally. The firmware never reads `$5800`, so the video hardware composites
the mark over the base glyph.

### Cursor and scrolling

`cursorCol` (`$1B1F`) wraps against 40 and 39; `cursorRow` (`$1B1E`) is clamped
between `scrollTop` and `scrollBottom` with a ceiling in `$1B00`. `$EC16` is
the wrap logic; `$EC8D` and `$ECA1` are the bounded row moves, each falling
through to scroll at the window edge.

Double-width rendering uses the bit-doubling table at `$AEFC`, whose entries
are the index with each bit duplicated.

---

## 7. ROM map

```
$8000-$877F   G0 character set            96 glyphs x 20 bytes
$8780-$8EFF   G2 accents and symbols      96 glyphs
$8F00-$967F   G1 mosaics                  96 glyphs
$9680-$9DFF   line drawing                96 glyphs
$9E00-$A20F   8x10 font                  104 glyphs x 10 bytes
$A210-$AEEF   firmware
$AEF0-$AEFB   font base addresses
$AEFC-$B1FF   bit-doubling table
$B200-$B32C   reset and payload transfer
$B32D-$B33F   C64 cartridge header ("CBM80")
$B340-$B3A5   C64 cold-start loader        executed at C64 $8000
$B3A6-$B3A7   payload load address ($1000)
$B3A8-$D108   C64 6502 terminal application, runs at $1000-$2D60
$D109-$D348   CEPT dispatch tables
$D419-$D460   format-parameter tables
$E000-$EF66   firmware
$EF67-$EFCA   terminal mode names
$EFF5-$F080   status messages and version string
$F9FA-$FA39   ASCII-mode C0 dispatch table
$FC45-$FD0C   screen line-address tables
$FD21-$FFEF   unused ($FF fill, 719 bytes)
$FFF0-$FFFF   hardware vectors
```

100% of the image is accounted for: 13663 bytes of code, 19105 of typed data.
43 bytes in four fragments are unreachable dead code — no reference anywhere in
the image, no branch target, and each preceded by an instruction that ends flow.

---

## 8. Revisions

Two ROM images exist. Both carry a version string at `$EFF5`:

| Image | Version |
|---|---|
| `c64_btx_decoder_ii.bin` | Decodersoftware **V3.3** |
| `CV30113 C375-B1-1 (EX)` | Decodersoftware **V3.1** |

Only two things changed between them: the `$04AF` guard was removed from the
`CAN` handler, and three umlaut glyphs went from lowercase to uppercase. The
entire C64 payload is byte-for-byte identical. See
`docs/cv30113-revision-diff.md`.

---

## 9. Not established

- **Font bit 15.** The contiguous-graphics reading is an inference from
  distribution; the firmware never reads the font, so nothing in the ROM tests
  the bit. A schematic would settle it.
- **`$1B22`–`$1B2D`.** Part of the display-state block, individually unnamed.
- **`$61F9`–`$61FD`.** Write-only control registers whose function is unknown.
- **The `$D419` no-match path.** Reachable, but harmless: two of its three
  outputs are read only for bit 7 and the third is clamped. Documented in the
  sidecar rather than treated as a bug.
- **The C64 payload's internals.** Its 61-entry jump table is labelled and its
  variable/text split is established, but only three routines carry evidenced
  names; the other 57 are structural. Tracing entries transitively is not
  discriminating - nearly all reach a shared input loop - so naming them means
  reading each one, a task the size of the 6801 side.
