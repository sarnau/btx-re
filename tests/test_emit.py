from dis65xx.emit import emit, format_operand
from dis65xx.decode import decode
from dis65xx.opcodes import Mode
from dis65xx.sidecar import Sidecar
from dis65xx.trace import trace


def _sidecar(**kw) -> Sidecar:
    defaults = dict(rom="x", base=0x8000, entry_points=[0x8000], labels={},
                    line_comments={}, block_comments={}, symbols={}, regions=[])
    defaults.update(kw)
    return Sidecar(**defaults)


def test_forces_extended_when_the_operand_would_fit_in_direct():
    # FD 00 F2 is STD extended on $00F2; a shortest-fit assembler would pick
    # direct, so the emitter must force it.
    insn = decode(bytes([0xFD, 0x00, 0xF2]), 0, 0x8000)
    assert format_operand(insn, _sidecar()) == ">$00F2"


def test_does_not_force_extended_when_direct_is_impossible():
    # CLR has no direct form, so plain $0000 is unambiguous.
    insn = decode(bytes([0x7F, 0x00, 0x00]), 0, 0x8000)
    assert format_operand(insn, _sidecar()) == "$0000"


def test_does_not_force_extended_for_large_operands():
    insn = decode(bytes([0xBD, 0xF1, 0xD8]), 0, 0x8000)
    assert format_operand(insn, _sidecar()) == "$F1D8"


def test_uses_symbols_for_direct_operands():
    insn = decode(bytes([0x97, 0x02]), 0, 0x8000)
    assert format_operand(insn, _sidecar(symbols={0x02: "PORT1"})) == "PORT1"


def test_formats_immediate_and_indexed():
    assert format_operand(decode(bytes([0x86, 0x10]), 0, 0x8000), _sidecar()) == "#$10"
    assert format_operand(decode(bytes([0x8E, 0x04, 0x00]), 0, 0x8000), _sidecar()) == "#$0400"
    assert format_operand(decode(bytes([0x6E, 0x00]), 0, 0x8000), _sidecar()) == "$00,X"


def test_emits_header_labels_comments_and_data():
    #   $8000: 86 01     LDAA #$01
    #   $8002: 39        RTS
    #   $8003: FF        unreached -> FCB
    data = bytes([0x86, 0x01, 0x39, 0xFF])
    sc = _sidecar(labels={0x8000: "start"}, line_comments={0x8000: "load one"},
                  block_comments={0x8000: "Entry point."})
    result = trace(data, base=0x8000, entry_points=[0x8000])
    text = emit(data, result, sc)
    assert "CPU     6801" in text
    assert "ORG     $8000" in text
    assert "start:" in text
    assert "; Entry point." in text
    assert "LDAA    #$01" in text
    assert "; load one" in text
    assert "FCB     $FF" in text
    assert text.rstrip().endswith("END")


def test_unreached_bytes_are_grouped_sixteen_per_line():
    data = bytes([0x39]) + bytes(40)
    result = trace(data, base=0x8000, entry_points=[0x8000])
    text = emit(data, result, _sidecar())
    fcb_lines = [ln for ln in text.splitlines() if "FCB" in ln]
    assert len(fcb_lines) == 3  # 16 + 16 + 8
