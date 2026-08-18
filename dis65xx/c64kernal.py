"""C64 ROM entry points, taken from the ROM source itself.

Names come from Michael Steil's reconstruction of the original Commodore
sources at https://github.com/mist64/c64rom, built with cc65 so every label
carries its real address. Earlier versions of this file used names invented
here - IEC_CIOUT, KERNAL_CLOSE - which looked plausible and were wrong.

These live in the C64's address space, not in this ROM, so they are emitted as
EQU symbols. Naming them matters for more than readability: an L<address> name
for $FDA3 would share a namespace with a listing label at ROM $FDA3 on the
6801 side.

Addresses the source does not label keep a KERNAL_<addr> form, which says C64
ROM without claiming to know what it is.
"""

from __future__ import annotations

NAMES: dict[int, str] = {
    0xECF0: "LDTB2",
    0xFE72: "NNMI20",
    0xFD88: "SIZE",
    0xE3BF: "INITCZ",
    0xE56C: "STUPT",
    0xEA31: "KEY",
    0xED09: "TALK",
    0xED0C: "LISTN",
    0xEDB9: "SECND",
    0xEDC7: "TKSA",
    0xEDDD: "CIOUT",
    0xEDEF: "UNTLK",
    0xEDFE: "UNLSN",
    0xEE13: "ACPTR",
    0xF3D5: "OPENI",
    0xF3F6: "OP35",
    0xF642: "CLSEI",
    0xFD15: "RESTOR",
    0xFDA3: "IOINIT",
    0xFF5B: "PCINT",
    0xFFC0: "OPEN",
    0xFFC3: "CLOSE",
    0xFFC6: "CHKIN",
    0xFFC9: "CKOUT",
    0xFFCC: "CLRCH",
    0xFFCF: "BASIN",
    0xFFD2: "BSOUT",
    0xFFE1: "STOP",
    0xFFE4: "GETIN",
    0xFFE7: "CLALL",
    # Not labelled in the source: $FFFC is a CPU vector rather than a routine.
    0xFFFC: "KERNAL_RESET",
}

# Entry points partway into a labelled routine. The bootstrap enters ramtas
# eight bytes in, at $FD90, which skips the memory clear and keeps only the
# pointer setup - so name it relative to the label the source does give.
INTERIOR: dict[int, tuple[str, int]] = {
    0xFD90: ("SIZE", 0xFD88),
}


def known_name(addr: int) -> str | None:
    """The ROM source's own name, or None. Unlike name_for this never invents
    a KERNAL_<addr>, so it is safe to apply to any operand rather than only to
    call targets."""
    if addr in NAMES:
        return NAMES[addr]
    if addr in INTERIOR:
        name, base = INTERIOR[addr]
        return f"{name}+{addr - base}"
    return None


def name_for(addr: int) -> str | None:
    """Symbol for a C64 ROM address, or None if it is not in ROM space."""
    if addr in NAMES:
        return NAMES[addr]
    if addr in INTERIOR:
        name, base = INTERIOR[addr]
        return f"{name}+{addr - base}"
    if addr >= 0xA000:
        return f"KERNAL_{addr:04X}"
    return None
