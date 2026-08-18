"""Listing generation. Pure formatting — no analysis decisions are made here.

Output targets asl (Macro Assembler AS) syntax and is also accepted verbatim by
dis65xx.asm, which is what enforces byte-identity.
"""

from __future__ import annotations

import dataclasses

from dis65xx import c64kernal, codec6502
from dis65xx.decode import Insn
from dis65xx.opcodes import Mode, modes_for
from dis65xx.opcodes6502 import Mode as M65
from dis65xx.sidecar import Sidecar
from dis65xx.trace import CODE, TraceResult

BYTES_PER_FCB = 16
_INDENT = 8      # leading spaces before a mnemonic
_MNEM_WIDTH = 8  # column width the mnemonic is padded to


_JUMPS = ("JMP", "JSR")


def _sweep_6502(data: bytes, base: int, end: int, region, known: dict[int, str]):
    """Linear 6502 sweep that splits at known labels, exactly as _emit_6502 does.

    The emitter breaks an instruction when a label falls inside it, which shifts
    its framing. A collector that ignored that would decode different
    instructions after the first split and miss the branches in them.
    """
    addr = region.start
    while addr < min(region.end, end):
        try:
            insn = codec6502.decode(data, addr - base, addr)
        except ValueError:
            addr += 1
            continue
        if insn.end > end:
            addr += 1
            continue
        split = next((x for x in range(insn.addr + 1, insn.end) if x in known), None)
        if split is not None:
            addr = split
            continue
        yield insn
        addr = insn.end


def collect_c64_targets(data: bytes, result: TraceResult, sidecar: Sidecar) -> set[int]:
    """Absolute JMP/JSR operands in the 6502 code.

    These name locations in the C64's address space - the cartridge window at
    $8000 or the payload's home at $1000 - not positions in this listing. They
    are emitted as L<addr> symbols defined by EQU to the address itself, so the
    operand still assembles to the same bytes while reading as a name.
    """
    base = result.base
    end = base + len(data)
    out: set[int] = set()
    for region in sidecar.regions:
        if region.kind != "code6502":
            continue
        for insn in _sweep_6502(data, base, end, region, sidecar.labels):
            if insn.mode is M65.ABS and insn.mnemonic in _JUMPS:
                out.add(insn.operand)
    return out


def collect_targets(data: bytes, result: TraceResult, sidecar: Sidecar,
                    known: dict[int, str] | None = None) -> dict[int, str]:
    """Auto-label every branch and jump target that has no name of its own.

    Named L<addr>, so a target reads as a label instead of a bare address. Only
    addresses inside the image qualify. The 6502 payload's absolute operands are
    RUNTIME addresses with no position in this listing, so only its relative
    branches are collected; its JMP/JSR operands stay literal.
    """
    base = result.base
    end = base + len(data)
    targets: set[int] = set()

    for insn in result.insns.values():
        if insn.mode is Mode.REL:
            targets.add(insn.operand)
        elif insn.mnemonic in _JUMPS and insn.mode in (Mode.EXT, Mode.DIR):
            targets.add(insn.operand)

    labels = {**(known or {}), **sidecar.labels}
    for region in sidecar.regions:
        if region.kind != "code6502":
            continue
        for insn in _sweep_6502(data, base, end, region, labels):
            if insn.mode is M65.REL:
                targets.add(insn.operand)

    return {a: f"L{a:04X}" for a in targets
            if base <= a < end and a not in sidecar.labels}


def format_operand(insn: Insn, sidecar: Sidecar) -> str:
    mode = insn.mode
    value = insn.operand

    if mode is Mode.INH:
        return ""
    if mode is Mode.IMM8:
        return f"#${value:02X}"
    if mode is Mode.IMM16:
        return f"#${value:04X}"
    if mode is Mode.IDX:
        return f"${value:02X},X"
    if mode is Mode.REL:
        return sidecar.labels.get(value) or f"${value:04X}"
    if mode is Mode.DIR:
        return sidecar.symbols.get(value) or f"${value:02X}"
    if mode is Mode.EXT:
        name = sidecar.symbols.get(value) or sidecar.labels.get(value)
        # Force extended when a shortest-fit assembler would choose direct.
        prefix = ">" if value < 0x100 and Mode.DIR in modes_for(insn.mnemonic) else ""
        return prefix + (name if name else f"${value:04X}")

    raise AssertionError(f"unhandled mode {mode}")  # pragma: no cover


def format_operand_6502(insn, sidecar: Sidecar | None = None) -> str:
    """6502 operand syntax. Absolute operands stay literal - the payload runs at
    a different address than it is stored, so its references name runtime
    locations with no position in this ROM."""
    m, v = insn.mode, insn.operand
    if m is M65.IMP:
        return ""
    if m is M65.ACC:
        return "A"
    if m is M65.IMM:
        return f"#${v:02X}"
    if m is M65.ZP:
        return f"${v:02X}"
    if m is M65.ZPX:
        return f"${v:02X},X"
    if m is M65.ZPY:
        return f"${v:02X},Y"
    if m is M65.IZX:
        return f"(${v:02X},X)"
    if m is M65.IZY:
        return f"(${v:02X}),Y"
    if m is M65.IND:
        return f"(${v:04X})"
    if m is M65.ABS and insn.mnemonic in _JUMPS:
        return c64kernal.name_for(v) or f"L{v:04X}"
    if m is M65.REL:
        if sidecar is not None:
            named = sidecar.labels.get(v)
            if named:
                return named
        return f"${v:04X}"
    # Absolute forms need > when a shortest-fit assembler would pick zero page.
    pre = ">" if v < 0x100 else ""
    if m is M65.ABS:
        return f"{pre}${v:04X}"
    if m is M65.ABX:
        return f"{pre}${v:04X},X"
    if m is M65.ABY:
        return f"{pre}${v:04X},Y"
    raise AssertionError(m)  # pragma: no cover


# The C64-side code lives in two address spaces. The bootstrap executes in
# place from the cartridge window at $8000; the payload is streamed to the C64
# and runs at $1000. An absolute operand names a location in one of those, not
# in this listing, so it stays literal - but it can be cross-referenced.
_C64_SPACES = ((0xB32D, 0xB3A6, 0x332D),   # bootstrap, runs at $8000
               (0xB3A6, 0xD109, 0xA3A8))   # payload,   runs at $1000


def _c64_xref(addr: int, insn, sidecar: Sidecar) -> str | None:
    """Where an absolute 6502 operand lives in this ROM, if anywhere."""
    if insn.mode is not M65.ABS or insn.mnemonic not in _JUMPS:
        return None
    for lo, hi, offset in _C64_SPACES:
        if lo <= addr < hi:
            rom_addr = insn.operand + offset
            if not lo <= rom_addr < hi:
                return None
            name = sidecar.labels.get(rom_addr)
            return f"{name} (${rom_addr:04X})" if name else f"ROM ${rom_addr:04X}"
    return None


def _emit_6502(data: bytes, base: int, start: int, end: int,
               sidecar: Sidecar, lines: list[str]) -> None:
    """Linear-disassemble a code6502 region. Undecodable bytes become FCB."""
    addr = start
    pending: list[int] = []

    def flush() -> None:
        while pending:
            chunk = bytes(pending[:BYTES_PER_FCB])
            del pending[:BYTES_PER_FCB]
            lines.append(_fcb_line(chunk))

    while addr < end:
        # The caller already emitted any label and banner at the region start;
        # re-emitting here would define the symbol twice.
        label = sidecar.labels.get(addr) if addr != start else None
        block = sidecar.block_comments.get(addr) if addr != start else None
        if label or block:
            flush()
            lines.append("")
            if block:
                for line in block.strip().splitlines():
                    lines.append(f"; {line}" if line else ";")
            if label:
                lines.append(f"{label}:")
        try:
            insn = codec6502.decode(data, addr - base, addr)
        except ValueError:
            pending.append(data[addr - base])
            addr += 1
            continue
        if insn.end > end:
            pending.append(data[addr - base])
            addr += 1
            continue
        # A label strictly inside this instruction means a branch targets a byte
        # the linear sweep framed over. Emit the leading bytes as FCB so the
        # label can sit on its own address.
        split = next((x for x in range(insn.addr + 1, insn.end)
                      if x in sidecar.labels or x in sidecar.block_comments), None)
        if split is not None:
            flush()
            lines.append(_fcb_line(data[addr - base:split - base])
                         + f"{'':<8}; branch target below splits this instruction")
            addr = split
            continue
        flush()
        body = f"{'':{_INDENT}}{insn.mnemonic:{_MNEM_WIDTH}}{format_operand_6502(insn, sidecar)}".rstrip()
        comment = sidecar.line_comments.get(addr) or _c64_xref(addr, insn, sidecar)
        lines.append(f"{body:<40}; {comment}" if comment else body)
        addr = insn.end
    flush()


def _instruction_line(insn: Insn, sidecar: Sidecar) -> str:
    operand = format_operand(insn, sidecar)
    body = f"{'':{_INDENT}}{insn.mnemonic:{_MNEM_WIDTH}}{operand}".rstrip()
    comment = sidecar.line_comments.get(insn.addr)
    return f"{body:<40}; {comment}" if comment else body


def _fcb_line(chunk: bytes) -> str:
    values = ",".join(f"${b:02X}" for b in chunk)
    return f"{'':{_INDENT}}{'FCB':{_MNEM_WIDTH}}{values}"


def _c64_equ_lines(targets: set[int], sidecar: Sidecar) -> list[str]:
    """EQU definitions for the 6502 control-flow targets."""
    if not targets:
        return []
    lines = ["; C64-space jump and call targets. Defined by EQU rather than as",
             "; listing labels: they name addresses in the C64's memory, not",
             "; positions in this ROM, so the operands assemble unchanged.",
             ""]
    width = max((len(c64kernal.name_for(a) or f"L{a:04X}") for a in targets), default=8)
    for addr in sorted(targets):
        name = c64kernal.name_for(addr) or f"L{addr:04X}"
        lines.append(f"{name:<{max(width, 8)}} EQU     ${addr:04X}")
    lines.append("")
    return lines


def _equ_lines(sidecar: Sidecar) -> list[str]:
    """Declare sidecar symbols so the listing is self-contained.

    format_operand() renders hardware registers and RAM locations by name. Those
    addresses live outside the ROM image, so unlike code labels nothing in the
    listing defines them — without these EQUs the listing does not assemble.
    """
    if not sidecar.symbols:
        return []
    width = max(len(n) for n in sidecar.symbols.values())
    lines = ["; Hardware registers and RAM locations referenced below.", ""]
    for addr, name in sorted(sidecar.symbols.items()):
        value = f"${addr:02X}" if addr < 0x100 else f"${addr:04X}"
        lines.append(f"{name:{max(width, _INDENT - 1)}} EQU     {value}")
    lines.append("")
    return lines


def _fdb_line(words: list[int]) -> str:
    values = ",".join(f"${w:04X}" for w in words)
    return f"{'':{_INDENT}}{'FDB':{_MNEM_WIDTH}}{values}"


def emit(data: bytes, result: TraceResult, sidecar: Sidecar) -> str:
    # Branch and jump targets become labels. The sidecar's own names win, so a
    # routine that has been identified keeps its name instead of an L-address.
    # Collecting changes the emitter's framing (a label splits an instruction),
    # which can expose branches that were not visible on the previous pass, so
    # iterate until the label set stops growing.
    auto: dict[int, str] = {}
    for _ in range(8):
        found = collect_targets(data, result, sidecar, auto)
        if found == auto:
            break
        auto = found
    else:
        raise ValueError("branch-label collection did not converge")
    sidecar = dataclasses.replace(sidecar, labels={**auto, **sidecar.labels})
    c64_targets = collect_c64_targets(data, result, sidecar)

    clash = {a for a in c64_targets
             if (c64kernal.name_for(a) or f"L{a:04X}") in sidecar.labels.values()}
    if clash:
        raise ValueError("L-name collision between a listing label and a C64 "
                         "target: " + ", ".join(f"${a:04X}" for a in sorted(clash)))

    base = result.base
    lines: list[str] = [
        "; Commodore BTX Decoder II firmware",
        "; GENERATED by dis65xx from sidecar/decoder_ii.toml - DO NOT EDIT.",
        ";",
        "; Regenerate and verify with:  python3 build.py",
        "",
        f"{'':{_INDENT}}{'CPU':{_MNEM_WIDTH}}6801",
        "",
    ]
    lines += _equ_lines(sidecar)
    lines += _c64_equ_lines(c64_targets, sidecar)
    lines += [
        f"{'':{_INDENT}}{'ORG':{_MNEM_WIDTH}}${base:04X}",
        "",
    ]

    addr = base
    end = base + len(data)
    pending: list[int] = []  # unclassified bytes waiting to be flushed as FCB

    def flush() -> None:
        while pending:
            chunk = bytes(pending[:BYTES_PER_FCB])
            del pending[:BYTES_PER_FCB]
            lines.append(_fcb_line(chunk))

    current_cpu = "6801"
    while addr < end:
        desired = sidecar.cpu_at(addr)
        if desired != current_cpu:
            flush()
            lines.append("")
            lines.append(f"{'':{_INDENT}}{'CPU':{_MNEM_WIDTH}}{desired}")
            lines.append("")
            current_cpu = desired

        label = sidecar.labels.get(addr)
        block = sidecar.block_comments.get(addr)
        region = sidecar.region_at(addr)
        insn = result.insns.get(addr)
        # An address the tracer decoded from is a real entry point even if an
        # overlapping instruction later marked its bytes as operand - that is
        # exactly the skip-chain case, where each stub is entered directly.
        is_code = insn is not None and (
            result.kind[addr - base] is CODE or addr in sidecar.labels)

        if label or block:
            flush()
            lines.append("")
            if block:
                for line in block.strip().splitlines():
                    lines.append(f"; {line}" if line else ";")
            if label:
                lines.append(f"{label}:")

        if region is not None and region.kind == "code6502" and addr == region.start:
            flush()
            _emit_6502(data, base, region.start, min(region.end, end), sidecar, lines)
            addr = min(region.end, end)
            continue

        if region is not None and region.kind in ("words", "ptr_table") and not is_code:
            flush()
            count = min(8, (min(region.end, end) - addr) // 2)
            if count:
                words = [
                    int.from_bytes(data[addr - base + 2 * i: addr - base + 2 * i + 2], "big")
                    for i in range(count)
                ]
                lines.append(_fdb_line(words))
                addr += 2 * count
                continue

        if is_code:
            # A label strictly inside this instruction is an overlapping entry
            # point - the skip-chain idiom, where a 3-byte CPX # swallows the
            # following stub. Emit the leading bytes as FCB so the label can sit
            # on its own address instead of being silently dropped.
            split = next((x for x in range(insn.addr + 1, insn.end)
                          if x in sidecar.labels or x in sidecar.block_comments), None)
            if split is not None:
                flush()
                lines.append(_fcb_line(data[addr - base:split - base])
                             + f"{'':<8}; overlapped by the entry point below")
                addr = split
                continue
            flush()
            lines.append(_instruction_line(insn, sidecar))
            addr = insn.end
            continue

        pending.append(data[addr - base])
        addr += 1
        # Break the FCB run at any address that carries its own annotation.
        if addr < end and (addr in sidecar.labels or addr in sidecar.block_comments):
            flush()

    flush()
    lines.append("")
    lines.append(f"{'':{_INDENT}}END")
    return "\n".join(lines) + "\n"
