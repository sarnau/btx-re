"""6502 instruction decode and encode, mirroring dis6801.decode/encode.

Absolute operands are emitted verbatim. The embedded C64 payload is stored at
one address and runs at another, so its absolute references are runtime
addresses that do not correspond to positions in this ROM; keeping them literal
is what preserves byte-identity.
"""

from __future__ import annotations

import dataclasses

from dis6801.opcodes6502 import SIZE, TABLE, Mode, opcode_for


@dataclasses.dataclass(frozen=True)
class Insn6502:
    addr: int
    opcode: int
    mnemonic: str
    mode: Mode
    operand: int | None
    size: int

    @property
    def end(self) -> int:
        return self.addr + self.size


_ONE_BYTE_OPERAND = (Mode.IMM, Mode.ZP, Mode.ZPX, Mode.ZPY, Mode.IZX, Mode.IZY)
_TWO_BYTE_OPERAND = (Mode.ABS, Mode.ABX, Mode.ABY, Mode.IND)


def decode(data: bytes, offset: int, addr: int) -> Insn6502:
    if offset >= len(data):
        raise ValueError(f"truncated instruction at ${addr:04X}: past end of image")
    opcode = data[offset]
    entry = TABLE.get(opcode)
    if entry is None:
        raise ValueError(f"illegal opcode ${opcode:02X} at ${addr:04X}")
    mnemonic, mode = entry
    size = SIZE[mode]
    if offset + size > len(data):
        raise ValueError(f"truncated instruction ${opcode:02X} at ${addr:04X}")

    if mode in (Mode.IMP, Mode.ACC):
        operand = None
    elif mode in _ONE_BYTE_OPERAND:
        operand = data[offset + 1]
    elif mode in _TWO_BYTE_OPERAND:
        operand = int.from_bytes(data[offset + 1:offset + 3], "little")
    elif mode is Mode.REL:
        delta = data[offset + 1]
        if delta >= 0x80:
            delta -= 0x100
        operand = (addr + size + delta) & 0xFFFF
    else:  # pragma: no cover
        raise AssertionError(mode)

    return Insn6502(addr=addr, opcode=opcode, mnemonic=mnemonic,
                    mode=mode, operand=operand, size=size)


def encode(mnemonic: str, mode: Mode, operand: int | None, *, addr: int) -> bytes:
    opcode = opcode_for(mnemonic, mode)
    if opcode is None:
        raise ValueError(f"no opcode for {mnemonic} {mode.value}")
    if mode in (Mode.IMP, Mode.ACC):
        if operand is not None:
            raise ValueError(f"{mnemonic} takes no operand")
        return bytes([opcode])
    if operand is None:
        raise ValueError(f"{mnemonic} {mode.value} requires an operand")
    if mode in _ONE_BYTE_OPERAND:
        if not 0 <= operand <= 0xFF:
            raise ValueError(f"${operand:X} does not fit in {mode.value} for {mnemonic}")
        return bytes([opcode, operand])
    if mode in _TWO_BYTE_OPERAND:
        if not 0 <= operand <= 0xFFFF:
            raise ValueError(f"${operand:X} does not fit in {mode.value} for {mnemonic}")
        return bytes([opcode]) + operand.to_bytes(2, "little")
    if mode is Mode.REL:
        delta = operand - (addr + SIZE[mode])
        if not -128 <= delta <= 127:
            raise ValueError(
                f"{mnemonic} to ${operand:04X} from ${addr:04X} is out of range ({delta})")
        return bytes([opcode, delta & 0xFF])
    raise AssertionError(mode)  # pragma: no cover
