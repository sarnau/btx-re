from dis6801.opcodes import Mode, SIZE, TABLE, modes_for


def test_known_encodings_from_the_rom():
    # Every one of these was hand-decoded from the reset path and vector stubs.
    assert TABLE[0xFE] == ("LDX", Mode.EXT)
    assert TABLE[0x6E] == ("JMP", Mode.IDX)
    assert TABLE[0x3B] == ("RTI", Mode.INH)
    assert TABLE[0x8E] == ("LDS", Mode.IMM16)
    assert TABLE[0x86] == ("LDAA", Mode.IMM8)
    assert TABLE[0x97] == ("STAA", Mode.DIR)
    assert TABLE[0x7F] == ("CLR", Mode.EXT)
    assert TABLE[0xCC] == ("LDD", Mode.IMM16)
    assert TABLE[0xFD] == ("STD", Mode.EXT)
    assert TABLE[0xBD] == ("JSR", Mode.EXT)
    assert TABLE[0xDC] == ("LDD", Mode.DIR)
    assert TABLE[0xC3] == ("ADDD", Mode.IMM16)


def test_6801_only_opcodes_present():
    for opcode, mnem in [
        (0x04, "LSRD"), (0x05, "ASLD"), (0x38, "PULX"), (0x3A, "ABX"),
        (0x3C, "PSHX"), (0x3D, "MUL"), (0x21, "BRN"), (0x9D, "JSR"),
    ]:
        assert TABLE[opcode][0] == mnem


def test_every_entry_has_a_known_size():
    for opcode, (mnem, mode) in TABLE.items():
        assert mode in SIZE, f"${opcode:02X} {mnem} has no size"


def test_modes_for_reports_available_modes():
    assert modes_for("STD") == {Mode.DIR, Mode.EXT, Mode.IDX}
    assert modes_for("JMP") == {Mode.IDX, Mode.EXT}
    assert Mode.DIR not in modes_for("CLR")


def test_opcode_map_is_dense_enough():
    # The 6801 leaves a scattering of holes; anything below this means the
    # table is incomplete rather than faithful.
    assert len(TABLE) >= 195
