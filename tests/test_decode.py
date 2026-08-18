import pytest

from conftest import ROM_BASE
from dis65xx.decode import Insn, decode
from dis65xx.opcodes import Mode


def test_decodes_the_sci_vector_stub():
    # $F129: FE 00 F0  LDX $00F0 / 6E 00  JMP 0,X
    data = bytes([0xFE, 0x00, 0xF0, 0x6E, 0x00])
    first = decode(data, 0, 0xF129)
    assert first == Insn(addr=0xF129, opcode=0xFE, mnemonic="LDX",
                         mode=Mode.EXT, operand=0x00F0, size=3)
    second = decode(data, 3, 0xF12C)
    assert second == Insn(addr=0xF12C, opcode=0x6E, mnemonic="JMP",
                          mode=Mode.IDX, operand=0x00, size=2)


def test_decodes_relative_branch_operand_as_signed_target():
    # $9000: 26 FE  BNE $9000  (offset -2, branches to itself)
    insn = decode(bytes([0x26, 0xFE]), 0, 0x9000)
    assert insn.mnemonic == "BNE"
    assert insn.mode is Mode.REL
    assert insn.operand == 0x9000  # pc-after-insn + (-2)


def test_decodes_forward_relative_branch():
    insn = decode(bytes([0x20, 0x10]), 0, 0x9000)
    assert insn.operand == 0x9012


def test_decodes_reset_entry_from_the_real_rom(rom):
    addr = 0xB200
    insn = decode(rom, addr - ROM_BASE, addr)
    assert insn.mnemonic == "LDS"
    assert insn.mode is Mode.IMM16
    assert insn.operand == 0x0400
    assert insn.size == 3


def test_illegal_opcode_raises():
    with pytest.raises(ValueError, match=r"\$00"):
        decode(bytes([0x00, 0x00]), 0, 0x8000)


def test_truncated_instruction_raises():
    with pytest.raises(ValueError, match="truncated"):
        decode(bytes([0xBD, 0xF1]), 0, 0x8000)
