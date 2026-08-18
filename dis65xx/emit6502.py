"""Standalone 6502 source for a C64 block.

Assembled on its own at the address it really runs at, so every internal target
is an ordinary label at an ordinary address. That is the whole point of keeping
the two instruction sets in separate files: neither listing has to carry the
other's addressing quirks.
"""

from __future__ import annotations

from dis65xx import c64kernal, codec6502
from dis65xx.opcodes6502 import Mode
from dis65xx.sidecar import C64Block, Sidecar

BYTES_PER_FCB = 16
_INDENT = 8
_MNEM_WIDTH = 8
_JUMPS = ("JMP", "JSR")


def _operand(insn, labels: dict[int, str], symbols: dict[int, str] | None = None) -> str:
    m, v = insn.mode, insn.operand
    if m is Mode.IMP:
        return ""
    if m is Mode.ACC:
        return "A"
    if m is Mode.IMM:
        return f"#${v:02X}"
    symbols = symbols or {}
    if m is Mode.ZP:
        return symbols.get(v) or f"${v:02X}"
    if m is Mode.ZPX:
        return f"${v:02X},X"
    if m is Mode.ZPY:
        return f"${v:02X},Y"
    if m is Mode.IZX:
        return f"(${v:02X},X)"
    if m is Mode.IZY:
        return f"(${v:02X}),Y"
    if m is Mode.IND:
        # The operand names the location holding the address, not the target,
        # so a data symbol describes it better than a code label.
        return f"({symbols.get(v) or labels.get(v) or f'${v:04X}'})"
    if m is Mode.REL:
        return labels.get(v) or f"${v:04X}"
    # A control transfer names a label; anything else prefers a hardware or
    # RAM symbol, since it is referring to a location, not to code.
    if insn.mnemonic in _JUMPS and m is not Mode.IND:
        name = labels.get(v)
    else:
        name = symbols.get(v) or labels.get(v)
    pre = ">" if v < 0x100 and name is None else ""
    body = name or f"${v:04X}"
    if m is Mode.ABS:
        return pre + body
    if m is Mode.ABX:
        return pre + body + ",X"
    if m is Mode.ABY:
        return pre + body + ",Y"
    raise AssertionError(m)  # pragma: no cover


_DATA_KINDS = {"bytes", "string", "words", "ptr_table", "chargen"}
_WORD_KINDS = {"ptr_table", "words"}


def _decode_block(data: bytes, base: int, block: C64Block, labels: dict[int, str],
                  sidecar: Sidecar | None = None):
    """Yield (runtime_addr, insn_or_None, byte) walking the block.

    A byte covered by a data region is never disassembled - that is what keeps
    the German text and the tables after the jump table from being rendered as
    nonsense instructions.
    """
    addr = block.start
    while addr < block.end:
        rt = addr - block.offset
        split = None
        region = sidecar.region_at(addr) if sidecar is not None else None
        if region is not None and region.kind in _DATA_KINDS:
            yield rt, None, data[addr - base]
            addr += 1
            continue
        try:
            insn = codec6502.decode(data, addr - base, rt)
        except ValueError:
            insn = None
        if insn is not None and addr + insn.size <= block.end:
            split = next((x for x in range(rt + 1, rt + insn.size) if x in labels), None)
        if insn is None or addr + insn.size > block.end or split is not None:
            yield rt, None, data[addr - base]
            addr += 1
            continue
        yield rt, insn, None
        addr += insn.size


def collect_labels(data: bytes, base: int, block: C64Block,
                   sidecar: Sidecar) -> dict[int, str]:
    """Named labels from the sidecar, plus L<addr> for every internal target."""
    named = {a - block.offset: n for a, n in sidecar.labels.items()
             if block.start <= a < block.end}
    # Pointer tables inside the block name locations in it; each entry should
    # resolve to a label, which also breaks the data runs at record boundaries.
    table_targets: set[int] = set()
    for region in sidecar.regions:
        if region.kind not in _WORD_KINDS:
            continue
        if not (block.start <= region.start < block.end):
            continue
        for a in range(region.start, min(region.end, block.end) - 1, 2):
            table_targets.add(int.from_bytes(data[a - base:a - base + 2], "little"))

    labels = dict(named)
    for _ in range(8):
        targets: set[int] = set(table_targets)
        for rt, insn, _b in _decode_block(data, base, block, labels, sidecar):
            if insn is None:
                continue
            if insn.mode is Mode.REL:
                targets.add(insn.operand)
            elif insn.mode is Mode.ABS and insn.mnemonic in _JUMPS:
                targets.add(insn.operand)
        found = dict(named)
        found.update({a: f"L{a:04X}" for a in targets
                      if block.org <= a < block.org + (block.end - block.start)
                      and a not in named})
        if found == labels:
            break
        labels = found
    else:
        raise ValueError(f"{block.name}: label collection did not converge")
    return labels


def emit_block(data: bytes, base: int, block: C64Block, sidecar: Sidecar) -> str:
    labels = collect_labels(data, base, block, sidecar)

    # External references: C64 ROM entry points and anything outside this block.
    external: dict[int, str] = {}
    for _rt, insn, _b in _decode_block(data, base, block, labels, sidecar):
        if insn is None or insn.operand is None:
            continue
        if insn.mode not in (Mode.ABS, Mode.IND) or insn.mnemonic not in _JUMPS:
            continue
        v = insn.operand
        if v in labels:
            continue
        name = c64kernal.name_for(v)
        if name:
            external[v] = name

    lines = [
        f"; C64 {block.name} - extracted from the BTX Decoder II ROM.",
        f"; Lives at ${block.start:04X}-${block.end - 1:04X} in the ROM and runs at "
        f"${block.org:04X} on the C64.",
        "; GENERATED by dis65xx - DO NOT EDIT. Regenerate with: python3 build.py",
        "",
        f"{'':{_INDENT}}{'CPU':{_MNEM_WIDTH}}6502",
        "",
    ]
    # Which hardware/RAM symbols this block actually touches.
    touched: dict[int, str] = {}
    for _rt, insn, _b in _decode_block(data, base, block, labels, sidecar):
        if insn is None or insn.operand is None:
            continue
        if insn.mnemonic in _JUMPS and insn.mode is not Mode.IND:
            continue
        name = sidecar.c64_symbols.get(insn.operand)
        if name:
            touched[insn.operand] = name
    if touched:
        lines.append("; Decoder hardware and RAM, as the C64 sees it.")
        width = max(len(n) for n in touched.values())
        for addr in sorted(touched):
            lines.append(f"{touched[addr]:<{max(width, 8)}} EQU     ${addr:04X}")
        lines.append("")
    if external:
        lines.append("; C64 ROM entry points.")
        width = max(len(n) for n in external.values())
        for addr in sorted(external):
            lines.append(f"{external[addr]:<{max(width, 8)}} EQU     ${addr:04X}")
        lines.append("")
    lines.append(f"{'':{_INDENT}}{'ORG':{_MNEM_WIDTH}}${block.org:04X}")
    lines.append("")

    all_names = {**external, **labels}
    used_symbols = {a: n for a, n in sidecar.c64_symbols.items()}
    pending: list[int] = []

    def _printable(b: int) -> bool:
        # '"' delimits an FCC string and '\\' starts an escape sequence in asl,
        # so a run containing either falls back to FCB rather than needing
        # quoting rules that the two assemblers might read differently.
        return 0x20 <= b < 0x7F and b not in (0x22, 0x5C)

    def flush() -> None:
        """Emit pending data, showing runs of text as text."""
        while pending:
            run = 0
            while run < len(pending) and _printable(pending[run]):
                run += 1
            if run >= 4:
                text = bytes(pending[:run]).decode("latin-1")
                del pending[:run]
                for i in range(0, len(text), 40):
                    lines.append(f"{'':{_INDENT}}{'FCC':{_MNEM_WIDTH}}"
                                 f'"{text[i:i + 40]}"')
                continue
            take = run if run else 1
            while take < len(pending) and not _printable(pending[take]):
                take += 1
            take = min(take, BYTES_PER_FCB)
            chunk = bytes(pending[:take])
            del pending[:take]
            lines.append(f"{'':{_INDENT}}{'FCB':{_MNEM_WIDTH}}"
                         + ",".join(f"${b:02X}" for b in chunk))

    words_left = 0
    for rt, insn, byte in _decode_block(data, base, block, labels, sidecar):
        if rt in labels:
            flush()
            rom_addr = rt + block.offset
            block_comment = sidecar.block_comments.get(rom_addr)
            lines.append("")
            if block_comment:
                for line in block_comment.strip().splitlines():
                    lines.append(f"; {line}" if line else ";")
            lines.append(f"{labels[rt]}:")
        region = sidecar.region_at(rt + block.offset)
        if region is not None and region.kind in _WORD_KINDS and insn is None:
            if words_left == 0:
                flush()
                lo = rt + block.offset
                count = min(8, (min(region.end, block.end) - lo) // 2)
                if count:
                    vals = [int.from_bytes(data[lo - base + 2 * i:lo - base + 2 * i + 2],
                                           "little") for i in range(count)]
                    lines.append(f"{'':{_INDENT}}{'DW':{_MNEM_WIDTH}}"
                                 + ",".join(all_names.get(v) or f"${v:04X}"
                                            for v in vals))
                    words_left = count * 2
            if words_left:
                words_left -= 1
                continue
        if insn is None:
            pending.append(byte)
            continue
        flush()
        body = (f"{'':{_INDENT}}{insn.mnemonic:{_MNEM_WIDTH}}"
                f"{_operand(insn, all_names, used_symbols)}").rstrip()
        comment = sidecar.line_comments.get(rt + block.offset)
        lines.append(f"{body:<40}; {comment}" if comment else body)
    flush()
    lines.append("")
    lines.append(f"{'':{_INDENT}}END")
    return "\n".join(lines) + "\n"
