from conftest import ROM_BASE
from dis6801.trace import CODE, OPERAND, UNKNOWN, trace


def test_traces_a_straight_line_until_rts():
    # LDAA #$01 / RTS / FCB $FF
    data = bytes([0x86, 0x01, 0x39, 0xFF])
    result = trace(data, base=0x8000, entry_points=[0x8000])
    assert result.kind[0] is CODE     # LDAA opcode
    assert result.kind[1] is OPERAND  # its immediate byte
    assert result.kind[2] is CODE     # RTS opcode
    assert result.kind[3] is UNKNOWN  # unreached
    assert sorted(result.insns) == [0x8000, 0x8002]


def test_follows_a_conditional_branch_and_the_fallthrough():
    #   $8000: 26 02     BNE $8004
    #   $8002: 86 01     LDAA #$01
    #   $8004: 39        RTS
    data = bytes([0x26, 0x02, 0x86, 0x01, 0x39])
    result = trace(data, base=0x8000, entry_points=[0x8000])
    assert sorted(result.insns) == [0x8000, 0x8002, 0x8004]


def test_stops_at_unconditional_jump_but_follows_the_target():
    #   $8000: 7E 80 04  JMP $8004
    #   $8003: FF        (data, unreachable)
    #   $8004: 39        RTS
    data = bytes([0x7E, 0x80, 0x04, 0xFF, 0x39])
    result = trace(data, base=0x8000, entry_points=[0x8000])
    assert sorted(result.insns) == [0x8000, 0x8004]
    assert result.kind[3] is UNKNOWN


def test_follows_jsr_and_continues_after_it():
    #   $8000: BD 80 04  JSR $8004
    #   $8003: 39        RTS
    #   $8004: 39        RTS
    data = bytes([0xBD, 0x80, 0x04, 0x39, 0x39])
    result = trace(data, base=0x8000, entry_points=[0x8000])
    assert sorted(result.insns) == [0x8000, 0x8003, 0x8004]
    assert 0x8004 in result.call_targets


def test_indexed_jump_target_is_unresolved_not_an_error():
    #   $8000: 6E 00     JMP 0,X
    data = bytes([0x6E, 0x00])
    result = trace(data, base=0x8000, entry_points=[0x8000])
    assert sorted(result.insns) == [0x8000]
    assert result.unresolved == [0x8000]


def test_illegal_opcode_stops_that_path_and_is_recorded():
    #   $8000: 86 01     LDAA #$01
    #   $8002: 00        illegal
    data = bytes([0x86, 0x01, 0x00])
    result = trace(data, base=0x8000, entry_points=[0x8000])
    assert sorted(result.insns) == [0x8000]
    assert result.bad_opcodes == [0x8002]


def test_traces_the_real_rom_vector_stubs(rom):
    entries = [0xB200] + [0xF129 + 5 * i for i in range(6)] + [0xF147]
    result = trace(rom, base=ROM_BASE, entry_points=entries)
    # Each stub is LDX $00xx / JMP 0,X — two instructions, then unresolved.
    assert 0xF129 in result.insns
    assert 0xF12C in result.insns
    assert 0xF12C in result.unresolved
    # The reset path reaches real code, so coverage must be non-trivial.
    assert result.coverage()["code"] > 100
