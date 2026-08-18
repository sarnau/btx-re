"""Loading and validation of the analysis metadata.

Everything a human learns about the ROM lives in the sidecar. The generated
listing is a pure function of (ROM bytes, sidecar).
"""

from __future__ import annotations

import dataclasses
import pathlib
import tomllib

REGION_KINDS = {"code", "bytes", "words", "string", "ptr_table", "chargen",
                "code6502"}


@dataclasses.dataclass(frozen=True)
class Region:
    start: int
    end: int      # exclusive
    kind: str


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
    if "entry_points" in meta:
        raise ValueError(
            "entry_points must be a top-level key placed ABOVE the [meta] header; "
            "found it nested under [meta]"
        )

    regions = []
    for r in raw.get("regions", []):
        if r["kind"] not in REGION_KINDS:
            raise ValueError(f"unknown region kind {r['kind']!r} at ${r['start']:04X}")
        if r["end"] <= r["start"]:
            raise ValueError(f"empty region at ${r['start']:04X}")
        regions.append(Region(start=r["start"], end=r["end"], kind=r["kind"]))

    regions.sort(key=lambda r: r.start)
    for a, b in zip(regions, regions[1:]):
        if a.end > b.start:
            raise ValueError(
                f"regions overlap: ${a.start:04X}-${a.end:04X} and ${b.start:04X}-${b.end:04X}"
            )

    return Sidecar(
        rom=meta["rom"],
        base=meta["base"],
        entry_points=list(raw.get("entry_points", [])),
        labels=_int_keys(raw.get("labels", {})),
        line_comments=_int_keys(raw.get("line_comments", {})),
        block_comments=_int_keys(raw.get("block_comments", {})),
        symbols=_int_keys(raw.get("symbols", {})),
        regions=regions,
    )
