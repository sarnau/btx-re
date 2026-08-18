import pytest

from dis65xx.decode import decode
from dis65xx.encode import encode
from dis65xx.opcodes import TABLE, Mode


def test_encodes_known_instructions():
    assert encode("LDX", Mode.EXT, 0x00F0, addr=0xF129) == bytes([0xFE, 0x00, 0xF0])
    assert encode("JMP", Mode.IDX, 0x00, addr=0xF12C) == bytes([0x6E, 0x00])
    assert encode("RTI", Mode.INH, None, addr=0xF147) == bytes([0x3B])
    assert encode("LDS", Mode.IMM16, 0x0400, addr=0xB200) == bytes([0x8E, 0x04, 0x00])


def test_encodes_relative_branch_from_absolute_target():
    assert encode("BNE", Mode.REL, 0x9000, addr=0x9000) == bytes([0x26, 0xFE])
    assert encode("BRA", Mode.REL, 0x9012, addr=0x9000) == bytes([0x20, 0x10])


def test_relative_branch_out_of_range_raises():
    with pytest.raises(ValueError, match="out of range"):
        encode("BRA", Mode.REL, 0x9200, addr=0x9000)


def test_operand_too_large_raises():
    with pytest.raises(ValueError, match="does not fit"):
        encode("LDAA", Mode.IMM8, 0x100, addr=0x9000)


def test_every_table_entry_round_trips():
    """encode -> decode -> encode is a fixpoint for every opcode and mode."""
    addr = 0x9000
    for opcode, (mnemonic, mode) in sorted(TABLE.items()):
        operand = _sample_operand(mode, addr)
        raw = encode(mnemonic, mode, operand, addr=addr)
        assert raw[0] == opcode, f"${opcode:02X} {mnemonic} encoded as ${raw[0]:02X}"
        insn = decode(raw, 0, addr)
        assert insn.mnemonic == mnemonic
        assert insn.mode is mode
        assert insn.operand == operand
        assert encode(insn.mnemonic, insn.mode, insn.operand, addr=addr) == raw


def _sample_operand(mode: Mode, addr: int) -> int | None:
    if mode is Mode.INH:
        return None
    if mode in (Mode.IMM8, Mode.DIR, Mode.IDX):
        return 0x42
    if mode in (Mode.IMM16, Mode.EXT):
        return 0x1234
    if mode is Mode.REL:
        return addr + 0x10
    raise AssertionError(mode)
