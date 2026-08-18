"""The NMOS 6502 opcode table.

The ROM embeds a C64-side 6502 program alongside the 68B01 firmware (see the
$B32D block comment in the sidecar), so the toolchain needs both instruction
sets. This module mirrors dis6801.opcodes: pure data plus lookup helpers.

Only the 151 documented opcodes are listed. Undocumented ones stay absent so a
sweep over data fails loudly rather than inventing instructions.
"""

from __future__ import annotations

import enum


class Mode(enum.Enum):
    IMP = "imp"   # implied
    ACC = "acc"   # accumulator
    IMM = "imm"   # #$xx
    ZP = "zp"     # $xx
    ZPX = "zpx"   # $xx,X
    ZPY = "zpy"   # $xx,Y
    ABS = "abs"   # $xxxx
    ABX = "abx"   # $xxxx,X
    ABY = "aby"   # $xxxx,Y
    IND = "ind"   # ($xxxx)
    IZX = "izx"   # ($xx,X)
    IZY = "izy"   # ($xx),Y
    REL = "rel"   # branch


SIZE: dict[Mode, int] = {
    Mode.IMP: 1, Mode.ACC: 1,
    Mode.IMM: 2, Mode.ZP: 2, Mode.ZPX: 2, Mode.ZPY: 2,
    Mode.IZX: 2, Mode.IZY: 2, Mode.REL: 2,
    Mode.ABS: 3, Mode.ABX: 3, Mode.ABY: 3, Mode.IND: 3,
}

_M = Mode
TABLE: dict[int, tuple[str, Mode]] = {
    0x00: ("BRK", _M.IMP), 0x01: ("ORA", _M.IZX), 0x05: ("ORA", _M.ZP),
    0x06: ("ASL", _M.ZP),  0x08: ("PHP", _M.IMP), 0x09: ("ORA", _M.IMM),
    0x0A: ("ASL", _M.ACC), 0x0D: ("ORA", _M.ABS), 0x0E: ("ASL", _M.ABS),
    0x10: ("BPL", _M.REL), 0x11: ("ORA", _M.IZY), 0x15: ("ORA", _M.ZPX),
    0x16: ("ASL", _M.ZPX), 0x18: ("CLC", _M.IMP), 0x19: ("ORA", _M.ABY),
    0x1D: ("ORA", _M.ABX), 0x1E: ("ASL", _M.ABX),
    0x20: ("JSR", _M.ABS), 0x21: ("AND", _M.IZX), 0x24: ("BIT", _M.ZP),
    0x25: ("AND", _M.ZP),  0x26: ("ROL", _M.ZP),  0x28: ("PLP", _M.IMP),
    0x29: ("AND", _M.IMM), 0x2A: ("ROL", _M.ACC), 0x2C: ("BIT", _M.ABS),
    0x2D: ("AND", _M.ABS), 0x2E: ("ROL", _M.ABS),
    0x30: ("BMI", _M.REL), 0x31: ("AND", _M.IZY), 0x35: ("AND", _M.ZPX),
    0x36: ("ROL", _M.ZPX), 0x38: ("SEC", _M.IMP), 0x39: ("AND", _M.ABY),
    0x3D: ("AND", _M.ABX), 0x3E: ("ROL", _M.ABX),
    0x40: ("RTI", _M.IMP), 0x41: ("EOR", _M.IZX), 0x45: ("EOR", _M.ZP),
    0x46: ("LSR", _M.ZP),  0x48: ("PHA", _M.IMP), 0x49: ("EOR", _M.IMM),
    0x4A: ("LSR", _M.ACC), 0x4C: ("JMP", _M.ABS), 0x4D: ("EOR", _M.ABS),
    0x4E: ("LSR", _M.ABS),
    0x50: ("BVC", _M.REL), 0x51: ("EOR", _M.IZY), 0x55: ("EOR", _M.ZPX),
    0x56: ("LSR", _M.ZPX), 0x58: ("CLI", _M.IMP), 0x59: ("EOR", _M.ABY),
    0x5D: ("EOR", _M.ABX), 0x5E: ("LSR", _M.ABX),
    0x60: ("RTS", _M.IMP), 0x61: ("ADC", _M.IZX), 0x65: ("ADC", _M.ZP),
    0x66: ("ROR", _M.ZP),  0x68: ("PLA", _M.IMP), 0x69: ("ADC", _M.IMM),
    0x6A: ("ROR", _M.ACC), 0x6C: ("JMP", _M.IND), 0x6D: ("ADC", _M.ABS),
    0x6E: ("ROR", _M.ABS),
    0x70: ("BVS", _M.REL), 0x71: ("ADC", _M.IZY), 0x75: ("ADC", _M.ZPX),
    0x76: ("ROR", _M.ZPX), 0x78: ("SEI", _M.IMP), 0x79: ("ADC", _M.ABY),
    0x7D: ("ADC", _M.ABX), 0x7E: ("ROR", _M.ABX),
    0x81: ("STA", _M.IZX), 0x84: ("STY", _M.ZP),  0x85: ("STA", _M.ZP),
    0x86: ("STX", _M.ZP),  0x88: ("DEY", _M.IMP), 0x8A: ("TXA", _M.IMP),
    0x8C: ("STY", _M.ABS), 0x8D: ("STA", _M.ABS), 0x8E: ("STX", _M.ABS),
    0x90: ("BCC", _M.REL), 0x91: ("STA", _M.IZY), 0x94: ("STY", _M.ZPX),
    0x95: ("STA", _M.ZPX), 0x96: ("STX", _M.ZPY), 0x98: ("TYA", _M.IMP),
    0x99: ("STA", _M.ABY), 0x9A: ("TXS", _M.IMP), 0x9D: ("STA", _M.ABX),
    0xA0: ("LDY", _M.IMM), 0xA1: ("LDA", _M.IZX), 0xA2: ("LDX", _M.IMM),
    0xA4: ("LDY", _M.ZP),  0xA5: ("LDA", _M.ZP),  0xA6: ("LDX", _M.ZP),
    0xA8: ("TAY", _M.IMP), 0xA9: ("LDA", _M.IMM), 0xAA: ("TAX", _M.IMP),
    0xAC: ("LDY", _M.ABS), 0xAD: ("LDA", _M.ABS), 0xAE: ("LDX", _M.ABS),
    0xB0: ("BCS", _M.REL), 0xB1: ("LDA", _M.IZY), 0xB4: ("LDY", _M.ZPX),
    0xB5: ("LDA", _M.ZPX), 0xB6: ("LDX", _M.ZPY), 0xB8: ("CLV", _M.IMP),
    0xB9: ("LDA", _M.ABY), 0xBA: ("TSX", _M.IMP), 0xBC: ("LDY", _M.ABX),
    0xBD: ("LDA", _M.ABX), 0xBE: ("LDX", _M.ABY),
    0xC0: ("CPY", _M.IMM), 0xC1: ("CMP", _M.IZX), 0xC4: ("CPY", _M.ZP),
    0xC5: ("CMP", _M.ZP),  0xC6: ("DEC", _M.ZP),  0xC8: ("INY", _M.IMP),
    0xC9: ("CMP", _M.IMM), 0xCA: ("DEX", _M.IMP), 0xCC: ("CPY", _M.ABS),
    0xCD: ("CMP", _M.ABS), 0xCE: ("DEC", _M.ABS),
    0xD0: ("BNE", _M.REL), 0xD1: ("CMP", _M.IZY), 0xD5: ("CMP", _M.ZPX),
    0xD6: ("DEC", _M.ZPX), 0xD8: ("CLD", _M.IMP), 0xD9: ("CMP", _M.ABY),
    0xDD: ("CMP", _M.ABX), 0xDE: ("DEC", _M.ABX),
    0xE0: ("CPX", _M.IMM), 0xE1: ("SBC", _M.IZX), 0xE4: ("CPX", _M.ZP),
    0xE5: ("SBC", _M.ZP),  0xE6: ("INC", _M.ZP),  0xE8: ("INX", _M.IMP),
    0xE9: ("SBC", _M.IMM), 0xEA: ("NOP", _M.IMP), 0xEC: ("CPX", _M.ABS),
    0xED: ("SBC", _M.ABS), 0xEE: ("INC", _M.ABS),
    0xF0: ("BEQ", _M.REL), 0xF1: ("SBC", _M.IZY), 0xF5: ("SBC", _M.ZPX),
    0xF6: ("INC", _M.ZPX), 0xF8: ("SED", _M.IMP), 0xF9: ("SBC", _M.ABY),
    0xFD: ("SBC", _M.ABX), 0xFE: ("INC", _M.ABX),
}

_MODES: dict[str, set[Mode]] = {}
for _op, (_m, _mode) in TABLE.items():
    _MODES.setdefault(_m, set()).add(_mode)
_BY_KEY = {(m, mode): op for op, (m, mode) in TABLE.items()}


def modes_for(mnemonic: str) -> set[Mode]:
    return _MODES.get(mnemonic, set())


def opcode_for(mnemonic: str, mode: Mode) -> int | None:
    return _BY_KEY.get((mnemonic, mode))
