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
`$00F0`–`$00FB`. `nullHandler` at `$F147` is a bare `RTI`, installed into the
vectors reset does not need. IRQ1 gets the two instructions right after it,
`hostIrq`:

```
hostIrq:
        JSR     fetchHostByte
        RTI
```

so an interrupt from the C64 side is answered by pulling a byte out of the ring
and nothing else. That the soft vectors live in
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

## 3. The C64 interface

### The cartridge announces itself

The C64 sees an autostart cartridge at `$8000`, but not by an address decode
onto this ROM. `copyBootstrapToC64` at `$B2B4` puts it there:

```
        LDS     #c64BootstrapBlock-1
        LDX     #c64Window
loop:   PULA
        STAA    $00,X
        INX
        CPX     #$6079
        BCS     loop
```

`$6079 - $6000` is 121 bytes — exactly the size of the bootstrap block,
`$B32D`–`$B3A5`. So the decoder **copies** the whole thing, CBM80 header
included, into the dual-port window it shares with the C64. The C64 autostarts
out of RAM the decoder filled a moment earlier, which is also what makes
`btxLoadLo`/`btxLoadHi` writable at all.

Everything the header proves still holds, because the bytes are the same either
way:

```
$B32D  13 80              cold start vector  $8013
$B32F  72 FE              warm start vector  $FE72, the KERNAL's NNMI20
$B331  C3 C2 CD 38 30     "CBM80"
```

`$8013` resolves to ROM `$B340`, exactly where `c64CartStart` begins, and the
`JMP $8036` at `$B36D` lands on `c64CopyLoop`.

### The C64 pulls its own software across

`c64CartStart` runs from the copy in the window, not from this ROM. It runs
the C64's cold-start sequence, then fetches a two-byte destination pointer and copies a byte stream into it:

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

`$8000` holds the cold-start vector `$8013` while the cartridge is visible, and
the loader has already written the fetched load address over it in
`btxLoadLo`/`btxLoadHi`. Because the window is RAM the decoder filled — not
ROM — that store simply takes, and once the cartridge is banked out the same
indirect read returns `$1000`. That is why the loader stores the load address
twice: to `c64Ptr` for its own copy loop, and to `btxLoadLo` for this jump.

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
`ANDB #$1F` on `c64OutFifo` and `CMPB #$40` on `hostInFifo`, the C64 `CPX #$20` on
`btxRxFifo` and `CPX #$40` on `btxTxFifo`. `sendPayloadToC64` uses `ANDB #$0F`
over `$6080`, and `c64BootGetByte` `CPX #$10` — a third, smaller ring that
exists only during the transfer.

Once the payload is running, `$8080`–`$808B` is reused as a **mailbox** and
`$8080` itself becomes the reduced-display toggle. The mailbox is one display
cell, and the two sides name the same twelve bytes:

| | 6801 writes | C64 reads |
|---|---|---|
| `$x081` | `cellCurCol` — `cursorCol` with `cursorVisible` folded in | cursor column |
| `$x082` | `cellCurRow` | cursor row |
| `$x083` | `cellChar` ← `glyphCode` | `c64CellChar` |
| `$x084` | `cellAccent` ← `accentCode` | `c64CellAccent` |
| `$x085` | `cellAttr0` ← `$00E0` | `c64Cell3` — stored, never read |
| `$x086` | `cellSet` ← `$00E1` | `c64CellSet` |
| `$x087` | `cellAttr2` ← `$00E2` | `c64Cell5` — stored, never read |
| `$x088` | `cellAttr3` ← `$00E3` | `c64CellAttr` |
| `$x089` | `cellRow` ← `renderRow` | plot row |
| `$x08A` | `cellCol` ← `renderCol` | plot column |
| `$x08B` | `cellReady` | the flag both sides poll |

`cellSet` is worth a second look. On the C64 side `c64CellSet`'s low three bits
select the character set; on the 6801 side the byte is `$00E1`, and `$00E1 & $07
== 5` is exactly the test that sets `drcsCell`. The two processors agree on the
field without either listing saying so.

### The interface register map

Every register in the window now has a name on at least one side. The 6801 sees
it at `c64Window`, the C64 at `$8000`:

| 6801 | C64 | |
|---|---|---|
| — | `btxLoadLo` `btxLoadHi` | the payload's load address, and what `c64StartPayload` jumps through |
| — | `btxCartCtrl` | write `$00` to unmap the cartridge |
| `c64FifoWr` `c64FifoRd` | `btxRxWr` `btxRxRd` | indices into the decoder → C64 ring |
| `c64XferEn` | `btxXferEn` | transfer enable |
| `c64Status` | `btxStatus` | status |
| — | `btxTxWr` `btxTxRd` | indices into the C64 → decoder ring |
| — | `btxHostActive` | write-only, purpose open |
| `c64XferDone` | `btxXferDone` | completion |
| `pageCount` | `btxPageCount` | `$1A` markers counted off the line |
| — | `btxSessionUp` | the startup page has already been sent |
| `$6020` | `btxRxFifo` | 32-byte ring, decoder → C64 |
| `$6040` | `btxTxFifo` | 64-byte ring, C64 → decoder |
| `c64Fifo` | `btxFifo00` | boot ring base, then the reduced-display toggle |
| `cellCurCol`…`cellReady` | `btxCellCurCol`…`btxCellReady` | the cell mailbox above |
| `c64StatusMsg` | `btxStatusMsg` | `statusMsg`, the status-message index |
| — | `btxIrqCtrl` | bit 6 enables, bit 7 says the interrupt is the decoder's |
| `c64IrqSet` | `btxIrqAck` | write to raise, read to clear |
| `c64IrqArmA` `c64IrqArmB` | `btxIrqArmA` `btxIrqArmB` | strobes, purpose open |

### The four registers only one side touches

`$8005`, `$800F` and `$8012` are addressed by the C64 alone — the 6801 never
touches `$6005`, `$600F` or `$6012` by any mode. What they do follows from
where the payload writes them:

- **`btxCartCtrl`** at `$8005` gets `#$00` in exactly one place: `c64MenuQuit`,
  the instruction before `JMP (KERNAL_RESET)`. That is what makes Quit work —
  the cartridge unmaps itself so the reset comes up as a plain C64, which is
  what `ceptMonitorMsg` has just told the user to expect.
- **`btxSessionUp`** at `$8012` is read once and set once, both in
  `c64StartSession`: if it is already set the startup page is not sent again.
  It lives in the cartridge, so it survives the C64 reset that Quit performs.
- **`btxHostActive`** at `$800F` is written `$FF` and never read.

`$8011` is the one both sides share, and the 6801 end says what it counts:

```
LF0B5:  LDAB    pageCount
        CMPB    pageCount
        BNE     LF0B5
        INCB
        STAB    pageCount
```

reached only from `CMPA #$1A` at the end of the receive loop. So the decoder
counts `$1A` bytes arriving from the line — the end-of-page marker in the BTX
datastream — and publishes the count as `btxPageCount`. `c64WaitDecoder` spins
until it changes, which is how a macro waits for the next page, and
`c64MacroRecOpen` paces the macro file against it.

### The interrupt line

The mailbox has a doorbell, and both ends of it are unambiguous. The 6801 only
ever **writes** `$61F9` — never reads it — and every write is the instruction
after `STAA $608B`, the flag saying a cell is waiting:

```
        LDAA    renderRow
        STAA    $6089
        LDAA    renderCol
        STAA    $608A
        LDAA    #$FF
        STAA    $608B
        STAA    c64IrqSet
```

The C64 only ever **reads** `$81F9` — never writes it — on both exit paths of
`c64IrqPlotCell`, after consuming that cell. Write to raise, read to clear,
with no counterexample on either side.

`btxIrqCtrl` at `$81F8` is the C64's control and status half. Cold start writes
`#$40` to enable it, and the IRQ handler tests bit 7 to decide whose interrupt
this is:

```
L2359:  BIT     btxIrqCtrl
        BMI     L2364           ours - service the cell and RTI
        JSR     vecIrqPlotCell
        JMP     KEY             not ours - chain to the KERNAL
```

So the decoder-driven display update is genuinely interrupt-driven on the C64
side, sharing `CINV` with the KERNAL's keyboard scan rather than displacing it.

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
otherwise pulls from `hostFifoGet` — the `hostInFifo` end of the ring the C64 fills
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
- 9-bit frame shifted out of `txShift`, fed from `txRing` at `$04EE`

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

`showModeName` at `$EF1C` does the same for `strModeNames` — and because it
loads S one byte early for `PUL`, the listing shows which record each flag
picks without any arithmetic:

```
        LDS     #strModeNames-1         CEPT
        LDS     #strModeNames+79        HEX
        LDS     #strModeNames+19        PRESTEL
        LDS     #strModeNames+59        ANTIOPE
        LDS     #strModeNames+39        ASCII
```

It is what identifies the terminal-mode flags — it tests each in turn and loads S with
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

`parallelMode` chooses between the two C1 sets. `escSelectC1Set` (ESC `$22` — CEPT's
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
(`gsetGL` and `gsetGLDefault`), while `ctlSS2` sets only `gsetSS`.

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
own control codes are kept from disturbing the overlay — and it is the guard
the CV30113 revision still has on `CAN`.

### A second interpreter

`asciiCtrlTable` is a 32-entry C0 table with its own dispatcher, `asciiDispatchC0`, serving the
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

| | Address | Set |
|---|---|---|
| `fontG0` | `$8000` | Latin alphanumerics |
| `fontAccents` | `$8780` | non-spacing diacriticals and symbols (G2) |
| `fontMosaic` | `$8F00` | 2×3 block mosaics (G1) |
| `fontSet3` | `$9680` | line drawing and diagonals |

Each glyph is 10 rows of 2 bytes, and **rows are stored little-endian** — the
first byte of a row is its *right* half. Read in the 6801's usual big-endian
order every glyph splits across two characters, which is what makes this data
look like noise at first sight.

Ink is **bits 0–11** of the swapped word — the 12-pixel CEPT cell, bit 11
leftmost. Bits 12–14 are zero in all 3840 rows of all four sets.

**Bit 15 marks a glyph as separable**, and the firmware reads it:

```
LA69B:  LDX     glyphPtr
        TST     $02,X           bit 15 of row 0 - glyphPtr is base-1,
        BPL     LA6F8           so $02,X is glyph byte 1
        LDAA    >curSet
        BITA    #$07            only for a graphics set
        BEQ     LA6F8
        LDAA    >curAttr0
        BITA    #$08            separated by attribute
        BNE     LA6B7
        LDAA    >curSet
        ANDA    #$20            or by set
        BEQ     LA6F8
```

When all three hold, the glyph is copied into `glyphBuf` and ANDed with
`separationMask`, twenty bytes at `$AF7B`:

```
row 0  001111001111        rows 2, 6 and 9 are cleared entirely
row 1  001111001111
row 2  000000000000
```

The mask clears the leftmost two pixels of each 6-pixel block column and the
last row of each block row — turning contiguous mosaics into **separated
mosaics**, in software. The block grid it implies, two columns by rows 0–2,
3–6 and 7–9, is the same one the mosaic codes use: `$21` lights rows 0–2 of the
left half, `$30` rows 7–9 of it.

The distribution confirms the reading and is per-glyph exactly — bit 15 is set
on all ten rows of a glyph or none:

| Set | Bit 15 set | |
|---|---|---|
| G0 | 1 / 96 | only `$7C`, the full-height vertical bar |
| accents | 0 / 96 | |
| mosaic | 94 / 96 | all but `$4E` and `$5E` |
| line | 60 / 96 | the 36 clear include the 35 placeholder slots |

*This corrects an earlier reading in these notes, which had bit 15 as a
**contiguous** flag consumed by the video hardware, on the grounds that the
firmware never touches the font. That was wrong: the glyph fetch reads through
the stack pointer with `PUL` and the flag test is an indexed byte off
`glyphPtr`, so neither appears as a font reference under ordinary addressing.*

`fontNarrow` at `$9E00`–`$A20F` is a second font: 104 glyphs, 8 pixels wide, one byte per
row, no byte-order surprise.

### Display memory

Four planes, addressed through the `lineAddr` tables at `$FC45`–`$FD0C` — 25 rows of 40
columns, the CEPT page plus a status line:

| Plane | Base | Per cell | Row stride | Role |
|---|---|---|---|---|
| `planeRender` | `$4000` | 1 byte | 40 | render byte derived from attribute bits at `$E860` |
| `planeAttr` | `$4400` | 4 bytes | 160 | attributes, written only by the C1 handlers |
| `planeChar` | `$5400` | 1 byte | 40 | **character codes** |
| `planeAccent` | `$5800` | 1 byte | 40 | combining-accent overlay |

`$E9A5` reloads all four row pointers whenever the cursor row changes, from the
four `lineAddr` tables.

The redraw walks the planes with a second set of pointers in internal RAM —
`walkChar`, `walkAttr` and `walkAccent` — and unpacks each cell's four
attribute bytes into `curAttr0`, `curSet`, `curAttr2` and `curAttr3` before
deciding anything about it. Four bytes in internal RAM are cheaper to reach
than four indexed loads, and every glyph decision reads them repeatedly. They
are also exactly what the mailbox forwards to the C64, so `curSet` and the
payload's `c64CellSet` are the same byte two processors apart. Those strides are the clearest statement of the
layout anywhere in the image: three planes are one byte per cell across 40
columns, and `planeAttr` is four, which is exactly why its rows sit 160 apart.

What separates attributes from content is decisive: `attr0`–`attr3` are never
assigned a raw byte — only ever bit-manipulated by the C1 handlers — whereas
the incoming character lands in `charCode` and only that value reaches
`planeChar`.

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
depends on the format `fmtLookup` resolved from the `fmtKeys` tables:

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

The `drcsPlane` buffers are staging only. A finished definition lives in
`drcsStore` above `$0800`, 48 bytes per character, written by `drcsWriteRow`
through `drcsStorePtr` and cleared 96 characters at a time by
`drcsClearStore`. `drcsGlyphPtr` at `$A7E7` addresses it as
`(glyphCode & $7F - $20) * 48 + $07FF`.

The renderer decides which store to use from `drcsCell`, set when the current
G-set code is 5 — the ESC 2/8 2/0 designation. A DRCS cell is also **12 rows
rather than 10**, which is what `blitRows` carries into the blit loop.

### The three display modes

`$A351` onward is where a cell can be simplified before it is drawn. All three
modes act on the copy in `curAttr0`–`curAttr3`, never on the planes, so a page
is not damaged by being looked at in a reduced form.

| Mode | Set by | Effect |
|---|---|---|
| `modePlain` | host `T`, the C64's F4 | strips the page back to defaults — `curAttr2` to `$07`, `curAttr3` to `$99`, `curSet` to its low nibble |
| `modeMono` | host `W`/`w` | forces `fgColour` 7 and `bgColour` 0 instead of unpacking them from the attributes |
| `modeReveal` | host `R`, the C64's F2 | forces bit 3 of `curAttr3` on |

`modeReveal` is the one worth spelling out. Bit 3 of `curAttr3` is what makes a
character visible: with it clear, `$A479` copies `bgColour` over `fgColour` and
the cell is painted in its own background. So the bit is **conceal**, and
`modeReveal` overrides it — the teletext function of that name, on the key it
traditionally sits on.

`$99`, the value `modePlain` writes to `curAttr3`, is the same one `clearPlanes`
writes at power-on, so "plain" is literally "as if nothing had set anything".

### Alternative letter shapes

`$A618` runs when `curSet` bit 3 is set — the accent bit — and it swaps the
glyph before anything else looks at it:

```
        LDAB    #$FF
next:   INCB
        LDX     #accentLetters
        ABX
        LDAA    $00,X
        BEQ     miss                $00 terminates the list
        CMPA    glyphCode
        BNE     next
        ASLB
        LDX     #accentGlyphPtrs
        ABX
        LDX     $00,X
        DEX
        STX     glyphPtr
```

23 letters — **ACDEGHIJLNORSTUWYZ** and **dhlti** — each with its own 20-byte
glyph, the same size as an ordinary one. The three tables are contiguous and
their sizes check against each other: 23 letters plus a terminator end at
`$AFBB` where the addresses begin, 23 addresses spaced exactly 20 apart end at
`$AFE9` where the first glyph begins, and 23 glyphs of 20 end at `$B1B5`.

The letter list is the argument for what these are. It is the set that *changes
shape* under a diacritic rather than the set that takes one — which is why the
lowercase entries are `d`, `h`, `l`, `t` and `i`: the ascenders that must be
shortened to make room, and the `i` that loses its dot.

### Combining accents

`planeAccent` is not a duplicate character plane. When a character comes
from G2 and its code lies in `$40`–`$4F` — where CEPT puts the non-spacing
diacriticals — `$E972` stores it there and returns **without advancing the
cursor**, which is what makes it combining rather than spacing. The next
character picks up an attribute bit from `accentPending` and goes to `planeChar`
normally. The firmware never reads `planeAccent`, so the video hardware composites
the mark over the base glyph.

The C64 side shows the same design from the other end: the cell it receives has
the accent in its own byte, and `c64CellOut` recombines accent plus base letter
into a single printer code.

### Turning cells into pixels

The four planes are not what the video hardware scans. `redrawScreen` at
`$A210` walks them and renders into `videoRam` through `videoPtr`, one
12-pixel cell at a time.

`videoRam` runs `$5C00`–`videoRamTop` at `$5FFF` — 1 KB, ending exactly where
`c64Window` begins. Both clear loops step through it by
4 and stop on `CPX #videoRamTop`, which is what fixes the extent.

The unit of 4 bytes is a **scanline, not a cell**: `renderCol` never enters the
address at all, it goes to `PORT3` with `#$18` added, so the hardware latches
the column and the four bytes at `videoPtr` are that column's slice of one
line. 25 rows times `glyphRows` of 10 is 250 units, which is why the clear
fills the six-unit tail separately. Within a unit the renderer writes `$00,X`
and `$02,X` — plus `$04,X` and `$06,X` when `tallCell` is set — while the clear
touches only `$03,X`, from `rowFlags`. `redrawReq` at `$1B23` is how the CEPT handlers
ask for that: they set it, and the main loops test it against `statusDirty` and call
in.

Fetching a glyph is done in an unusual way. `glyphPtr` is set to the font base
for the current set — through `fontBaseTable` — plus 20 times `glyphCode`,
then decremented, because the 6801's `PUL` pre-increments. The rows are then
read with `PULA`: **S is loaded from `glyphPtr`** and the real stack is parked
in `savedS` under `SEI` for the duration.

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
arithmetic — worth the trouble at 40 columns times `glyphRows` a frame. `copyBootstrapToC64` uses the same trick to walk ROM, which is why it parks S
in what is otherwise `scrollEnd`, and `showModeName` and `showStatusMsg` use it
to walk their message tables.

Each pass ORs into `glyphBuf`, and `glyphPtr` is afterwards pointed at
`glyphBuf` itself, so a second pass composites over the first. That is how the
combining accent from `planeAccent` is merged onto its base character before
either reaches `videoRam`.

### Cursor and scrolling

`cursorCol` (`$1B1F`) wraps against 40 and 39; `cursorRow` (`$1B1E`) is clamped
between `scrollTop` and `scrollBottom` with a ceiling in `cursorRowMax`. `$EC16` is
the wrap logic; `$EC8D` and `$ECA1` are the bounded row moves, each falling
through to scroll at the window edge.

Double-width rendering uses `bitDoubleTable` at `$AEFB` — 64 words, entry n
being n with every bit duplicated, **byte-swapped**: entry 1 reads `FDB $0300`,
not `$0003`. All 64 were checked against the doubling by computation. The swap
is the one the font rows use and is there for the same reason — readers do
`LDX #bitDoubleTable / ABX / LDD $00,X` with an even index, so what `LDD`
yields is already in display order.

Four tables sit back to back with no gaps, and the sizes are what pin each
boundary — 10 bytes, then one, then 128, then 20:

```
$AEF0  fontBaseTable   FDB fontG0,fontAccents,fontMosaic,fontSet3,fontNarrow
$AEFA  lineWidthMax    $27 - renderCol wraps against this, so the line width
                       is a ROM constant rather than an immediate
$AEFB  bitDoubleTable  64 little-endian words
$AF7B  separationMask  20 bytes
```

---

## 8. ROM map

```
$8000-$877F   fontG0             96 glyphs x 20 bytes
$8780-$8EFF   fontAccents        96 glyphs, CEPT G2
$8F00-$967F   fontMosaic         96 glyphs, CEPT G1
$9680-$9DFF   fontSet3           96 glyphs, line drawing
$9E00-$A20F   fontNarrow        104 glyphs x 10 bytes, 8 pixels wide
$A210-$AEEF   firmware
$AEF0-$AEF9   fontBaseTable      5 words, one per character set
$AEFA         lineWidthMax       $27
$AEFB-$AF7A   bitDoubleTable     64 words
$AF7B-$AF8E   separationMask     20 bytes
$AF8F-$AFA2   fontSet6Glyph      one glyph, for G-set code 6
$AFA3-$AFBA   accentLetters      23 letters and a terminator
$AFBB-$AFE8   accentGlyphPtrs    23 words, spaced 20 apart
$AFE9-$B1B4   accentGlyphs       23 glyphs x 20 bytes
$B1B5-$B1FF   fontTail           75 bytes
$B200-$B32C   reset and sendPayloadToC64
$B32D-$B33F   c64CartHeader      the "CBM80" autostart header
$B340-$B3A5   c64CartStart, executed at C64 $8000
$B3A6-$B3A7   c64LoadAddr        $1000
$B3A8-$D108   C64 terminal application, runs at $1000-$2D60
$D109-$D348   ctrlTableC0..csiTable, the CEPT dispatch tables
$D419-$D460   fmtKeys, fmtVals041C, fmtVals041D, fmtVals041F
$E000-$EF66   firmware
$EF67-$EFCA   strModeNames       5 records x 20
$EFF5-$F080   strStatusMsgs      7 records x 20, including the version
$F9FA-$FA39   asciiCtrlTable     32 handler addresses
$FC45-$FD0C   lineAddrAccent, lineAddrChar, lineAddrAttr, lineAddrRender
$FD21-$FFEF   romPadding         $FF fill, 719 bytes
$FFF0-$FFFF   vectors            the eight 6801 hardware vectors
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

Only two things changed between them: the `inStatusLine` guard was removed from
the `CAN` handler, and three umlaut glyphs went from lowercase to uppercase. The
entire C64 payload is byte-for-byte identical. See
`docs/cv30113-revision-diff.md`.

---

## 10. Not established

- **The modem protocol's finer points.** Every variable is named and the CRC is
  identified — `modemCrc` at `$F818` is a reflected CRC-16 with polynomial
  `$8005` — but the meaning of individual DBT-03 command bytes in `modemCmd`
  and `modemReply` is read off their values, not from a specification.
- **`$61FC`/`$61FD` and `$81FC`/`$81FD`.** Strobes in the interrupt-enable
  path, named `c64IrqArmA`/`c64IrqArmB` and `btxIrqArmA`/`btxIrqArmB`. What
  they gate is not established — only that the data written is ignored, since
  the C64 writes back what it read and the 6801 writes whatever `PORT1` left in
  A.
- **`btxHostActive` at `$800F`.** Write-only, `$FF`, at exactly two points —
  the top of `c64MainLoop` and just before `c64StartSession` sends the startup
  page. Nothing reads it and the 6801 never addresses `$600F`, so the name
  records where it is written, not what it does.
- **`videoReg0`–`videoReg3`.** Four bytes at `$1B2A` written to `P3CSR` with
  `AND #$1F`, initialised to 0, 1, 2, 3. They select something in the video
  hardware; nothing in the ROM says what.
- **The `fmtLookup` no-match path.** Reachable, but harmless: two of its three
  outputs are read only for bit 7 and the third is clamped. Documented in the
  sidecar rather than treated as a bug.
- **Two cell bytes.** The decoder puts `curAttr0` and `curAttr2` into `cellAttr0` and
  `cellAttr2`, and the C64 stores them as `c64Cell3` and `c64Cell5` and never
  reads them back. Both sides carry the fields; neither uses them.

---

## 11. Where this stands

100% of the 32 KB is classified, the listing reassembles byte-identical under
two independent assemblers, and no operand anywhere in either processor's
source is a bare address — an address inside a named table now reads as
`NAME+offset`, and one loaded a byte early for a `PUL` walk as `NAME-1`. Every location in external RAM, every zero-page
variable on both sides, every dispatch-table entry, every C64 ROM entry point
and every register in the shared interface window carries a name.

That last one is what closed most recently, and it is worth saying what it
bought: the two listings can now be read against each other. `cellSet` and
`c64CellSet` are the same field, `pageCount` and `btxPageCount` the same
counter, `statusMsg` and `btxStatusMsg` the same index — so a question about
one processor can be answered from the other. Several findings here came out
that way rather than from either image alone.

Every named routine also carries a note saying what it does, and a test fails
if one gains a name without one — a name says what something is called, which
is not the same thing.

The names are not uniformly strong, and the difference matters when reading
them. Most are evidenced from behaviour: `inStatusLine` by the routine pair
that saves and restores around it, `modeAscii` by the record `showModeName`
prints for it, `drcsCell` by the G-set code that sets it and the twelve rows
that follow, `modeReveal` by the bit it forces and what clearing that bit does
to the cell.

A few are named for where they sit rather than what they do, and those are
exactly the open questions above: `btxIrqArmA`/`btxIrqArmB` name a position in
the interrupt-enable sequence, `btxHostActive` the two places it is written,
and `videoReg0`–`videoReg3` the register they are shifted into. `spare0616`
and `spare04D3` are cleared at reset and never touched again.

That is the shape of what is left. Each needs a schematic — the ROM writes
these locations and never reads them back, so no amount of further reading
will say what is on the other side of the write.
