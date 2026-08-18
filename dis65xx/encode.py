"""Single-instruction encoding, the inverse of dis65xx.decode."""

from __future__ import annotations

from dis65xx.opcodes import SIZE, Mode, opcode_for


def encode(mnemonic: str, mode: Mode, operand: int | None, *, addr: int) -> bytes:
    """Encode one instruction placed at `addr`.

    For Mode.REL, `operand` is the absolute branch target, matching Insn.operand.
    """
    opcode = opcode_for(mnemonic, mode)
    if opcode is None:
        raise ValueError(f"no opcode for {mnemonic} {mode.value}")

    if mode is Mode.INH:
        if operand is not None:
            raise ValueError(f"{mnemonic} takes no operand")
        return bytes([opcode])

    if operand is None:
        raise ValueError(f"{mnemonic} {mode.value} requires an operand")

    if mode in (Mode.IMM8, Mode.DIR, Mode.IDX):
        if not 0 <= operand <= 0xFF:
            raise ValueError(f"${operand:X} does not fit in {mode.value} for {mnemonic}")
        return bytes([opcode, operand])

    if mode in (Mode.IMM16, Mode.EXT):
        if not 0 <= operand <= 0xFFFF:
            raise ValueError(f"${operand:X} does not fit in {mode.value} for {mnemonic}")
        return bytes([opcode]) + operand.to_bytes(2, "big")

    if mode is Mode.REL:
        delta = operand - (addr + SIZE[mode])
        if not -128 <= delta <= 127:
            raise ValueError(
                f"{mnemonic} to ${operand:04X} from ${addr:04X} is out of range ({delta})"
            )
        return bytes([opcode, delta & 0xFF])

    raise AssertionError(f"unhandled mode {mode}")  # pragma: no cover
