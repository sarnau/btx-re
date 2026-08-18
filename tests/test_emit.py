import pathlib
import re

from dis65xx.emit import emit, format_operand
from dis65xx.decode import decode
from dis65xx.opcodes import Mode
from dis65xx.sidecar import Sidecar
from dis65xx.trace import trace

ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT = ROOT / "out"


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
    # Only data takes an offset. A code label plus one would be naming a point
    # inside an instruction, so the rule is asserted rather than a name list:
    # every base must be a symbol (an EQU) or a label sitting in a data region.
    import tomllib
    sidecar = tomllib.load(open(ROOT / "sidecar" / "decoder_ii.toml", "rb"))
    data = [(r["start"], r["end"]) for r in sidecar["regions"]
            if r["kind"] != "code"]
    labels = {v: int(k, 16) for k, v in sidecar["labels"].items()}
    symbols = set(sidecar["symbols"].values())
    # the emitter names each 6502 block itself; a block is BINCLUDEd data
    from dis65xx.emit import c64_block_label
    blocks = {c64_block_label(b["name"]): b["start"] for b in sidecar["c64_blocks"]}
    body = "\n".join(ln.split(";")[0] for ln in src.splitlines())
    for base in set(re.findall(r"\b([A-Za-z_][A-Za-z0-9_]*)[+-]\d+", body)):
        if base in symbols or base in blocks:
            continue
        a = labels.get(base)
        assert a is not None, base
        assert any(s <= a < e for s, e in data), f"{base} is a code label"


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


def test_site_symbols_name_each_use_of_a_shared_byte():
    """$048E is a mask complement, a cell offset and a fill value in three
    different routines, so the name is attached to the site."""
    src = (OUT / "btx_decoder_ii.asm").read_text()
    for name in ("attrMaskInv", "attrCellOffset", "attrFillHi"):
        assert re.search(rf"^{name}\s+EQU\s+\$048E$", src, re.M), name
    # no operand is left as the bare address - only the three EQUs mention it
    bare = [ln for ln in src.splitlines()
            if "$048E" in ln and not ln.lstrip().startswith(";")
            and "EQU" not in ln]
    assert not bare, bare
    assert "COMB\n        STAB    attrMaskInv" in src
    assert "ASLB\n        STAB    attrCellOffset" in src


def test_repeat_and_fill_share_a_byte_under_different_names():
    """$0490 is ctlRPT's countdown, ctlCAN's saved column and the row fill's
    index. Three roles, three names, one address."""
    src = (OUT / "btx_decoder_ii.asm").read_text()
    for name in ("repeatCount", "eraseStartCol", "fillCol"):
        assert re.search(rf"^{name}\s+EQU\s+\$0490$", src, re.M), name
    bare = [ln for ln in src.splitlines()
            if "$0490" in ln and not ln.lstrip().startswith(";")
            and "EQU" not in ln]
    assert not bare, bare
    assert "DEC     repeatCount" in src
    assert "INC     fillCol" in src


def test_site_symbols_reject_an_address_that_is_not_an_instruction():
    """A wrong site address must announce itself rather than silently doing
    nothing - the same failure mode a mid-instruction label has."""
    import dataclasses
    import pytest
    from dis65xx.sidecar import load_sidecar
    sc = load_sidecar(ROOT / "sidecar" / "decoder_ii.toml")
    rom = (ROOT / sc.rom).resolve().read_bytes()
    res = trace(rom, base=sc.base, entry_points=sc.entry_points)
    # $8000 is the middle of the font, never decoded as an instruction
    bad = dataclasses.replace(sc, site_symbols={**sc.site_symbols, 0x8001: "nope"})
    with pytest.raises(ValueError, match="not an instruction"):
        emit(rom, res, bad)


def test_c64_blocks_carry_a_label():
    """The 6502 sources define their own labels, but those live in a separate
    assembly - so the 6801 listing names each block itself."""
    src = (OUT / "btx_decoder_ii.asm").read_text()
    for label, inc in (("c64BootstrapBlock:", "c64_bootstrap.bin"),
                       ("c64PayloadBlock:", "c64_payload.bin")):
        i = src.index(label)
        assert f'BINCLUDE "{inc}"' in src[i:i + 200], label


def test_every_named_routine_is_documented():
    """A name says what something is called; the note says what it does. A
    routine that gains a name without one is the gap this catches."""
    import tomllib
    sidecar = tomllib.load(open(ROOT / "sidecar" / "decoder_ii.toml", "rb"))
    labels = {int(k, 16): v for k, v in sidecar["labels"].items()}
    documented = ({int(k, 16) for k in sidecar["block_comments"]}
                  | {int(k, 16) for k in sidecar["line_comments"]})
    src = (OUT / "btx_decoder_ii.asm").read_text()
    missing = []
    for addr, name in sorted(labels.items()):
        if addr < 0x8000 or name.startswith("orphan"):
            continue
        # a routine is a label the listing follows with an instruction
        m = re.search(rf"^{name}:\n(?:;.*\n)*\s+([A-Z]{{2,4}})", src, re.M)
        if not m or m.group(1) in ("FCB", "FCC", "FDB", "DW"):
            continue
        if addr not in documented:
            missing.append(f"${addr:04X} {name}")
    assert not missing, missing


def test_architecture_document_matches_the_sources():
    """Prose goes stale silently while the listings are regenerated under it,
    so the document is checked the same way the listings are."""
    import subprocess
    r = subprocess.run(["python3", str(ROOT / "tools" / "checkdoc.py")],
                       capture_output=True, text=True, cwd=ROOT)
    assert r.returncode == 0, r.stdout


def test_no_subroutine_is_left_unnamed():
    """A JSR target still called L<addr> is a function nobody has looked at."""
    for f in ("btx_decoder_ii.asm", "c64_payload.asm", "c64_bootstrap.asm"):
        src = (OUT / f).read_text()
        anon = set(re.findall(r"^\s+JSR\s+(L[0-9A-F]{4})\s*$", src, re.M))
        assert not anon, (f, sorted(anon))
