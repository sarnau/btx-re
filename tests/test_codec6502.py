import pytest

from conftest import ROM_BASE
from dis65xx.codec6502 import Insn6502, decode, encode
from dis65xx.opcodes6502 import Mode


def test_decodes_the_cartridge_bootstrap(rom):
    # $B32D+ is the C64 cold-start; these were hand-decoded earlier.
    def at(addr):
        return decode(rom, addr - ROM_BASE, addr)

    assert at(0xB340).mnemonic == "LDX" and at(0xB340).operand == 0x00
    i = at(0xB342)
    assert (i.mnemonic, i.mode, i.operand, i.size) == ("STX", Mode.ABS, 0xD016, 3)
    i = at(0xB345)
    assert (i.mnemonic, i.mode, i.operand) == ("JSR", Mode.ABS, 0xFDA3)
    i = at(0xB368)
    assert (i.mnemonic, i.mode, i.operand) == ("STA", Mode.IZY, 0x61)
    i = at(0xB36F)
    assert (i.mnemonic, i.mode, i.operand) == ("JMP", Mode.ABS, 0x8036)


def test_relative_branch_target_is_absolute():
    # $B36B: D0 F6  BNE -10  -> $B363
    i = decode(bytes([0xD0, 0xF6]), 0, 0xB36B)
    assert i.mode is Mode.REL
    assert i.operand == 0xB363


def test_indirect_jump():
    i = decode(bytes([0x6C, 0x00, 0x80]), 0, 0xB377)
    assert (i.mnemonic, i.mode, i.operand) == ("JMP", Mode.IND, 0x8000)


def test_illegal_opcode_raises():
    with pytest.raises(ValueError, match=r"\$02"):
        decode(bytes([0x02, 0x00]), 0, 0x1000)


def test_encode_round_trips_every_opcode():
    from dis65xx.opcodes6502 import TABLE

    addr = 0x1000
    for op, (mnem, mode) in sorted(TABLE.items()):
        operand = _sample(mode, addr)
        raw = encode(mnem, mode, operand, addr=addr)
        assert raw[0] == op, f"${op:02X} {mnem} -> ${raw[0]:02X}"
        i = decode(raw, 0, addr)
        assert (i.mnemonic, i.mode, i.operand) == (mnem, mode, operand)
        assert encode(i.mnemonic, i.mode, i.operand, addr=addr) == raw


def _sample(mode: Mode, addr: int):
    if mode in (Mode.IMP, Mode.ACC):
        return None
    if mode is Mode.REL:
        return addr + 0x10
    if mode in (Mode.ABS, Mode.ABX, Mode.ABY, Mode.IND):
        return 0x1234
    return 0x42


def test_relative_out_of_range_raises():
    with pytest.raises(ValueError, match="out of range"):
        encode("BNE", Mode.REL, 0x1200, addr=0x1000)
