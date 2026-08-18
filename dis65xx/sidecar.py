"""Loading and validation of the analysis metadata.

Everything a human learns about the ROM lives in the sidecar. The generated
listing is a pure function of (ROM bytes, sidecar).
"""

from __future__ import annotations

import dataclasses
import pathlib
import tomllib

REGION_KINDS = {"code", "bytes", "words", "words_le", "words_raw", "string", "ptr_table", "chargen",
                "code6502", "petscii", "byte_word"}


@dataclasses.dataclass(frozen=True)
class C64Block:
    """A stretch of 6502 code embedded in the ROM.

    It is assembled as its own program at `org` - the address it really runs at
    on the C64 - and the result is pulled back into the 6801 listing as binary.
    Keeping the two instruction sets in separate sources means neither listing
    has to carry the other's addressing quirks.
    """

    name: str
    start: int    # in this ROM
    end: int      # exclusive
    org: int      # where it runs on the C64

    @property
    def offset(self) -> int:
        """rom_address - runtime_address."""
        return self.start - self.org


@dataclasses.dataclass(frozen=True)
class Region:
    start: int
    end: int      # exclusive
    kind: str
    # Fixed record width, for a string region holding padded records. Breaking
    # the FCC lines on it puts one record per line, which is what makes the
    # padding legible as padding.
    width: int | None = None


@dataclasses.dataclass
class Sidecar:
    rom: str
    base: int
    entry_points: list[int]
    labels: dict[int, str]
    line_comments: dict[int, str]
    block_comments: dict[int, str]
    symbols: dict[int, str]
    regions: list[Region]
    # Instructions whose 16-bit immediate is a constant that happens to
    # collide with a named address, so it must not be printed as the name.
    literal_immediates: frozenset[int] = frozenset()
    # Per-instruction operand names, for a location the firmware reuses. One
    # address cannot carry one honest name when it is a mask in one routine and
    # an index in another, so the name is attached to the site instead.
    site_symbols: dict[int, str] = dataclasses.field(default_factory=dict)
    c64_blocks: list[C64Block] = dataclasses.field(default_factory=list)
    c64_symbols: dict[int, str] = dataclasses.field(default_factory=dict)
    c64_pointers: list[int] = dataclasses.field(default_factory=list)

    def c64_block_at(self, addr: int) -> C64Block | None:
        for b in self.c64_blocks:
            if b.start <= addr < b.end:
                return b
        return None

    def region_at(self, addr: int) -> Region | None:
        for r in self.regions:
            if r.start <= addr < r.end:
                return r
        return None


def _int_keys(table: dict[str, str]) -> dict[int, str]:
    return {int(k, 0): v for k, v in table.items()}


def load_sidecar(path: str | pathlib.Path) -> Sidecar:
    raw = tomllib.loads(pathlib.Path(path).read_text())
    meta = raw["meta"]

    # TOML assigns bare keys written after a [table] header to that table. An
    # entry_points list placed below [meta] therefore becomes meta.entry_points
    # and vanishes from the top level, producing a silently empty disassembly.
    for key in ("entry_points", "literal_immediates"):
        if key in meta:
            raise ValueError(
                f"{key} must be a top-level key placed ABOVE the [meta] header; "
                f"found it nested under [meta]"
            )

    regions = []
    for r in raw.get("regions", []):
        if r["kind"] not in REGION_KINDS:
            raise ValueError(f"unknown region kind {r['kind']!r} at ${r['start']:04X}")
        if r["end"] <= r["start"]:
            raise ValueError(f"empty region at ${r['start']:04X}")
        regions.append(Region(start=r["start"], end=r["end"], kind=r["kind"],
                              width=r.get("width")))

    regions.sort(key=lambda r: r.start)
    for a, b in zip(regions, regions[1:]):
        if a.end > b.start:
            raise ValueError(
                f"regions overlap: ${a.start:04X}-${a.end:04X} and ${b.start:04X}-${b.end:04X}"
            )

    blocks = [C64Block(name=b["name"], start=b["start"], end=b["end"], org=b["org"])
              for b in raw.get("c64_blocks", [])]
    blocks.sort(key=lambda b: b.start)
    for a, b in zip(blocks, blocks[1:]):
        if a.end > b.start:
            raise ValueError(f"c64_blocks overlap: {a.name} and {b.name}")

    return Sidecar(
        rom=meta["rom"],
        base=meta["base"],
        entry_points=list(raw.get("entry_points", [])),
        labels=_int_keys(raw.get("labels", {})),
        line_comments=_int_keys(raw.get("line_comments", {})),
        block_comments=_int_keys(raw.get("block_comments", {})),
        symbols=_int_keys(raw.get("symbols", {})),
        regions=regions,
        literal_immediates=frozenset(raw.get("literal_immediates", [])),
        site_symbols=_int_keys(raw.get("site_symbols", {})),
        c64_blocks=blocks,
        c64_symbols=_int_keys(raw.get("c64_symbols", {})),
        c64_pointers=list(raw.get("c64_pointers", [])),
    )
