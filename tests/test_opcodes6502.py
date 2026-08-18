from dis65xx.opcodes6502 import SIZE, TABLE, Mode, modes_for, opcode_for


def test_encodings_from_the_c64_bootstrap():
    # Hand-decoded from $B32D, the cartridge cold-start.
    assert TABLE[0x20] == ("JSR", Mode.ABS)
    assert TABLE[0xA2] == ("LDX", Mode.IMM)
    assert TABLE[0x8E] == ("STX", Mode.ABS)
    assert TABLE[0x6C] == ("JMP", Mode.IND)
    assert TABLE[0x91] == ("STA", Mode.IZY)
    assert TABLE[0x4C] == ("JMP", Mode.ABS)
    assert TABLE[0xD0] == ("BNE", Mode.REL)
    assert TABLE[0xE6] == ("INC", Mode.ZP)
    assert TABLE[0x85] == ("STA", Mode.ZP)
    assert TABLE[0xC8] == ("INY", Mode.IMP)
    assert TABLE[0xB0] == ("BCS", Mode.REL)
    assert TABLE[0x60] == ("RTS", Mode.IMP)


def test_accumulator_and_implied_are_distinct():
    assert TABLE[0x0A] == ("ASL", Mode.ACC)
    assert TABLE[0xEA] == ("NOP", Mode.IMP)


def test_legal_opcode_count():
    # The NMOS 6502 has exactly 151 documented opcodes.
    assert len(TABLE) == 151


def test_every_entry_has_a_size():
    for op, (mnem, mode) in TABLE.items():
        assert mode in SIZE, f"${op:02X} {mnem}"


def test_modes_and_opcode_lookup_round_trip():
    assert Mode.ZP in modes_for("STA")
    assert Mode.ABS in modes_for("STA")
    assert opcode_for("STA", Mode.ABS) == 0x8D
    assert opcode_for("STA", Mode.IMM) is None


def test_no_illegal_opcodes_present():
    for op in (0x02, 0x03, 0x07, 0x0B, 0x1A, 0xFF):
        assert op not in TABLE
