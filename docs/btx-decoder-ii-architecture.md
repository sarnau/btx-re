# Commodore BTX Decoder II — architecture

How the Commodore Bildschirmtext Decoder II works, reconstructed from the
firmware. Every claim here is traceable to the annotated listing; the few
inferences that are not proven from code are marked as such.

Companion material: `sidecar/decoder_ii.toml` holds the annotations,
`out/btx_decoder_ii.asm` the generated 6801 listing, `out/c64_bootstrap.asm`
and `out/c64_payload.asm` the C64-side 6502 sources, and
`docs/cv30113-revision-diff.md` the comparison against the other ROM revision.

---

## 1. What the module is

A cartridge that turns a Commodore 64 into a terminal for **Bildschirmtext**,
the German videotex service. It is not a passive ROM: the cartridge carries its
own processor, its own screen memory and its own video output, and drives the
modem itself. The C64 supplies the keyboard, mass storage and a printer.

Two processors are involved, and the single 32 KB ROM serves both:

| | |
|---|---|
| **MC68B01** | on the cartridge. Runs the decoder firmware: modem, CEPT interpretation, screen generation |
| **C64's 6502** | runs a terminal application that the cartridge feeds to it at power-on |

The split is unusual and worth stating plainly: **the picture the user looks at
comes out of the cartridge, not the C64.** The manual tells them to move the
monitor cable over to the module, and the splash screen the C64 prints says so
in as many words — *"Bitte stecken Sie Ihren Monitor an das Btx-Modul an."* The
C64's own screen carries a reduced text-only rendering, offered as a fallback
for people with one monitor, and the menu item that toggles it is called
Screen.

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
$0400-$07FF   external RAM - every location named, see below
$0800-$1AFF   drcsStore, 96 redefinable characters of 48 bytes
$1B00-$1B2D   display and cursor state
$2000-$2FFF   rxBuffer, the 4 KB receive ring
$4000-$5BC7   display memory, four planes
$5C00-$5FFF   videoRam, the pixels the hardware scans
$6009-$6011   C64 interface control and status
$6020-$603F   32-byte ring, decoder to C64
$6040-$607F   64-byte ring, C64 to decoder
$6080-$608F   16-byte ring during payload transfer, cell mailbox afterwards
$6090         c64StatusMsg - the status message index, mirrored for the C64
$61F9-$61FD   write-only control registers
$8000-$FFFF   this ROM
```

External RAM divides into six groups, and naming them is what made the rest of
this document possible:

| Range | |
|---|---|
| `$0402`–`$0425` | DRCS reception — the row being assembled and its four plane buffers |
| `$0487`–`$04B9` | CEPT display state — colour, character sets, attributes, the status line |
| `$04BE`–`$04ED` | modem and host-link state |
| `$05EF`–`$05F5` | the receive ring's pointers |
| `$0616`–`$0627` | the ASCII terminal renderer |
| `$07B0`–`$07E3` | the glyph renderer |

Three bytes are reused for unrelated jobs in different routines and carry a
name per site rather than per address: `$048E` is a mask complement, a cell
offset and a fill value; `$0490` is a repeat count, a saved column and a fill
index; `$0488` and `$0489` each split two ways.

### Reset

`$B200` sets the stack, configures ports 1 and 2, installs the interrupt soft
vectors, then initialises the subsystems and hands the C64 its software.

---

## 3. Bringing the C64 up

### The cartridge announces itself

`$B32D` in ROM is what the C64 sees at `$8000`, proved by the autostart header
sitting there:

```
$B32D  13 80              cold start vector  $8013
$B32F  72 FE              warm start vector  $FE72, the KERNAL's NNMI20
$B331  C3 C2 CD 38 30     "CBM80"
```

`$8013` resolves to ROM `$B340`, exactly where `c64CartStart` begins.

### The C64 pulls its own software across

`c64CartStart` runs from cartridge ROM. It runs the C64's cold-start sequence,
then fetches a two-byte destination pointer and copies a byte stream into it:

```
JSR IOINIT / SIZE+8 / RESTOR / PCINT   KERNAL init
LDX #$00 / STX $D016                   VIC setup
JSR c64BootGetByte / STA c64Ptr        destination low byte
JSR c64BootGetByte / STA c64PtrHi      destination high byte
LDY #$00
c64CopyLoop:
  JSR c64BootGetByte / BCS c64StartPayload / STA (c64Ptr),Y
  INY / BNE c64CopyLoop / INC c64PtrHi / JMP c64CopyLoop
```

`SIZE+8` is an entry eight bytes into the KERNAL's memory sizing, at the tail
that sets `MEMSTR` to `$0800` and `HIBASE` to `$0400`. It skips the
top-of-memory scan, which would otherwise walk into the cartridge window. Both
the bootstrap and the payload's own cold start enter there.

The two bytes the loop reads first are the `$00 $10` at ROM `$B3A6` — **load
address `$1000`**. The payload proper is `$B3A8`–`$D108`, 7521 bytes, landing at
`$1000`–`$2D60`.

Handing over is the last thing `c64StartPayload` does:

```
LDA #$00 / STA btxStatus     bank the cartridge out
JMP (btxLoadLo)              jump through $8000
```

`$8000` is cartridge ROM while the cartridge is mapped, holding the cold-start
vector `$8013`. The loader has already written the fetched load address into
`$8000`/`$8001` — stores land in the RAM underneath the ROM — so once the
cartridge is banked out, the same indirect read returns `$1000`. That is why
the loader stores the load address twice: to `$61`/`$62` for its own copy loop
and to `$8000`/`$8001` for this jump.

### The dual-port interface

The same hardware window appears at different addresses on each side — the 6801
at `$6000`, the C64 at `$8000`, with matching low offsets. There are **three
rings**, and the boot one is not the one used afterwards:

| 6801 | C64 | Size | Direction |
|---|---|---|---|
| `$6020` | `btxRxFifo` `$8020` | 32 | decoder → C64, indices `$8009`/`$800A` |
| `$6040` | `btxTxFifo` `$8040` | 64 | C64 → decoder, indices `$800D`/`$800E` |
| `$6080` | `btxFifo00` `$8080` | 16 | payload transfer only |

The masks prove the sizes on both sides independently: the 6801 uses
`ANDB #$1F` at `$6020` and `CMPB #$40` at `$6040`, the C64 `CPX #$20` on
`btxRxFifo` and `CPX #$40` on `btxTxFifo`. `sendPayloadToC64` uses `ANDB #$0F`
over `$6080`, and `c64BootGetByte` `CPX #$10` — a third, smaller ring that
exists only during the transfer.

Once the payload is running, `$8080`–`$808B` is reused as a **mailbox**: the
decoder posts one display cell there and raises `$808B`, and the payload's IRQ
handler picks it up. `$8080` itself becomes the reduced-display toggle.

`sendPayloadToC64` (`$B2E2`) drives the boot transfer:

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
and act only when the two reads agree — `CMPB c64FifoRd` here, `LDA btxRxWr /
CMP btxRxWr / BNE` in the C64 bootstrap. The idiom recurs everywhere the two
processors share a byte, including the cell mailbox and the line counter.

---

## 4. The C64 terminal application

7521 bytes at `$1000`–`$2D60`. Everything it does is reached through a
**61-entry `JMP` table** at `$1000`–`$10B6` — no routine is called except
through its slot, so the table is the whole API surface. Entries are labelled
`vec<Name>` and their bodies `c64<Name>`; vectors 44 and 45 share one body under
two slots.

### The menu is what unlocks it

Vector 4 reads one letter and looks it up in `c64MenuKeys`. The two lines it
displays are messages 3 and 4 of the string table:

```
Load Capture Display Macro Xfer Screen
ASCII Btx Keybd Telesoft Edit Pause Quit
```

Each letter therefore names its handler, and the 31 German status records
confirm what each one does:

| Key | Vector | Handler | Status message |
|---|---|---|---|
| L | 15 | `c64MenuLoad` | "von Diskette: File?" |
| C | 20 | `c64MenuCapture` | "Capture-Modus ein - Ende: STOP-Taste" |
| D | 22 | `c64MenuDisplay` | "Capture-Puffer anzeigen" |
| M | 19 | `c64MenuMacro` | "Macro ausfuehren: Kennung?" |
| X | 23 | `c64MenuXfer` | "Drucker oder File" |
| S | 24 | `c64MenuScreen` | "C-64-Monitor zugeschaltet" |
| A | 12 | `c64MenuAscii` | — |
| B | 13 | `c64MenuBtx` | — |
| K | 26 | `c64MenuKeybd` | "Keyboard: deutsch oder ASCII?" |
| T | 57 | `c64MenuTelesoft` | "Telesoftware: File?" |
| E | 28 | `c64MenuEdit` | "Macro anlegen: Kennung?" |
| P | 27 | `c64MenuPause` | "Pause: wieviele Sekunden (1-9)?" |
| Q | 14 | `c64MenuQuit` | — |

The splash text agrees: it tells the user to reach the menu with `<F7>`, and
`c64MainLoop` opens it on PETSCII `$88`, which is F7.

### The other six layers

| Layer | Vectors | |
|---|---|---|
| transport | 6, 8, 7 | the two rings, plus a blocking read |
| keyboard | 53, 5, 3, 29 | GETIN or macro playback → QWERTZ fixup → CEPT |
| display | 31, 30, 40, 9 | the status-line overlay |
| rendering | 46, 43, 44, 55, 56, 25 | one cell → a character → screen, printer or cursor |
| disk | 16–18, 33, 38, 39, 48, 59, 60, 49 | device 8 channels 4 and 15; 49 is the device 4 printer |
| macro | 34–37, 47, 50–52 | `@:BTX-MAK-<id>` on channels 5 and 6 |

### Its state

Five flags decide how a keystroke is handled, and between them they explain
most of the branching:

| Flag | Set by | Effect |
|---|---|---|
| `c64GermanFlag` | Keybd | search `c64GermanKeys`; swap Y and Z |
| `c64AlphaFlag` | ASCII / Btx | put `c64AsciiKeys` first in the search chain |
| `c64CapFlag` | Capture | route incoming bytes into the buffer |
| `c64RecFlag` | Edit | echo every key to the macro file |
| `c64PlayFlag` | Macro | read keys from the macro file, not the keyboard |

`c64XlatKey` searches up to three tables in order — `c64AsciiKeys` (2-byte
records, ASCII mode only), `c64GermanKeys` (4-byte, umlauts composed with the
CEPT diaeresis `$19 $48` plus a base letter), then `c64CtrlKeys` (4-byte,
cursor and colour and function keys). A key expands to at most three CEPT
bytes.

The capture buffer runs from `c64BufStart` — which is where the startup page
happens to sit, so the first capture overwrites it — up to `$8000`, the
cartridge window. `c64CapEnd` records how far it is filled.

### Character-cell rendering

The decoder hands the C64 six bytes per cell: the character, a combining
accent, an attribute byte whose low three bits select the character set and
whose bit 3 marks an accent, another attribute byte carrying reverse and
conceal, and two more that the payload stores and never reads.
`c64CellToChar` turns those into one code; `c64CellToScreen` plots it and
`c64CellOut` sends it to the printer or a file, mapping the umlaut compositions
onto the printer's `$BB`–`$DD`.

Two paths deliver cells. Under the IRQ, `c64IrqPlotCell` takes one from the
mailbox whenever the decoder posts it. For a hardcopy, `c64MenuXfer` walks rows
`$11`–`$29` and asks for all 40 columns of each.

### What the C64 says back

Everything the C64 sends arrives through one byte. `fetchHostByte` at `$F1D8`
runs masked, returns at once if `pendingByte` is already occupied, and
otherwise pulls from `hostFifoGet` — the `$6040` end of the ring the C64 fills
through `c64SendByte`. `execHostByte` at `$F45B` is the consumer, and `CLR
pendingByte` is how it says it is done.

The commands are the `$10 xx` pairs the payload's menu handlers send, so the
two listings read against each other directly:

| Sent by | Bytes | Effect on the decoder |
|---|---|---|
| `c64MenuCapture` / `c64CaptureEnd` | `$10 $4D` / `$10 $6D` | `captureMode` — forward every received byte to the C64 |
| `c64MenuLoad` / `c64MenuDisplay` | `$10 $4C` / `$10 $6C` | `hostFeedMode` — the C64 is supplying the page |
| `c64MenuKeybd` | `$10 $4E` / `$10 $6E` | `germanFont` — which font the reduced display uses |
| `c64ShowMsgPtr` / `c64HideMsg` | `$10 $75` / `$10 $55` | `blinkOff` — stop the cursor blinking over the overlay |
| `c64ShowMsgPtr` / `c64HideMsg` | `$10 $5A` / `$10 $7A` | begin a status-line record, and put the default line back |

`captureMode` is the clearest of them: `TST captureMode / JSR $F979` in two
different loops, and `$F979` is the routine that pushes a byte into the
decoder-to-C64 ring. Capture is implemented by the decoder echoing its input,
not by the C64 listening in.

### What a status-line record is

`$10 $5A` sets `promptLeft` to `$2C` — 44 — and the bytes that follow are
counted down against it. The first four go somewhere different from the rest:

```
LF23F:  LDX     #$5300
        ABX
        LDAB    #$04
LF245:  STAA    $00,X
        ABX
        CPX     #$53A0
        BCS     LF245
```

`$5300` is `planeAttr`'s last row, and the loop writes one byte across all 40
cells of it. So the record's four-byte header is not metadata about the
string — **it is the status line's four attribute bytes**. The remaining 40 go
to `planeChar` at `$57C0`, the same row.

`resetStatusLine` at `$EED2` writes the defaults the same way — `$00`, `$C0`,
`defaultColour`, `$98 AND revealMask` — which is what `$10 $7A` restores.

That settles the header bytes. Across the 31 records only two combinations
occur:

| Byte 1 | Byte 2 | Records |
|---|---|---|
| `$C0` | `$01` | 30 — a message, colour 1 |
| `$80` | `$07` | 1 — `msgBlank`, colour 7 |

`msgBlank` is the record `c64ClearMsg` sends to wipe the line, and `$80`/`$07`
are exactly the values `clearPlanes` puts in attribute bytes 1 and 2 at
power-on. So the odd record out is the one that restores the defaults, and
byte 2 is the colour — the same `$01` and `$07` that host commands `,` and `.`
put into `defaultColour`.

The count byte follows from this too. A full-width record is `$2C`, four
attributes plus forty characters; the prompts are `$18`, four plus **twenty**,
so they overwrite only the left half of the line and leave columns 20–39 for
the user's typed reply to echo into.

---

## 5. The line: 1200 down, 75 up

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

### Above the bits

Received bytes land in `rxBuffer`, a 4 KB ring at `$2000` walked by `rxBufPut`
and `rxBufGet` through `rxBufRd`, `rxBufWr` and `rxBufMark`. The third pointer
is what makes it more than a queue: `modemToggle` decides whether `rxBufMark`
follows `rxBufWr`, so a block can be re-read after a failed check.

That check is `modemCrc` at `$F818`. `modemByte` goes in, `modemShift` carries
the running value, and eight rounds of `LSRD` with `EORA #$A0 / EORB #$01` on a
set bit is a **reflected CRC-16, polynomial `$8005`** — the one X.25 and the
DBT-03 datex link use.

Three 16-bit counters at `timerA`, `timerB` and `timerC` are decremented on
every timer interrupt by `timerHandlerAlt` and waited on by testing their high
byte for `$FF`. The connect sequence loads `timerA` with `$07D0` and later
`$84D0`, `timerB` with `$10AE`, and `timerC` with `$00C8` before each of its
short waits.

`connected` is set when the sequence succeeds, and it gates everything above —
`execHostByte`, the blink, the abort path.

### Saying so on screen

`statusMsg` holds a byte offset into `strStatusMsgs`, and `showStatusMsg` at
`$EFCB` prints the record there — parking S in `savedSP` and using `PUL` to
walk the text, the same trick the glyph fetch uses.

Every write to `statusMsg` is followed by the same write to `$6090`, which is
`c64StatusMsg` — the byte the C64 reads as `btxStatusMsg`. So the two
processors share the connection state through the message index itself, and
the C64's tests name themselves:

| Value | Record | Where the C64 tests it |
|---|---|---|
| `$28` | "Verbindung" | `c64GetKey`, before translating a carriage return |
| `$50` | "Abbruch" | `c64TelesoftByte`, to abandon a download |

`showModeName` at `$EF1C` does the same for `strModeNames`, and it is what
identifies the terminal-mode flags — it tests each in turn and loads S with
that record's address:

| Flag | Record |
|---|---|
| — | "CEPT Bildschirmtext " |
| `modePrestel` | "PRESTEL Videotex   " |
| `modeAscii` | "ASCII Terminal-Mode" |
| `modeAntiope` | "ANTIOPE            " |
| `modeHex` | "HEX-Tastatureingabe" |

---

## 6. CEPT interpretation

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
sequences each are actually implemented. Slot `$1B` of the C0 table is
`ctlEsc`, which is what feeds `escTable`; `$9B` reaches `csiTable` the same
way.

### Character sets and shifts

The escape assignments are ISO 2022: `$28`–`$2B` designate G0–G3, `$6E`/`$6F`
are the locking shifts LS2/LS3, `$7C`–`$7E` the single shifts. The distinction
shows in the code — `escLS2` sets both the current and the saved G-set
(`$049D`, `$04A2`), while `ctlSS2` sets only `$049F`.

The C64 side speaks the same protocol back: `c64XlatKey`'s tables emit CEPT
sequences directly, and the pages the payload sends — the startup page, the
"Makro-Verzeichnis" listing, the "C-64-Betrieb" notice — are CEPT records the
firmware then decodes through exactly these tables.

### The state a page builds up

`gsetG0`–`gsetG3` hold the four designations, `gsetGL` and `gsetGR` the two
halves currently in force, `gsetSS` a pending single shift and `gsetGLDefault`
what a single shift falls back to. Colour is two variables: `colourIndex`, three
bits, and `clutIndex`, two. `applyColour` at `$E9CE` shows how they combine —
`attr2` takes `(clutIndex << 3) | colourIndex` in its low five bits and `attr3`
takes `clutIndex` in its low two.

Attributes reach the planes through `setAttrSpan` at `$E4FF`, which takes four
parameters:

```
        A -> attrValue      the bits to set
        B -> attrMask       which bits they replace
        X -> attrSpanBit    the render-plane bit marking this span
             attrByteIndex  which of the cell's four attribute bytes
```

It finds the span by walking `planeRender` from the cursor until `attrSpanBit`
stops being set, and writes at `cursorCol * 4 + attrByteIndex`.

### The status line

The bottom row is not part of the page, and `enterStatusLine` at `$E6C6` is how
the firmware borrows it. It sets `inStatusLine`, copies nine variables into
`savedRow`, `savedCol`, `savedClut`, `savedParallel`, `savedGL`, `savedGR`,
`savedG0`, `savedG1` and `saved04A4`, resets the ones it needs neutral, and
moves the cursor to the last row. `leaveStatusLine` at `$E744` puts all nine
back.

`inStatusLine` is then tested by sixteen CEPT handlers, which is how the page's
own control codes are kept from disturbing the overlay — and it is the `$04AF`
guard the CV30113 revision still has on `CAN`.

### A second interpreter

`$F9FA` is a 32-entry C0 table with its own dispatcher at `$FBA9`, serving the
ASCII terminal mode. Nine handlers cover its 32 slots, and they are a plain
glass-tty set over `asciiCol` and `asciiRow` in an 80x24 space:

| Slot | Handler | |
|---|---|---|
| `$08` | `asciiBS` | column back one, overwrite with a space |
| `$09` | `asciiHT` | column to the next multiple of 8, wrapping into `asciiLF` |
| `$0A` | `asciiLF` | row down, scroll at 23 |
| `$0B` | `asciiVT` | `JMP asciiLF` |
| `$0C` | `asciiFF` | home and clear |
| `$0D` | `asciiCR` | column to 0 |
| `$12` | `asciiCurLeft` | column down one, floored |
| `$14` | `asciiCurUp` | row down one, floored |

The other 24 slots are `asciiIgnored`, a bare `RTS`. There is no cursor-right —
but one of the four dead fragments, at `$FAE4`, sits immediately after
`asciiVT` and does exactly a column increment with wrap at 80. It is the
handler that was dropped from the table.

---

## 7. Display

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

| Plane | Base | Per cell | Row stride | Role |
|---|---|---|---|---|
| `planeRender` | `$4000` | 1 byte | 40 | render byte derived from attribute bits at `$E860` |
| `planeAttr` | `$4400` | 4 bytes | 160 | attributes, written only by the C1 handlers |
| `planeChar` | `$5400` | 1 byte | 40 | **character codes** |
| `planeAccent` | `$5800` | 1 byte | 40 | combining-accent overlay |

`$E9A5` reloads all four row pointers whenever the cursor row changes, from the
four tables at `$FC45`–`$FD0C`. Those strides are the clearest statement of the
layout anywhere in the image: three planes are one byte per cell across 40
columns, and `planeAttr` is four, which is exactly why its rows sit 160 apart.

What separates attributes from content is decisive: `$04B1`–`$04B4` are never
assigned a raw byte — only ever bit-manipulated by the C1 handlers — whereas
the incoming character lands in `$04B5` and only that value reaches `$5400`.

### DRCS

A CEPT page can define its own characters, and `$D4C4` is where they are
loaded. `drcsDefineChar` takes the character code and clears 102 bytes, which
is the layout stated exactly:

| | | |
|---|---|---|
| `$0420` | `drcsRowHi` `drcsRowLo` | the row being assembled, 16 bits |
| `$0422` | `drcsSel0`–`drcsSel3` | which planes this data belongs to |
| `$0426` `$043E` `$0456` `$046E` | `drcsPlane0`–`drcsPlane3` | 24 bytes each — 12 rows of 2 |

Four planes is what makes a DRCS cell colour: each contributes one bit per
pixel, and the selectors say which of them the incoming rows go into.

Each data byte carries six pixels (`ANDA #$3F`). Whether they are expanded
depends on the format `fmtLookup` resolved from the `$D419` tables:

- `drcsWide` — `LSRA / ROR / ASR`, eight times. `ROR` walks a bit out of A into
  bit 7 and `ASR` then copies it down one, so **every input bit lands twice**.
  Horizontal pixel doubling, done in the shift rather than through
  `bitDoubleTable`.
- `drcsTall` — `drcsStoreRow` is called twice for the same row: the vertical
  half of the same idea.
- `drcsFormat = 4` — no expansion; A goes straight into `drcsRowLo`.

The doubling lands exactly where the fonts are. Six input pixels become twelve,
and the 16-bit pair leaves its top four bits clear — the same shape as the ROM
character sets, whose ink occupies columns 4–15 of a 16-bit row. A DRCS
character and a built-in glyph are the same 12-pixel cell.

The planes at `$0426` are staging only. A finished definition lives in
`drcsStore` above `$0800`, 48 bytes per character, written by `drcsWriteRow`
through `drcsStorePtr` and cleared 96 characters at a time by
`drcsClearStore`. `drcsGlyphPtr` at `$A7E7` addresses it as
`(glyphCode & $7F - $20) * 48 + $07FF`.

The renderer decides which store to use from `drcsCell`, set when the current
G-set code is 5 — the ESC 2/8 2/0 designation. A DRCS cell is also **12 rows
rather than 10**, which is what `blitRows` carries into the blit loop.

### Combining accents

The `$5800` plane is not a duplicate character plane. When a character comes
from G2 and its code lies in `$40`–`$4F` — where CEPT puts the non-spacing
diacriticals — `$E972` stores it there and returns **without advancing the
cursor**, which is what makes it combining rather than spacing. The next
character picks up an attribute bit from `accentPending` and goes to `$5400`
normally. The firmware never reads `$5800`, so the video hardware composites
the mark over the base glyph.

The C64 side shows the same design from the other end: the cell it receives has
the accent in its own byte, and `c64CellOut` recombines accent plus base letter
into a single printer code.

### Turning cells into pixels

The four planes are not what the video hardware scans. `redrawScreen` at
`$A210` walks them and renders into `videoRam` through `videoPtr`, one
12-pixel cell at a time.

`videoRam` runs `$5C00`–`videoRamTop` at `$5FFF` — 1 KB, ending exactly where
the C64 interface window at `$6000` begins. Both clear loops step through it by
4 and stop on `CPX #videoRamTop`, which is what fixes the extent.

The unit of 4 bytes is a **scanline, not a cell**: `renderCol` never enters the
address at all, it goes to `PORT3` with `#$18` added, so the hardware latches
the column and the four bytes at `videoPtr` are that column's slice of one
line. 25 rows times `glyphRows` of 10 is 250 units, which is why the clear
fills the six-unit tail separately. Within a unit the renderer writes `$00,X`
and `$02,X` — plus `$04,X` and `$06,X` when `tallCell` is set — while the clear
touches only `$03,X`, from `rowFlags`. `redrawReq` at `$1B23` is how the CEPT handlers
ask for that: they set it, and the main loops test it against `$04AE` and call
in.

Fetching a glyph is done in an unusual way. `glyphPtr` is set to the font base
for the current set — through `fontBaseTable` — plus 20 times `glyphCode`,
then decremented, because the 6801's `PUL` pre-increments. The rows are then
read with `PULA`: **S is loaded from `glyphPtr`** and the real stack is parked
in `$00EC` under `SEI` for the duration.

```
        LDS     glyphPtr
loop:   PULA
        ORAA    $00,X
        STAA    $00,X
        INX
        ...
        LDS     >$00EC
```

A byte fetched, combined and stored in three instructions with no pointer
arithmetic — worth the trouble at 40 columns times `glyphRows` a frame. The
payload transfer at `$B2B4` uses the same trick to walk ROM, which is why it
saves S into what is otherwise `scrollEnd`.

Each pass ORs into `glyphBuf`, and `glyphPtr` is afterwards pointed at
`glyphBuf` itself, so a second pass composites over the first. That is how the
combining accent from `planeAccent` is merged onto its base character before
either reaches `videoRam`.

### Cursor and scrolling

`cursorCol` (`$1B1F`) wraps against 40 and 39; `cursorRow` (`$1B1E`) is clamped
between `scrollTop` and `scrollBottom` with a ceiling in `$1B00`. `$EC16` is
the wrap logic; `$EC8D` and `$ECA1` are the bounded row moves, each falling
through to scroll at the window edge.

Double-width rendering uses the bit-doubling table at `$AEFC`, whose entries
are the index with each bit duplicated.

---

## 8. ROM map

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
$B340-$B3A5   c64CartStart, executed at C64 $8000
$B3A6-$B3A7   payload load address ($1000)
$B3A8-$D108   C64 terminal application, runs at $1000-$2D60
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

The payload's own layout, in runtime addresses:

```
$1000-$10B6   61-entry JMP dispatch table
$10B7-$10BF   nine bytes of alignment padding
$10C0-$10CD   seven pointer slots
$10CE-$1149   c64CtrlKeys      cursor, colour and function keys
$114A-$11A9   c64GermanKeys    the German layout and its umlauts
$11AA-$11B8   c64MacroFile     "@:BTX-MAK-<id>,S,W"
$11B9-$11F7   variables
$11F8-$166F   31 CEPT status records
$1670-$16AD   c64StrTable, indexing them
$16AE-$2D60   code, with data islands in it
```

The data below `$16AE` is one contiguous block; above it, data sits wherever
the routine that owns it happens to be:

```
$1871  c64AsciiKeys     $1997  c64MenuKeys      $1A41  ceptMonitorMsg
$2464  c64ExtraFile     $24A4  c64SplashText    $296F  ceptMacroDir
$2BBD  ceptStartPage
```

`ceptStartPage` is last, and that is not a coincidence: `c64BufStart` points at
it, so the capture buffer begins where the startup page ends the image.

100% of the image is accounted for: 13663 bytes of code, 19105 of typed data.
43 bytes in four fragments are unreachable dead code — no reference anywhere in
the image, no branch target, and each preceded by an instruction that ends
flow. One of them is identifiable: `$FAE4` is the ASCII mode's missing
cursor-right handler.

---

## 9. Revisions

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

## 10. Not established

- **Font bit 15.** The contiguous-graphics reading is an inference from
  distribution; the firmware never reads the font, so nothing in the ROM tests
  the bit. A schematic would settle it.
- **The modem protocol's finer points.** Every variable is named and the CRC is
  identified — `modemCrc` at `$F818` is a reflected CRC-16 with polynomial
  `$8005` — but the meaning of individual DBT-03 command bytes in `modemCmd`
  and `modemReply` is read off their values, not from a specification.
- **`$1B24`–`$1B26`.** Three display options toggled by host commands `R`, `T`
  and `W`, snapshotted for the redraw as `redrawT` and `redrawW`. Named after
  the letters that set them, since what they switch is not established.
- **`$61F9`–`$61FD`.** Write-only control registers whose function is unknown.
  Their C64-side counterparts `$81F8`–`$81FD` are touched by the payload's cold
  start and its IRQ handler, which says they carry an interrupt enable and
  acknowledge, but not which bit is which.
- **Four interface registers.** `$8005`, `$800F`, `$8011` and `$8012` are read
  or written by the payload without their meaning being established. `$8011` is
  a counter the decoder advances — `c64WaitDecoder` spins on it and
  `c64MacroRecOpen` paces the macro file against it.

  `$8090` is settled and now called `btxStatusMsg`: it is `c64StatusMsg` at
  `$6090`, the decoder's copy of `statusMsg`. The C64's `$28` and `$50` tests
  are records 2 and 4 — "Verbindung" and "Abbruch".
- **`videoReg0`–`videoReg3`.** Four bytes at `$1B2A` written to `P3CSR` with
  `AND #$1F`, initialised to 0, 1, 2, 3. They select something in the video
  hardware; nothing in the ROM says what.
- **The `$D419` no-match path.** Reachable, but harmless: two of its three
  outputs are read only for bit 7 and the third is clamped. Documented in the
  sidecar rather than treated as a bug.
- **Two cell bytes.** Both cell readers store `c64Cell3` and `c64Cell5`, and
  nothing in the payload reads them back.
---

## 11. Where this stands

100% of the 32 KB is classified, the listing reassembles byte-identical under
two independent assemblers, and no operand anywhere in either processor's
source is a bare address: every location in external RAM, every zero-page
variable on both sides, every dispatch-table entry and every C64 ROM entry
point carries a name.

The names are not uniformly strong, and the difference matters when reading
them. Most are evidenced — `inStatusLine` by the routine pair that saves and
restores around it, `modeAscii` by the record `showModeName` prints for it,
`drcsCell` by the G-set code that sets it and the 12 rows that follow. A few
are structural: `modeR`, `modeT` and `modeW` are named after the host command
letters that toggle them because what they switch is not established, and
`spare0616` and `spare04D3` are cleared at reset and never touched again.

The open questions above are what is left, and they share a shape: they need
either a schematic or a specification, not more reading of this ROM.
