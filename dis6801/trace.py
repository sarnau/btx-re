"""Recursive-descent code discovery.

All control-flow knowledge lives here. Takes bytes plus entry points, returns a
classification of every byte in the image.
"""

from __future__ import annotations

import dataclasses

from dis6801.decode import Insn, decode
from dis6801.opcodes import Mode

UNKNOWN = "unknown"
CODE = "code"
OPERAND = "operand"  # a byte belonging to an instruction, but not its opcode

# Instructions after which execution does not continue to the next address.
_ENDS_FLOW = {"JMP", "BRA", "RTS", "RTI"}
# Instructions that transfer control but return: the following address is live.
_CALLS = {"JSR", "BSR"}


@dataclasses.dataclass
class TraceResult:
    base: int
    kind: list[str]                  # one entry per byte of the image
    insns: dict[int, Insn]           # address -> decoded instruction
    call_targets: set[int]
    branch_targets: set[int]
    unresolved: list[int]            # addresses of computed jumps we could not follow
    bad_opcodes: list[int]           # addresses where decoding failed

    def coverage(self) -> dict[str, int]:
        counts = {UNKNOWN: 0, CODE: 0, OPERAND: 0}
        for k in self.kind:
            counts[k] += 1
        return {"code": counts[CODE] + counts[OPERAND], "unknown": counts[UNKNOWN]}


def trace(data: bytes, *, base: int, entry_points: list[int]) -> TraceResult:
    result = TraceResult(
        base=base,
        kind=[UNKNOWN] * len(data),
        insns={},
        call_targets=set(),
        branch_targets=set(),
        unresolved=[],
        bad_opcodes=[],
    )

    end = base + len(data)
    pending = [a for a in entry_points if base <= a < end]
    seen: set[int] = set()

    while pending:
        addr = pending.pop()
        while True:
            if not (base <= addr < end) or addr in seen:
                break
            seen.add(addr)

            try:
                insn = decode(data, addr - base, addr)
            except ValueError:
                if addr not in result.bad_opcodes:
                    result.bad_opcodes.append(addr)
                break

            if insn.end > end:
                result.bad_opcodes.append(addr)
                break

            result.insns[addr] = insn
            result.kind[addr - base] = CODE
            for i in range(addr + 1, insn.end):
                result.kind[i - base] = OPERAND

            target = _static_target(insn)
            if insn.mnemonic in _CALLS:
                if target is None:
                    result.unresolved.append(addr)
                else:
                    result.call_targets.add(target)
                    pending.append(target)
                addr = insn.end
                continue

            if insn.mode is Mode.REL and insn.mnemonic not in ("BRA", "BSR"):
                # Conditional branch: both the target and the fallthrough are live.
                # BRN never branches, but recording its target is harmless.
                result.branch_targets.add(insn.operand)
                pending.append(insn.operand)
                addr = insn.end
                continue

            if insn.mnemonic in _ENDS_FLOW:
                if target is not None:
                    result.branch_targets.add(target)
                    pending.append(target)
                elif insn.mnemonic == "JMP":
                    result.unresolved.append(addr)
                break

            addr = insn.end

    result.unresolved.sort()
    result.bad_opcodes.sort()
    return result


def _static_target(insn: Insn) -> int | None:
    """Absolute target of a transfer instruction, or None if computed at runtime."""
    if insn.mnemonic in ("JMP", "JSR"):
        if insn.mode in (Mode.EXT, Mode.DIR):
            return insn.operand
        return None  # indexed: depends on X
    if insn.mode is Mode.REL:
        return insn.operand
    return None
