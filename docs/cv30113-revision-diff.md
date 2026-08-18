# CV30113 vs decoder_ii — revision diff

Comparison of the two 32 KB ROM images:

| | SHA-256 (first 16) |
|---|---|
| `c64_btx_decoder_ii.bin` | `2799910767fdb706` |
| `c64_BTX_decoder_CV30113 C375-B1-1 (EX).BIN` | `2baed45ee9df97db` |

8549 of 32768 bytes differ (26.1%), but almost all of that is relocation rather
than change. Reproduce with `python3 tools/diffrom.py`.

## There are only two real changes

### 1. A missing guard was added (the substantive one)

CV30113 prepends eight bytes to the control-code handler at `$DB69`:

```
decoder_ii                    CV30113
$DB69: LDX  $040A             $DB69: TST  $04AF        <-- inserted
$DB6C: LDAA 0,X               $DB6C: BPL  $DB71
$DB6E: BMI  $DB66             $DB6E: JMP  $D355
                              $DB71: LDX  $040A
                              $DB74: LDAA 0,X
                              $DB76: BMI  $DB66
```

The inserted bytes are `7D 04 AF 2A 03 7E D3 55`. That is exactly the sequence
already standing in front of the neighbouring handlers at `$DB2B` and `$DB5B`
in *both* images, so the handler at `$DB69` was simply missing the `$04AF`
mode check its siblings have, and CV30113 supplies it.

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

- decoder_ii carries uppercase **Ä Ö Ü**
- CV30113 carries lowercase **ä ö ü**

No other glyph in any of the four character sets differs.

## What did NOT change

- The entire C64-side 6502 payload, `$B3A8`-`$D108`, is byte-for-byte identical.
  Both revisions ship the same terminal application.
- The G0, mosaic and set3 character generators.
- All jump-table structure; only the target addresses moved.
- The `$B211`-`$B29F` reset-area differences are purely relocated call targets
  (`JSR $E9A5`->`$E9AD`, `LDD #$F61F`->`#$F627` and so on), not logic changes.

## Which is newer

**CV30113 is very probably the later revision.** It adds a defensive check that
the other lacks, and gaining a guard is the normal direction for a fix; losing
one is not. The umlaut change is consistent with a deliberate character-set
correction but does not by itself indicate direction.

This is an inference from the content, not from any date or version string -
neither image carries one.
