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


def _entry_interiors(sidecar: Sidecar, block: C64Block) -> set[int]:
    """Addresses inside a table entry but not at its start.

    An entry's first byte can carry a label; the rest cannot, because a label
    there would collapse onto the following entry. Treating every byte after
    the region start as interior was too broad - it denied labels to entries
    two, three and four of a table as well.
    """
    out: set[int] = set()
    for region in sidecar.regions:
        if not (block.start <= region.start < block.end):
            continue
        step = 2 if region.kind in _WORD_KINDS else 3 if region.kind == "byte_word" else 0
        if not step:
            continue
        for entry in range(region.start, min(region.end, block.end), step):
            out |= {a - block.offset for a in range(entry + 1, min(entry + step, region.end))}
    return out


def _interior_name(v: int, labels: dict[int, str], span: int = 4) -> str | None:
    """`label+n` for an address inside a multi-byte entry.

    Those interiors carry no label of their own - one there would collapse onto
    the next entry - but the code does reference them, so name them relative to
    the entry they belong to.
    """
    for back in range(1, span + 1):
        if v - back in labels:
            return f"{labels[v - back]}+{back}"
    return None


def _operand(insn, labels: dict[int, str], symbols: dict[int, str] | None = None,
             interiors: set[int] | None = None) -> str:
    m, v = insn.mode, insn.operand
    if m is Mode.IMP:
        return ""
    if m is Mode.ACC:
        return "A"
    if m is Mode.IMM:
        return f"#${v:02X}"
    symbols = symbols or {}

    def sym(addr: int, width: int) -> str | None:
        """A symbol, or SYM+1 for the second byte of a named two-byte field.

        The ROM source names only the first byte of TIME, FNADR and CINV, and
        C64 listings write the others as FNADR+1 - so do that rather than
        invent a name the source does not have."""
        if addr in symbols:
            return symbols[addr]
        if addr - 1 in symbols:
            return f"{symbols[addr - 1]}+1"
        if addr - 2 in symbols:
            return f"{symbols[addr - 2]}+2"
        return None

    if m is Mode.ZP:
        return sym(v, 2) or f"${v:02X}"
    # These all name a zero-page location - a pointer for the indirect forms -
    # so they take a symbol just as a plain zero-page operand does.
    if m is Mode.ZPX:
        return f"{sym(v, 2) or f'${v:02X}'},X"
    if m is Mode.ZPY:
        return f"{sym(v, 2) or f'${v:02X}'},Y"
    if m is Mode.IZX:
        return f"({sym(v, 2) or f'${v:02X}'},X)"
    if m is Mode.IZY:
        return f"({sym(v, 2) or f'${v:02X}'}),Y"
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
        name = sym(v, 3) or labels.get(v)
        if name is None and interiors is not None and v in interiors:
            name = _interior_name(v, labels)
    pre = ">" if v < 0x100 and name is None else ""
    body = name or f"${v:04X}"
    if m is Mode.ABS:
        return pre + body
    if m is Mode.ABX:
        return pre + body + ",X"
    if m is Mode.ABY:
        return pre + body + ",Y"
    raise AssertionError(m)  # pragma: no cover


_DATA_KINDS = {"bytes", "string", "words", "ptr_table", "chargen", "petscii",
               "byte_word"}
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
        if not (block.start <= region.start < block.end):
            continue
        if region.kind in _WORD_KINDS:
            for a in range(region.start, min(region.end, block.end) - 1, 2):
                table_targets.add(int.from_bytes(data[a - base:a - base + 2], "little"))
        elif region.kind == "byte_word":
            # records of one byte then a 16-bit address
            for a in range(region.start, min(region.end, block.end) - 2, 3):
                table_targets.add(int.from_bytes(data[a + 1 - base:a + 3 - base], "little"))

    labels = dict(named)
    for _ in range(8):
        targets: set[int] = set(table_targets)
        seq = [i for _r, i, _b in _decode_block(data, base, block, labels, sidecar)]
        for insn in seq:
            if insn is None:
                continue
            if insn.mode is Mode.REL:
                targets.add(insn.operand)
            elif insn.mode is Mode.ABS and insn.mnemonic in _JUMPS:
                targets.add(insn.operand)
            elif insn.mode in (Mode.ABS, Mode.ABX, Mode.ABY):
                # A data reference into this block names a location in it, so
                # it deserves a label as much as a jump target does - unless it
                # is a hardware register that merely happens to fall in the
                # block's address range, as the BTX window does inside the
                # bootstrap. Those already have names of their own.
                if insn.operand not in sidecar.c64_symbols:
                    targets.add(insn.operand)
        # An address assembled into a zero-page pointer is a location too.
        targets.update(v for v in _pointer_targets(seq, set(sidecar.c64_pointers)).values()
                       if block.org <= v < block.org + (block.end - block.start))
        # A label cannot sit inside a multi-byte entry. The code reads these
        # tables a byte at a time - LDA table+1,Y for a record's address, or
        # the high half of a pointer - and a label there would collapse onto
        # the next entry, silently changing the operand. Those stay numeric.
        targets -= _entry_interiors(sidecar, block)

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


def _pointer_targets(decoded: list, pairs: set[int]) -> dict[tuple[int, int], int]:
    """Addresses assembled into a zero-page pointer, keyed by the pair of LDA
    addresses that supply the low and high bytes."""
    out: dict[tuple[int, int], int] = {}
    for i in range(len(decoded) - 3):
        a, b, c, d = decoded[i:i + 4]
        if not all(x is not None for x in (a, b, c, d)):
            continue
        # Any register will do, as long as each load is followed by the store
        # of the same one: LDA/STA, LDX/STX or LDY/STY.
        if a.mnemonic not in ("LDA", "LDX", "LDY"):
            continue
        reg = a.mnemonic[2]
        if (b.mnemonic, c.mnemonic, d.mnemonic) != (f"ST{reg}", a.mnemonic, f"ST{reg}"):
            continue
        if a.mode is not Mode.IMM or c.mode is not Mode.IMM:
            continue
        # The store can be zero page or absolute - CINV/CINVH at $0314 is a
        # pointer pair just as much as a zero-page one is.
        if b.mode not in (Mode.ZP, Mode.ABS) or d.mode not in (Mode.ZP, Mode.ABS):
            continue
        if b.mode is not d.mode:
            continue
        if b.operand not in pairs or d.operand != b.operand + 1:
            continue
        out[(a.addr, c.addr)] = a.operand | (c.operand << 8)
    return out


def _pointer_loads(decoded: list, labels: dict[int, str],
                   pairs: set[int]) -> dict[int, str]:
    """Find `LDA #lo / STA zp / LDA #hi / STA zp+1` and name the address.

    Those four instructions set up a 16-bit pointer, so the two immediates are
    halves of one address rather than independent constants.
    """
    out: dict[int, str] = {}
    for (lo_addr, hi_addr), target in _pointer_targets(decoded, pairs).items():
        name = labels.get(target)
        if not name:
            continue
        out[lo_addr] = f"{name}&255"
        out[hi_addr] = f"{name}>>8"
    return out


def emit_block(data: bytes, base: int, block: C64Block, sidecar: Sidecar) -> str:
    labels = collect_labels(data, base, block, sidecar)

    # External references: C64 ROM entry points and anything outside this block.
    # `external` is what gets an EQU; `extern_ops` is what operands print, and
    # the two differ where a name is an offset - SIZE+8 prints, SIZE declares.
    external: dict[int, str] = {}
    extern_ops: dict[int, str] = {}
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
            # A name like SIZE+8 is an offset into a labelled routine, so what
            # needs declaring is the base symbol, not the expression.
            extern_ops[v] = name
            if "+" in name:
                base_name, off = name.split("+", 1)
                external[v - int(off)] = base_name
            else:
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
            if name and "+" not in name:
                external[v] = name

    # Which hardware/RAM symbols this block actually touches.
    touched: dict[int, str] = {}
    for _rt, insn, _b in _decode_block(data, base, block, labels, sidecar):
        if insn is None or insn.operand is None:
            continue
        if insn.mnemonic in _JUMPS and insn.mode is not Mode.IND:
            continue
        # An immediate is a value, not an address. LDA #$6C is a CEPT byte and
        # must not drag $006C's symbol into the EQU list.
        if insn.mode is Mode.IMM:
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

    all_names = {**external, **extern_ops, **labels}
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

    interiors = _entry_interiors(sidecar, block)

    decoded = [insn for _rt, insn, _b in _decode_block(data, base, block, labels, sidecar)]
    ptr_loads = _pointer_loads(decoded, labels, set(sidecar.c64_pointers))

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
        if region is not None and region.kind == "byte_word" and insn is None:
            off = rt + block.offset - region.start
            if off % 3 == 0 and rt + block.offset + 3 <= min(region.end, block.end):
                flush()
                lo = rt + block.offset
                key = data[lo - base]
                target = int.from_bytes(data[lo + 1 - base:lo + 3 - base], "little")
                shown = chr(key) if 0x20 <= key < 0x7F else f"${key:02X}"
                lines.append(f"{'':{_INDENT}}{'FCB':{_MNEM_WIDTH}}${key:02X}"
                             f"{'':<20}; '{shown}'")
                lines.append(f"{'':{_INDENT}}{'DW':{_MNEM_WIDTH}}"
                             f"{all_names.get(target) or f'${target:04X}'}")
                words_left = 2
                continue
            if words_left:
                words_left -= 1
                continue

        if region is not None and region.kind in _WORD_KINDS and insn is None:
            if words_left == 0:
                flush()
                lo = rt + block.offset
                count = min(8, (min(region.end, block.end) - lo) // 2)
                # stop the group where the next label begins, or it would be
                # swallowed and collapse onto this line's address
                for k in range(1, count):
                    if (lo + 2 * k) - block.offset in labels:
                        count = k
                        break
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
        text = ptr_loads.get(rt)
        operand = (f"#{text}" if text
                   else _operand(insn, all_names, used_symbols, interiors))
        body = f"{'':{_INDENT}}{insn.mnemonic:{_MNEM_WIDTH}}{operand}".rstrip()
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
