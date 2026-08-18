# CV30113 vs decoder_ii — revision diff

Comparison of the two 32 KB ROM images:

| | SHA-256 (first 16) |
|---|---|
| `c64_btx_decoder_ii.bin` | `2799910767fdb706` |
| `c64_BTX_decoder_CV30113 C375-B1-1 (EX).BIN` | `2baed45ee9df97db` |

decoder_ii is **V3.3** and CV30113 is **V3.1**, from the `Decodersoftware Vx.x`
string at `$EFF5` in both images. So decoder_ii is the newer build and the
changes below run V3.1 -> V3.3.

8549 of 32768 bytes differ (26.1%), but almost all of that is relocation rather
than change. Reproduce with `python3 tools/diffrom.py`.

## There are only two real changes

### 1. A guard was removed (the substantive one)

V3.3 drops eight bytes from the `CAN` handler at `$DB69` that V3.1 had:

```
V3.3 (decoder_ii)             V3.1 (CV30113)
$DB69: LDX  $040A             $DB69: TST  $04AF        <-- only in V3.1
$DB6C: LDAA 0,X               $DB6C: BPL  $DB71
$DB6E: BMI  $DB66             $DB6E: JMP  $D355
                              $DB71: LDX  $040A
                              $DB74: LDAA 0,X
                              $DB76: BMI  $DB66
```

The inserted bytes are `7D 04 AF 2A 03 7E D3 55`. That is exactly the sequence
already standing in front of the neighbouring handlers at `$DB2B` and `$DB5B`
in *both* images. V3.1 gave `CAN` the same `$04AF` mode check its siblings
have; V3.3 removed it, so `CAN` now runs regardless of that mode.

`$04AF` is the same flag tested at `$D3A8` and `$D3C8` on the ESC and CSI
paths before they do any work.

Those eight bytes account for the entire remaining diff: everything above
`$DB69` shifts by +8, and every pointer that targets it was adjusted. The tail
of the image matches at shift +8 (83.7%, against roughly 10% at every other
shift). The dispatch tables show the same thing entry by entry - targets below
the insertion are untouched (`DA 40`, `DB 0E`, `DB 69`) while every target above
it gains 8 (`DB D2`->`DB DA`, `DC 6D`->`DC 75`).

### 2. Three umlaut glyphs were redrawn

In the G2 accent set at `$8780`, codes `$59`, `$5A` and `$5B` differ:

- V3.1 (CV30113) carries lowercase **ä ö ü**
- V3.3 (decoder_ii) carries uppercase **Ä Ö Ü**

No other glyph in any of the four character sets differs.

## What did NOT change

- The entire C64-side 6502 payload, `$B3A8`-`$D108`, is byte-for-byte identical.
  Both revisions ship the same terminal application.
- The G0, mosaic and set3 character generators.
- All jump-table structure; only the target addresses moved.
- The `$B211`-`$B29F` reset-area differences are purely relocated call targets
  (`JSR $E9A5`->`$E9AD`, `LDD #$F61F`->`#$F627` and so on), not logic changes.

## Which is newer

**decoder_ii is the later revision.** Both images carry a version string at
`$EFF5`, in the status-message block:

| Image | Version |
|---|---|
| `c64_btx_decoder_ii.bin` | `Decodersoftware V3.3` |
| `CV30113 C375-B1-1 (EX)` | `Decodersoftware V3.1` |

So the direction of both changes is the reverse of what this document first
concluded. Going from **V3.1 to V3.3**:

- the `TST $04AF` guard was **removed** from the handler at `$DB69`, not added;
- the umlauts went from lowercase **ä ö ü** to uppercase **Ä Ö Ü**.

`$DB69` is the `CAN` handler (C0 `$18`). Under V3.1 it aborted unless `$04AF`
was set; V3.3 lets it run unconditionally, so the newer build makes CAN take
effect in a mode where it was previously ignored. Read that way the removal is
itself a fix, which is why the heuristic that misled the first pass - "gaining
a guard is the normal direction, losing one is not" - was the wrong tool here.
The version strings are direct evidence and settle it.
