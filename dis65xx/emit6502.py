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
# Shorter runs are not worth showing as text: two printable bytes in binary
# data are usually coincidence, and rendering them as strings is misleading.
_MIN_FCC = 4
_INDENT = 8
_MNEM_WIDTH = 8
_JUMPS = ("JMP", "JSR")


# C64 lowercase charset: uppercase letters live at $C1-$DA and lowercase at
# $41-$5A. Writing that as ordinary text needs a translation, which both asl and
# dis65xx/asm.py express with CHARSET.
_PETSCII_CHARSET = ("        CHARSET $41,$5A,$C1",
                    "        CHARSET $61,$7A,$41")


def _petscii_char(b: int) -> str | None:
    """The source character CHARSET maps to this byte, or None if none does.

    Under the two ranges above, source $41-$5A becomes $C1-$DA and source
    $61-$7A becomes $41-$5A. Nothing maps to $61-$7A, so those bytes have to
    stay FCB however printable they look.
    """
    if 0xC1 <= b <= 0xDA:
        return chr(b - 0x80)          # shifted -> uppercase letter
    if 0x41 <= b <= 0x5A:
        return chr(b + 0x20)          # unshifted -> lowercase letter
    if 0x61 <= b <= 0x7A:
        return None                   # unreachable through the charset
    if 0x20 <= b < 0x7F and b not in (0x22, 0x5C):
        return chr(b)                 # digits and punctuation pass through
    return None


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


_DATA_KINDS = {"bytes", "string", "words", "ptr_table", "chargen", "petscii"}
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
    # Word tables name things too - a vector is an address like any other.
    for region in sidecar.regions:
        if region.kind not in _WORD_KINDS:
            continue
        if not (block.start <= region.start < block.end):
            continue
        for a in range(region.start, min(region.end, block.end) - 1, 2):
            v = int.from_bytes(data[a - base:a - base + 2], "little")
            if v in labels:
                continue
            name = sidecar.c64_symbols.get(v) or c64kernal.name_for(v)
            if name:
                external[v] = name

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

    def _flush_petscii() -> None:
        """Emit pending PETSCII, showing letters as letters."""
        while pending:
            run = 0
            while run < len(pending) and _petscii_char(pending[run]) is not None:
                run += 1
            if run >= _MIN_FCC:
                text = "".join(_petscii_char(b) for b in pending[:run])
                del pending[:run]
                for i in range(0, len(text), 40):
                    lines.append(f"{'':{_INDENT}}{'FCC':{_MNEM_WIDTH}}"
                                 f'"{text[i:i + 40]}"')
                continue
            take = BYTES_PER_FCB
            i = 1
            while i < min(len(pending), BYTES_PER_FCB):
                j = i
                while j < len(pending) and _petscii_char(pending[j]) is not None:
                    j += 1
                if j - i >= _MIN_FCC:
                    take = i
                    break
                i = max(j, i + 1)
            take = min(take, len(pending), BYTES_PER_FCB)
            chunk = bytes(pending[:take])
            del pending[:take]
            lines.append(f"{'':{_INDENT}}{'FCB':{_MNEM_WIDTH}}"
                         + ",".join(f"${b:02X}" for b in chunk))

    def _flush_ascii() -> None:
        """Emit pending data, showing runs of text as text."""
        while pending:
            run = 0
            while run < len(pending) and _printable(pending[run]):
                run += 1
            if run >= _MIN_FCC:
                text = bytes(pending[:run]).decode("latin-1")
                del pending[:run]
                for i in range(0, len(text), 40):
                    lines.append(f"{'':{_INDENT}}{'FCC':{_MNEM_WIDTH}}"
                                 f'"{text[i:i + 40]}"')
                continue
            # No text here, so emit a full FCB line and only stop early if a
            # real string starts partway through it. Breaking on every
            # printable byte fragmented the data into needless short lines.
            take = BYTES_PER_FCB
            i = 1
            while i < min(len(pending), BYTES_PER_FCB):
                j = i
                while j < len(pending) and _printable(pending[j]):
                    j += 1
                if j - i >= _MIN_FCC:
                    take = i
                    break
                i = max(j, i + 1)
            take = min(take, len(pending), BYTES_PER_FCB)
            chunk = bytes(pending[:take])
            del pending[:take]
            lines.append(f"{'':{_INDENT}}{'FCB':{_MNEM_WIDTH}}"
                         + ",".join(f"${b:02X}" for b in chunk))

    words_left = 0
    in_petscii = False
    def flush() -> None:
        """Flush through whichever encoding the pending bytes were collected
        under. Routing at each call site instead let a label or a word table
        flush PETSCII as plain ASCII, which CHARSET then shifted again."""
        if in_petscii:
            _flush_petscii()
        else:
            _flush_ascii()

    for rt, insn, byte in _decode_block(data, base, block, labels, sidecar):
        here = sidecar.region_at(rt + block.offset)
        if in_petscii and (here is None or here.kind != "petscii"):
            # Leave PETSCII before anything else is emitted, so the reset sits
            # with the text it applies to rather than after the next label.
            flush()
            lines.append(f"{'':{_INDENT}}CHARSET")
            in_petscii = False

        if rt in labels:
            flush()
            rom_addr = rt + block.offset
            block_comment = sidecar.block_comments.get(rom_addr)
            lines.append("")
            if block_comment:
                for line in block_comment.strip().splitlines():
                    lines.append(f"; {line}" if line else ";")
            lines.append(f"{labels[rt]}:")
            note = sidecar.line_comments.get(rom_addr)
            if note:
                lines.append(f"; {note}")
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
                                 + ",".join(all_names.get(v)
                                            or used_symbols.get(v)
                                            or f"${v:04X}" for v in vals))
                    words_left = count * 2
            if words_left:
                words_left -= 1
                continue
        if insn is None:
            reg = sidecar.region_at(rt + block.offset)
            if reg is not None and reg.kind == "petscii":
                if not in_petscii:
                    flush()
                    lines.extend(_PETSCII_CHARSET)
                    in_petscii = True
            pending.append(byte)
            continue
        flush()
        body = (f"{'':{_INDENT}}{insn.mnemonic:{_MNEM_WIDTH}}"
                f"{_operand(insn, all_names, used_symbols)}").rstrip()
        comment = sidecar.line_comments.get(rt + block.offset)
        # A label already printed this note above; repeating it on the first
        # instruction just doubles it.
        if comment and rt in labels:
            comment = None
        lines.append(f"{body:<40}; {comment}" if comment else body)
    if in_petscii:
        flush()
        lines.append(f"{'':{_INDENT}}CHARSET")
        in_petscii = False
    flush()
    lines.append("")
    lines.append(f"{'':{_INDENT}}END")
    return "\n".join(lines) + "\n"
