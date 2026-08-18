"""Single-instruction decoding. No control-flow knowledge, no output formatting."""

from __future__ import annotations

import dataclasses

from dis65xx.opcodes import SIZE, TABLE, Mode


@dataclasses.dataclass(frozen=True)
class Insn:
    addr: int        # address of the opcode byte
    opcode: int
    mnemonic: str
    mode: Mode
    operand: int | None  # None for inherent; absolute target for REL
    size: int

    @property
    def end(self) -> int:
        return self.addr + self.size


def decode(data: bytes, offset: int, addr: int) -> Insn:
    """Decode one instruction at `data[offset]`, which lives at address `addr`."""
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

    if mode is Mode.INH:
        operand = None
    elif mode in (Mode.IMM8, Mode.DIR, Mode.IDX):
        operand = data[offset + 1]
    elif mode in (Mode.IMM16, Mode.EXT):
        operand = int.from_bytes(data[offset + 1:offset + 3], "big")
    elif mode is Mode.REL:
        delta = data[offset + 1]
        if delta >= 0x80:
            delta -= 0x100
        operand = (addr + size + delta) & 0xFFFF
    else:  # pragma: no cover - Mode is exhaustive
        raise AssertionError(f"unhandled mode {mode}")

    return Insn(addr=addr, opcode=opcode, mnemonic=mnemonic,
                mode=mode, operand=operand, size=size)
