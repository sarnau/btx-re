"""Where to look next: unresolved computed jumps and the largest unreached gaps."""

from __future__ import annotations


def unreached_runs(kind: list[str], *, base: int, minimum: int = 16) -> list[tuple[int, int]]:
    """Contiguous runs of unclassified bytes, as (address, length)."""
    runs: list[tuple[int, int]] = []
    start = None
    for i, k in enumerate(kind):
        if k == "unknown":
            if start is None:
                start = i
        elif start is not None:
            if i - start >= minimum:
                runs.append((base + start, i - start))
            start = None
    if start is not None and len(kind) - start >= minimum:
        runs.append((base + start, len(kind) - start))
    return runs


def format_report(*, base: int, kind: list[str], unresolved: list[int],
                  bad_opcodes: list[int], top: int = 20) -> str:
    lines: list[str] = []

    lines.append(f"Unresolved computed jumps ({len(unresolved)}):")
    for addr in unresolved[:top]:
        lines.append(f"  ${addr:04X}")
    if len(unresolved) > top:
        lines.append(f"  ... and {len(unresolved) - top} more")

    if bad_opcodes:
        lines.append("")
        lines.append(f"Decode failures ({len(bad_opcodes)}):")
        for addr in bad_opcodes[:top]:
            lines.append(f"  ${addr:04X}")

    runs = sorted(unreached_runs(kind, base=base, minimum=1), key=lambda r: -r[1])
    lines.append("")
    lines.append(f"Largest unreached regions ({len(runs)} total):")
    for addr, length in runs[:top]:
        lines.append(f"  ${addr:04X}  {length} bytes")

    return "\n".join(lines)
