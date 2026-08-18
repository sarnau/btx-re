import pathlib
import re

from dis65xx.emit import emit, format_operand
from dis65xx.decode import decode
from dis65xx.opcodes import Mode
from dis65xx.sidecar import Sidecar
from dis65xx.trace import trace

OUT = pathlib.Path(__file__).resolve().parent.parent / "out"


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


def test_dispatch_tables_name_their_handlers():
    """Every slot of a ptr_table is a handler address. Showing them as hex hid
    the one thing the table is for."""
    src = (OUT / "btx_decoder_ii.asm").read_text()
    assert "FDB     parseNextByte,ctrlIgnored,ctrlIgnored" in src
    assert "FDB     stubSci,stubTof,stubOcf,stubIcf,stubIrq1,stubSwi," in src
    assert "FDB     asciiBS,asciiHT,asciiLF,asciiVT,asciiFF,asciiCR," in src
    # the screen line tables hold display RAM, so their entries resolve to a
    # plane and an offset rather than to code
    assert "FDB     planeAttr,planeAttr+160,planeAttr+320" in src
    # only data symbols take an offset - a code label plus one would be naming
    # a point inside an instruction
    offsets = set(re.findall(r"\b([A-Za-z_][A-Za-z0-9_]*)\+\d+", src))
    assert offsets <= {"planeRender", "planeAttr", "planeChar", "planeAccent"}, \
        sorted(offsets)


def test_string_regions_render_as_text():
    """These are ASCII, and 20-byte padded records - so one record per line."""
    src = (OUT / "btx_decoder_ii.asm").read_text()
    assert 'FCC     "CEPT Bildschirmtext "' in src
    assert 'FCC     "ASCII Terminal-Mode "' in src
    assert 'FCC     "Decodersoftware V3.3"' in src
    assert "FCB     $43,$45,$50,$54" not in src, "still emitting the text as bytes"


def test_address_immediates_take_names_but_constants_do_not():
    """LDX #table / ABX is an address; ADDD #$0002 is arithmetic. The 6801
    register file sits below $0100, so small constants collide with it."""
    src = (OUT / "btx_decoder_ii.asm").read_text()
    assert "LDX     #lineAddrChar" in src
    assert "LDD     #nullHandler" in src
    assert "ADDD    #$0002" in src and "ADDD    #PORT1" not in src
    assert "LDX     #$0000" in src and "LDX     #P1DDR" not in src


def test_literal_immediates_are_left_as_numbers():
    """$E4FF takes a two-byte parameter pair in X and stores it with STX, so
    the $4000 its callers load is not planeRender."""
    src = (OUT / "btx_decoder_ii.asm").read_text()
    body = src.split("c1aEBX:", 1)[1][:120]
    assert "LDX     #$4000" in body and "planeRender" not in body


def test_clear_planes_lands_on_an_instruction():
    """A label placed mid-instruction is emitted as FCB with an 'overlapped'
    note - which is how a wrong address announces itself."""
    src = (OUT / "btx_decoder_ii.asm").read_text()
    body = src.split("clearPlanes:", 1)[1][:80]
    assert "FCB" not in body, body


def test_drcs_buffers_are_named_across_their_whole_span():
    """The 102 bytes drcsDefineChar clears are the DRCS layout exactly: the
    row pair, four plane selectors, and four 24-byte planes."""
    src = (OUT / "btx_decoder_ii.asm").read_text()
    for name, addr in (("drcsRowHi", 0x0420), ("drcsSel0", 0x0422),
                       ("drcsPlane0", 0x0426), ("drcsPlane3", 0x046E),
                       ("drcsOffset", 0x0487)):
        assert re.search(rf"^{name}\s+EQU\s+\${addr:04X}$", src, re.M), name
    assert "LDX     #drcsPlane1" in src
    assert "ROR     drcsRowLo" in src and "ASR     drcsRowLo" in src
