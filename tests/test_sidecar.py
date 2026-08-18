import pytest

from dis6801.sidecar import Region, Sidecar, load_sidecar

TOML = """
entry_points = [0xB200, 0xF129]

[meta]
rom = "../C64 BTX Decoder/c64_btx_decoder_ii.bin"
base = 0x8000

[labels]
"0xB200" = "reset"
"0xF129" = "vecSci"

[line_comments]
"0xB200" = "stack top -> external RAM below $0400"

[block_comments]
"0xF129" = "SCI interrupt stub"

[symbols]
"0x0001" = "P2DDR"
"0x00F0" = "softVecSci"

[[regions]]
start = 0x8000
end = 0x8010
kind = "bytes"

[[regions]]
start = 0x8010
end = 0x8020
kind = "words"
"""


def test_loads_all_sections(tmp_path):
    path = tmp_path / "s.toml"
    path.write_text(TOML)
    s = load_sidecar(path)
    assert isinstance(s, Sidecar)
    assert s.base == 0x8000
    assert s.entry_points == [0xB200, 0xF129]
    assert s.labels[0xB200] == "reset"
    assert s.line_comments[0xB200].startswith("stack top")
    assert s.block_comments[0xF129] == "SCI interrupt stub"
    assert s.symbols[0x00F0] == "softVecSci"
    assert s.regions[0] == Region(start=0x8000, end=0x8010, kind="bytes")


def test_region_lookup_returns_the_covering_region(tmp_path):
    path = tmp_path / "s.toml"
    path.write_text(TOML)
    s = load_sidecar(path)
    assert s.region_at(0x8005).kind == "bytes"
    assert s.region_at(0x8010).kind == "words"
    assert s.region_at(0x9000) is None


def test_overlapping_regions_are_rejected(tmp_path):
    path = tmp_path / "s.toml"
    path.write_text(TOML + '\n[[regions]]\nstart = 0x8008\nend = 0x8012\nkind = "bytes"\n')
    with pytest.raises(ValueError, match="overlap"):
        load_sidecar(path)


def test_unknown_region_kind_is_rejected(tmp_path):
    path = tmp_path / "s.toml"
    path.write_text('entry_points = []\n[meta]\nrom = "x"\nbase = 0\n'
                    '[[regions]]\nstart = 0\nend = 1\nkind = "nonsense"\n')
    with pytest.raises(ValueError, match="unknown region kind"):
        load_sidecar(path)


def test_the_real_sidecar_loads():
    s = load_sidecar("sidecar/decoder_ii.toml")
    assert s.base == 0x8000
    assert 0xB200 in s.entry_points


def test_entry_points_nested_under_meta_is_rejected(tmp_path):
    """TOML puts bare keys after [meta] *inside* meta. That silently yields no
    entry points and an empty disassembly, so reject it loudly instead."""
    path = tmp_path / "s.toml"
    path.write_text('[meta]\nrom = "x"\nbase = 0\nentry_points = [1, 2]\n')
    with pytest.raises(ValueError, match="entry_points must be a top-level key"):
        load_sidecar(path)


def test_code6502_is_a_valid_region_kind(tmp_path):
    """The ROM embeds a C64-side 6502 program alongside the 6801 firmware."""
    path = tmp_path / "s.toml"
    path.write_text('entry_points = []\n[meta]\nrom = "x"\nbase = 0\n'
                    '[[regions]]\nstart = 0\nend = 16\nkind = "code6502"\n')
    assert load_sidecar(path).regions[0].kind == "code6502"
