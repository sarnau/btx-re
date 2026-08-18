"""The 6502 blocks are separate programs, linked into the 6801 listing as binary."""

import pathlib
import re

import build
from dis65xx.asm import assemble

OUT = pathlib.Path(__file__).resolve().parent.parent / "out"


def test_6801_listing_is_pure_6801():
    """One CPU directive, no 6502 anywhere. Separating the sources is the point."""
    listing = build.run(write=False).listing
    cpu = [ln.strip() for ln in listing.splitlines() if ln.strip().startswith("CPU")]
    assert cpu == ["CPU     6801"], cpu


def test_6801_listing_pulls_the_blocks_in_as_binary():
    listing = build.run(write=False).listing
    assert 'BINCLUDE "c64_bootstrap.bin"' in listing
    assert 'BINCLUDE "c64_payload.bin"' in listing


def test_each_block_assembles_to_exactly_its_rom_bytes():
    """build.run() raises if a block does not match, so reaching here is the
    check; this pins the sizes and load addresses too."""
    result = build.run(write=True)
    rom = result.rom
    for name, start, end, org in (("bootstrap", 0xB32D, 0xB3A6, 0x8000),
                                  ("payload", 0xB3A6, 0xD109, 0x0FFE)):
        blob = (OUT / f"c64_{name}.bin").read_bytes()
        assert blob == rom[start - 0x8000:end - 0x8000], name
        src = (OUT / f"c64_{name}.asm").read_text()
        assert f"ORG     ${org:04X}" in src, name
        assert "CPU     6502" in src, name


def test_payload_binary_is_a_real_prg():
    """The payload carries its own PRG load-address word and is ORGed two bytes
    early so the data still lands at $1000. The binary is therefore a genuine
    C64 .prg - which is exactly what the 6801 streams and the loader consumes."""
    build.run(write=True)
    blob = (OUT / "c64_payload.bin").read_bytes()
    assert blob[:2] == bytes([0x00, 0x10]), blob[:2].hex()
    src = (OUT / "c64_payload.asm").read_text()
    assert "c64LoadAddr:" in src
    assert "FCB     $00,$10" in src
    assert "ORG     $0FFE" in src


def test_blocks_are_contiguous_and_leave_no_loose_bytes():
    """The two BINCLUDEs sit back to back with nothing emitted between them."""
    import build as _b
    sc = __import__("dis65xx.sidecar", fromlist=["load_sidecar"]).load_sidecar(
        pathlib.Path(__file__).resolve().parent.parent / "sidecar" / "decoder_ii.toml")
    boot, payload = sc.c64_blocks
    assert boot.end == payload.start, (hex(boot.end), hex(payload.start))
    listing = build.run(write=False).listing
    between = listing.split('BINCLUDE "c64_bootstrap.bin"')[1].split(
        'BINCLUDE "c64_payload.bin"')[0]
    assert not [ln for ln in between.splitlines()
                if ln.strip() and not ln.lstrip().startswith(";")], between


def test_data_regions_render_as_fcb_not_instructions():
    """The German text and the tables must not be disassembled as code."""
    src = (OUT / "c64_payload.asm").read_text()
    assert re.search(r"^\s+FCB\s+\$", src, re.M)


def test_block_sources_use_real_labels_at_real_addresses():
    """Assembled at the address it runs at, so targets are ordinary labels -
    no EQU indirection and no cross-reference comments."""
    src = (OUT / "c64_bootstrap.asm").read_text()
    assert re.search(r"^L80[0-9A-F]{2}:", src, re.M), "expected labels in $80xx"
    assert "JSR     IOINIT" in src
    # the old mixed listing annotated each absolute operand with "; name ($ROM)"
    assert not re.search(r"^\s+(JSR|JMP)\s+\S+\s+;\s+\S+\s+\(\$[0-9A-F]{4}\)$",
                         src, re.M), "cross-references belong to the old mixed listing"


def test_kernal_entries_are_named_not_l_addresses():
    src = (OUT / "c64_bootstrap.asm").read_text()
    for name, addr in (("IOINIT", 0xFDA3), ("CINT", 0xFF5B), ("RESTOR", 0xFD15)):
        assert re.search(rf"^{name}\s+EQU\s+\${addr:04X}$", src, re.M), name
        assert f"L{addr:04X}" not in src


def test_binclude_round_trips_through_our_assembler(tmp_path):
    (tmp_path / "blob.bin").write_bytes(bytes([0xAA, 0xBB, 0xCC]))
    src = ('        ORG $8000\n        LDAA #$01\n'
           '        BINCLUDE "blob.bin"\n        LDAB #$02\n        END\n')
    _, out = assemble(src, include_dir=tmp_path)
    assert out == bytes([0x86, 0x01, 0xAA, 0xBB, 0xCC, 0xC6, 0x02])


def test_binclude_missing_file_is_reported():
    import pytest
    with pytest.raises(ValueError, match="BINCLUDE file not found"):
        assemble('        ORG $8000\n        BINCLUDE "nope.bin"\n        END\n')


def test_string_table_uses_labels_not_addresses():
    """DW because FDB is big-endian, and labels because each entry points at a
    record in this same block."""
    src = (OUT / "c64_payload.asm").read_text()
    assert "c64StrTable:" in src
    assert "DW      L11F8,L1211," in src
    for label in ("L11F8", "L126B", "L1643"):
        assert f"{label}:" in src, label


def test_text_records_render_as_readable_strings():
    """A record is five header bytes then characters; showing the characters as
    characters is the whole point of the listing."""
    src = (OUT / "c64_payload.asm").read_text()
    assert 'FCC     "Load Capture Display Macro Xfer Screen  "' in src
    assert 'FCC     "ASCII Btx Keybd Telesoft Edit Pause Quit"' in src
    # the header stays hex, immediately above its text
    idx = src.index('FCC     "Load Capture')
    assert "FCB     $2C,$00,$C0,$01,$98" in src[idx - 120:idx]


def test_fcc_round_trips_and_survives_a_semicolon():
    """A naive comment strip truncates any string containing ';'."""
    _, out = assemble('        ORG $1000\n        FCC "a;b"\n        FCB $FF\n        END\n')
    assert out == b"a;b\xff"


def test_dw_is_little_endian_and_fdb_is_big():
    _, dw = assemble("        ORG $1000\n        DW $1234\n        END\n")
    _, fdb = assemble("        ORG $1000\n        FDB $1234\n        END\n")
    assert dw == bytes([0x34, 0x12])
    assert fdb == bytes([0x12, 0x34])


def test_text_block_through_screen_init_is_data():
    """ROM $B552-$BA55 is records and their pointer table; code resumes at
    c64ScreenInit."""
    src = (OUT / "c64_payload.asm").read_text()
    head, tail = src.split("c64ScreenInit:", 1)
    table = head.split("c64StrTable:", 1)[1]
    assert not re.search(r"^\s+(LDA|STA|JSR|JMP)\s", table, re.M), \
        "the pointer table must not be disassembled as code"
    assert re.search(r"^\s+LDA\s", tail, re.M), "code should resume after the label"


def test_strings_avoid_characters_the_assemblers_read_differently():
    """asl treats \\ as an escape and " as the delimiter. A run containing
    either falls back to FCB rather than relying on quoting rules that the two
    assemblers might not agree on - asl rejected "A \\[" outright."""
    src = (OUT / "c64_payload.asm").read_text()
    for line in src.splitlines():
        if line.strip().startswith("FCC"):
            text = line.split('"', 1)[1].rsplit('"', 1)[0]
            assert "\\" not in text and '"' not in text, line


def test_fcc_threshold_keeps_binary_data_out_of_strings():
    """Two printable bytes in binary data are usually coincidence. Rendering
    them as text produced lines like FCC "^]" and FCC "<;", which read as
    strings but are not."""
    src = (OUT / "c64_payload.asm").read_text()
    shorts = [ln.strip() for ln in src.splitlines()
              if ln.strip().startswith("FCC") and len(ln.split('"')[1]) < 4]
    # only a chunked remainder of a long run may be short
    assert len(shorts) <= 1, shorts


def test_data_runs_are_not_fragmented():
    """Signature plus padding is one run; splitting it at every printable byte
    scattered it over several lines and hid the signature inside them."""
    src = (OUT / "c64_bootstrap.asm").read_text()
    head = src.split("c64CartSignature:", 1)[1].split("c64ColdStart:", 1)[0]
    fcb = [ln for ln in head.splitlines() if ln.strip().startswith("FCB")]
    assert len(fcb) == 1, fcb
    assert "$C3,$C2,$CD,$38,$30,$00,$00,$00,$FF" in fcb[0]


def test_cartridge_header_vectors_render_as_addresses():
    """$8000/$8001 and $8002/$8003 are the cold- and warm-start vectors, so
    they are DW, not bytes. The cold vector resolves to the label it points at,
    which is what proves ROM $B32D maps to C64 $8000."""
    src = (OUT / "c64_bootstrap.asm").read_text()
    assert "DW      c64ColdStart,KERNAL_FE72" in src
    assert re.search(r"^KERNAL_FE72\s+EQU\s+\$FE72$", src, re.M)
    # the signature stays bytes - FCC would change them, CBM is PETSCII
    assert "FCB     $C3,$C2,$CD,$38,$30" in src
