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
                                  ("payload", 0xB3A8, 0xD109, 0x1000)):
        blob = (OUT / f"c64_{name}.bin").read_bytes()
        assert blob == rom[start - 0x8000:end - 0x8000], name
        src = (OUT / f"c64_{name}.asm").read_text()
        assert f"ORG     ${org:04X}" in src, name
        assert "CPU     6502" in src, name


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
