"""The MC6801 opcode table.

Pure data. Cross-checked against MC6801RM_AD2_MC6801_Reference_Manual_May84.pdf.

Opcode map layout, inherited from the 6800:
    $00-$1F  inherent / misc          $80-$8F  immediate, accumulator A
    $20-$2F  relative branches        $90-$9F  direct,    accumulator A
    $30-$3F  inherent (stack etc.)    $A0-$AF  indexed,   accumulator A
    $40-$4F  inherent, accumulator A  $B0-$BF  extended,  accumulator A
    $50-$5F  inherent, accumulator B  $C0-$CF  immediate, accumulator B
    $60-$6F  indexed,  memory         $D0-$DF  direct,    accumulator B
    $70-$7F  extended, memory         $E0-$EF  indexed,   accumulator B
                                      $F0-$FF  extended,  accumulator B
"""

from __future__ import annotations

import enum


class Mode(enum.Enum):
    INH = "inh"      # inherent
    IMM8 = "imm8"    # immediate, 8-bit
    IMM16 = "imm16"  # immediate, 16-bit
    DIR = "dir"      # direct (zero page)
    IDX = "idx"      # indexed, unsigned 8-bit offset from X
    EXT = "ext"      # extended
    REL = "rel"      # relative, signed 8-bit


SIZE: dict[Mode, int] = {
    Mode.INH: 1,
    Mode.IMM8: 2,
    Mode.IMM16: 3,
    Mode.DIR: 2,
    Mode.IDX: 2,
    Mode.EXT: 3,
    Mode.REL: 2,
}

_I = Mode.INH
_M8 = Mode.IMM8
_M16 = Mode.IMM16
_D = Mode.DIR
_X = Mode.IDX
_E = Mode.EXT
_R = Mode.REL

TABLE: dict[int, tuple[str, Mode]] = {
    # $00-$1F
    0x01: ("NOP", _I),
    0x04: ("LSRD", _I),   # 6801
    0x05: ("ASLD", _I),   # 6801
    0x06: ("TAP", _I),
    0x07: ("TPA", _I),
    0x08: ("INX", _I),
    0x09: ("DEX", _I),
    0x0A: ("CLV", _I),
    0x0B: ("SEV", _I),
    0x0C: ("CLC", _I),
    0x0D: ("SEC", _I),
    0x0E: ("CLI", _I),
    0x0F: ("SEI", _I),
    0x10: ("SBA", _I),
    0x11: ("CBA", _I),
    0x16: ("TAB", _I),
    0x17: ("TBA", _I),
    0x19: ("DAA", _I),
    0x1B: ("ABA", _I),
    # $20-$2F relative branches
    0x20: ("BRA", _R),
    0x21: ("BRN", _R),    # 6801
    0x22: ("BHI", _R),
    0x23: ("BLS", _R),
    0x24: ("BCC", _R),
    0x25: ("BCS", _R),
    0x26: ("BNE", _R),
    0x27: ("BEQ", _R),
    0x28: ("BVC", _R),
    0x29: ("BVS", _R),
    0x2A: ("BPL", _R),
    0x2B: ("BMI", _R),
    0x2C: ("BGE", _R),
    0x2D: ("BLT", _R),
    0x2E: ("BGT", _R),
    0x2F: ("BLE", _R),
    # $30-$3F
    0x30: ("TSX", _I),
    0x31: ("INS", _I),
    0x32: ("PULA", _I),
    0x33: ("PULB", _I),
    0x34: ("DES", _I),
    0x35: ("TXS", _I),
    0x36: ("PSHA", _I),
    0x37: ("PSHB", _I),
    0x38: ("PULX", _I),   # 6801
    0x39: ("RTS", _I),
    0x3A: ("ABX", _I),    # 6801
    0x3B: ("RTI", _I),
    0x3C: ("PSHX", _I),   # 6801
    0x3D: ("MUL", _I),    # 6801
    0x3E: ("WAI", _I),
    0x3F: ("SWI", _I),
}


def _fill_unary(base: int, mode: Mode, suffix: str) -> None:
    """The $40/$50/$60/$70 quadrant shares one layout."""
    for lo, mnem in [
        (0x0, "NEG"), (0x3, "COM"), (0x4, "LSR"), (0x6, "ROR"),
        (0x7, "ASR"), (0x8, "ASL"), (0x9, "ROL"), (0xA, "DEC"),
        (0xC, "INC"), (0xD, "TST"), (0xF, "CLR"),
    ]:
        TABLE[base + lo] = (mnem + suffix, mode)


_fill_unary(0x40, _I, "A")
_fill_unary(0x50, _I, "B")
_fill_unary(0x60, _X, "")
_fill_unary(0x70, _E, "")
TABLE[0x6E] = ("JMP", _X)
TABLE[0x7E] = ("JMP", _E)


def _fill_accumulator(base: int, mode: Mode, wide: Mode, acc: str) -> None:
    """The $80..$FF quadrants share one layout, parameterised by accumulator."""
    d = "A" if acc == "A" else "B"
    entries: list[tuple[int, str, Mode]] = [
        (0x0, "SUB" + d, mode),
        (0x1, "CMP" + d, mode),
        (0x2, "SBC" + d, mode),
        (0x3, "SUBD" if acc == "A" else "ADDD", wide),
        (0x4, "AND" + d, mode),
        (0x5, "BIT" + d, mode),
        (0x6, "LDA" + d, mode),
        (0x7, "STA" + d, mode),
        (0x8, "EOR" + d, mode),
        (0x9, "ADC" + d, mode),
        (0xA, "ORA" + d, mode),
        (0xB, "ADD" + d, mode),
    ]
    for lo, mnem, m in entries:
        # STAA/STAB have no immediate form.
        if lo == 0x7 and mode is Mode.IMM8:
            continue
        TABLE[base + lo] = (mnem, m)


# Accumulator A quadrants. $8x immediate, $9x direct, $Ax indexed, $Bx extended.
_fill_accumulator(0x80, _M8, _M16, "A")
_fill_accumulator(0x90, _D, _D, "A")
_fill_accumulator(0xA0, _X, _X, "A")
_fill_accumulator(0xB0, _E, _E, "A")
# Accumulator B quadrants.
_fill_accumulator(0xC0, _M8, _M16, "B")
_fill_accumulator(0xD0, _D, _D, "B")
_fill_accumulator(0xE0, _X, _X, "B")
_fill_accumulator(0xF0, _E, _E, "B")

# The $xC..$xF columns differ between the A and B halves.
TABLE[0x8C] = ("CPX", _M16)
TABLE[0x8D] = ("BSR", _R)
TABLE[0x8E] = ("LDS", _M16)
TABLE[0x9C] = ("CPX", _D)
TABLE[0x9D] = ("JSR", _D)     # 6801
TABLE[0x9E] = ("LDS", _D)
TABLE[0x9F] = ("STS", _D)
TABLE[0xAC] = ("CPX", _X)
TABLE[0xAD] = ("JSR", _X)
TABLE[0xAE] = ("LDS", _X)
TABLE[0xAF] = ("STS", _X)
TABLE[0xBC] = ("CPX", _E)
TABLE[0xBD] = ("JSR", _E)
TABLE[0xBE] = ("LDS", _E)
TABLE[0xBF] = ("STS", _E)

TABLE[0xCC] = ("LDD", _M16)   # 6801
TABLE[0xCE] = ("LDX", _M16)
TABLE[0xDC] = ("LDD", _D)     # 6801
TABLE[0xDD] = ("STD", _D)     # 6801
TABLE[0xDE] = ("LDX", _D)
TABLE[0xDF] = ("STX", _D)
TABLE[0xEC] = ("LDD", _X)     # 6801
TABLE[0xED] = ("STD", _X)     # 6801
TABLE[0xEE] = ("LDX", _X)
TABLE[0xEF] = ("STX", _X)
TABLE[0xFC] = ("LDD", _E)     # 6801
TABLE[0xFD] = ("STD", _E)     # 6801
TABLE[0xFE] = ("LDX", _E)
TABLE[0xFF] = ("STX", _E)


_MODES_BY_MNEMONIC: dict[str, set[Mode]] = {}
for _op, (_mnem, _mode) in TABLE.items():
    _MODES_BY_MNEMONIC.setdefault(_mnem, set()).add(_mode)

_OPCODE_BY_KEY: dict[tuple[str, Mode], int] = {
    (mnem, mode): op for op, (mnem, mode) in TABLE.items()
}


def modes_for(mnemonic: str) -> set[Mode]:
    """Addressing modes available for a mnemonic. Empty set if unknown."""
    return _MODES_BY_MNEMONIC.get(mnemonic, set())


def opcode_for(mnemonic: str, mode: Mode) -> int | None:
    """Opcode byte for a mnemonic/mode pair, or None if that pair does not exist."""
    return _OPCODE_BY_KEY.get((mnemonic, mode))
