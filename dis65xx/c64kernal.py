"""C64 ROM entry points called by the embedded 6502 code.

These live in the C64's address space, not in this ROM, so they are emitted as
EQU symbols. Naming them matters for more than readability: an L<address> name
for, say, $FDA3 would sit in the same namespace as a listing label at ROM
$FDA3 and could collide with the 6801 side.

Only entries whose identity is well established carry a name. Anything else
gets a KERNAL_<addr> form - explicitly C64 ROM, but making no claim about what
it does.
"""

from __future__ import annotations

# Documented KERNAL jump-table entries.
JUMP_TABLE = {
    0xFFD2: "CHROUT", 0xFFE1: "STOP", 0xFFE4: "GETIN", 0xFFE7: "CLALL",
    0xFFBA: "SETLFS", 0xFFBD: "SETNAM", 0xFFC0: "OPEN", 0xFFC3: "CLOSE",
    0xFFC6: "CHKIN", 0xFFC9: "CHKOUT", 0xFFCC: "CLRCHN", 0xFFCF: "CHRIN",
    0xFFD5: "LOAD", 0xFFD8: "SAVE", 0xFFB7: "READST",
}

# Internal KERNAL routines behind those entries. The serial group is what the
# payload uses to drive the IEC bus directly instead of going through OPEN/LOAD.
INTERNAL = {
    0xED09: "IEC_TALK", 0xED0C: "IEC_LISTEN", 0xEDB9: "IEC_SECOND",
    0xEDC7: "IEC_TKSA", 0xEDDD: "IEC_CIOUT", 0xEDEF: "IEC_UNTLK",
    0xEDFE: "IEC_UNLSN", 0xEE13: "IEC_ACPTR",
    0xFD15: "RESTOR", 0xFDA3: "IOINIT", 0xFF5B: "CINT",
    0xF642: "KERNAL_CLOSE", 0xEA31: "IRQ_ENTRY",
    # The CPU vectors at the top of the map. $FFFC holds the address of the
    # reset routine, so JMP ($FFFC) restarts the machine.
    0xFFFC: "KERNAL_RESET",
}

NAMES: dict[int, str] = {**JUMP_TABLE, **INTERNAL}


def name_for(addr: int) -> str | None:
    """Symbol for a C64 ROM address, or None if it is not in ROM space."""
    if addr in NAMES:
        return NAMES[addr]
    if addr >= 0xA000:
        return f"KERNAL_{addr:04X}"
    return None
