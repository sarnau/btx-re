"""Listing generation. Pure formatting — no analysis decisions are made here.

Output targets asl (Macro Assembler AS) syntax and is also accepted verbatim by
dis65xx.asm, which is what enforces byte-identity.
"""

from __future__ import annotations

import dataclasses

from dis65xx.decode import Insn
from dis65xx.opcodes import Mode, modes_for
from dis65xx.sidecar import Sidecar
from dis65xx.trace import CODE, TraceResult

BYTES_PER_FCB = 16
_INDENT = 8      # leading spaces before a mnemonic
_MNEM_WIDTH = 8  # column width the mnemonic is padded to


_JUMPS = ("JMP", "JSR")


def collect_targets(data: bytes, result: TraceResult, sidecar: Sidecar) -> dict[int, str]:
    """Auto-label every branch and jump target that has no name of its own.

    Named L<addr>, so a target reads as a label instead of a bare address. Only
    addresses inside the image qualify. The 6502 blocks are assembled from their
    own sources and linked in as binary, so nothing here needs to know about
    them.
    """
    base = result.base
    end = base + len(data)
    targets: set[int] = set()

    for insn in result.insns.values():
        if insn.mode is Mode.REL:
            targets.add(insn.operand)
        elif insn.mnemonic in _JUMPS and insn.mode in (Mode.EXT, Mode.DIR):
            targets.add(insn.operand)

    # Every slot of a dispatch table is a handler address, and a handler
    # reached only through the table is not a branch target anywhere - so
    # without this the table prints hex for exactly the entries that have no
    # other way of being named. The traced-as-code test is what keeps the
    # screen line tables out: those hold display RAM addresses, not code.
    for region in sidecar.regions:
        if region.kind != "ptr_table":
            continue
        for a in range(region.start, region.end - 1, 2):
            if not (base <= a < end - 1):
                continue
            word = int.from_bytes(data[a - base:a - base + 2], "big")
            if word in result.insns:
                targets.add(word)

    return {a: f"L{a:04X}" for a in targets
            if base <= a < end and a not in sidecar.labels}


def format_operand(insn: Insn, sidecar: Sidecar) -> str:
    mode = insn.mode
    value = insn.operand
    # A site name wins over the global one: the same byte is a mask here and an
    # index there, and only the call site knows which.
    site = sidecar.site_symbols.get(insn.addr)

    if mode is Mode.INH:
        return ""
    if mode is Mode.IMM8:
        return f"#${value:02X}"
    if mode is Mode.IMM16:
        # A 16-bit immediate is usually an address being loaded into X - a
        # table base to index, or a handler to install in a soft vector - so
        # it takes a name when one exists. Only exact matches qualify, and
        # nothing below $0100: the 6801 register file lives there, so small
        # constants collide with it and ADDD #$0002 would print as ADDD #PORT1.
        name = None
        if value >= 0x0100 and insn.addr not in sidecar.literal_immediates:
            name = sidecar.symbols.get(value) or sidecar.labels.get(value)
            if name is None:
                # PUL pre-increments, so a table walked with the stack pointer
                # is loaded one byte early. NAME-1 is what that means.
                after = sidecar.symbols.get(value + 1) or sidecar.labels.get(value + 1)
                # A C64 block is assembled separately and linked in as binary,
                # so its own labels do not exist here - but the block start
                # does, because this listing emits a label for it.
                blk = sidecar.c64_block_at(value + 1)
                if blk is not None:
                    after = (c64_block_label(blk.name)
                             if value + 1 == blk.start else None)
                if after is not None:
                    name = f"{after}-1"
            if name is None:
                # Otherwise an address inside a labelled data region is an
                # offset into it.
                reg = (None if sidecar.c64_block_at(value) is not None
                       else sidecar.region_at(value))
                if reg is not None and reg.kind in ("bytes", "string", "words",
                                                    "words_raw", "ptr_table",
                                                    "chargen"):
                    base_name = (sidecar.labels.get(reg.start)
                                 or sidecar.symbols.get(reg.start))
                    if base_name:
                        off = value - reg.start
                        # A large offset into a data region is far more likely
                        # to be a constant that happens to land there.
                        if off <= 128:
                            name = f"{base_name}+{off}"
        return f"#{name}" if name else f"#${value:04X}"
    if mode is Mode.IDX:
        return f"${value:02X},X"
    if mode is Mode.REL:
        return sidecar.labels.get(value) or f"${value:04X}"
    if mode is Mode.DIR:
        return site or sidecar.symbols.get(value) or f"${value:02X}"
    if mode is Mode.EXT:
        name = site or sidecar.symbols.get(value) or sidecar.labels.get(value)
        if name is None:
            # An address inside a labelled word table is an entry of it, so it
            # reads as an offset from the table rather than as a loose address.
            # Restricted to word regions: anywhere else an offset could name a
            # point inside an instruction.
            reg = sidecar.region_at(value)
            if reg is not None and reg.kind in ("words", "words_le", "ptr_table"):
                base_name = sidecar.labels.get(reg.start) or sidecar.symbols.get(reg.start)
                if base_name:
                    name = f"{base_name}+{value - reg.start}"
        # Force extended when a shortest-fit assembler would choose direct.
        prefix = ">" if value < 0x100 and Mode.DIR in modes_for(insn.mnemonic) else ""
        return prefix + (name if name else f"${value:04X}")

    raise AssertionError(f"unhandled mode {mode}")  # pragma: no cover


def _instruction_line(insn: Insn, sidecar: Sidecar) -> str:
    operand = format_operand(insn, sidecar)
    body = f"{'':{_INDENT}}{insn.mnemonic:{_MNEM_WIDTH}}{operand}".rstrip()
    comment = sidecar.line_comments.get(insn.addr)
    return f"{body:<40}; {comment}" if comment else body


def _fcb_line(chunk: bytes) -> str:
    values = ",".join(f"${b:02X}" for b in chunk)
    return f"{'':{_INDENT}}{'FCB':{_MNEM_WIDTH}}{values}"


def _equ_lines(sidecar: Sidecar, aliases: dict[str, int]) -> list[str]:
    """Declare sidecar symbols so the listing is self-contained.

    format_operand() renders hardware registers and RAM locations by name. Those
    addresses live outside the ROM image, so unlike code labels nothing in the
    listing defines them — without these EQUs the listing does not assemble.
    """
    named = {**{n: a for a, n in sidecar.symbols.items()}, **aliases}
    if not named:
        return []
    width = max(len(n) for n in named)
    lines = ["; Hardware registers and RAM locations referenced below.", ""]
    for name, addr in sorted(named.items(), key=lambda kv: (kv[1], kv[0])):
        value = f"${addr:02X}" if addr < 0x100 else f"${addr:04X}"
        lines.append(f"{name:{max(width, _INDENT - 1)}} EQU     {value}")
    lines.append("")
    return lines


# How far into a named data region a word may point and still print as an
# offset from it. The largest real case is the attribute plane's last row, at
# base+3840.
_MAX_OFFSET = 0x1000


def _word_name(word: int, names: dict[int, str], symbols: dict[int, str],
               offsets: bool = True) -> str:
    """The name of what a word points at, as NAME or NAME+offset."""
    if word in names:
        return names[word]
    # A row of a display plane is the plane's base plus a multiple of the row
    # stride, so it reads as an offset from the plane. Only data symbols
    # qualify: a code label plus an offset would be naming a point inside an
    # instruction, which is never what a table entry means.
    if not offsets:
        return f"${word:04X}"
    base = max((a for a in symbols if a <= word and word - a < _MAX_OFFSET),
               default=None)
    if base is not None:
        return f"{symbols[base]}+{word - base}"
    return f"${word:04X}"


def _fdb_line(words: list[int], names: dict[int, str] | None = None,
              symbols: dict[int, str] | None = None, resolve: bool = True,
              offsets: bool = True) -> str:
    """A word that is the address of something named prints the name.

    Every entry of a ptr_table is a handler address, and the hardware vectors
    are addresses too, so showing them as hex hides the one thing the table is
    for - which handler each slot reaches."""
    if not resolve:
        # A table of values, not addresses: $0000 is zero, not P1DDR.
        return (f"{'':{_INDENT}}{'FDB':{_MNEM_WIDTH}}"
                + ",".join(f"${w:04X}" for w in words))
    names, symbols = names or {}, symbols or {}
    values = ",".join(_word_name(w, names, symbols, offsets) for w in words)
    return f"{'':{_INDENT}}{'FDB':{_MNEM_WIDTH}}{values}"


def c64_block_label(name: str) -> str:
    """The label this listing gives a separately-assembled 6502 block."""
    return f"c64{name.capitalize()}Block"


def _printable(b: int) -> bool:
    """Safe inside an FCC run.

    '"' would close the string and '\\' starts an escape in asl, so a run
    containing either falls back to FCB rather than relying on two assemblers
    agreeing about quoting."""
    return 0x20 <= b < 0x7F and b not in (0x22, 0x5C)


def emit(data: bytes, result: TraceResult, sidecar: Sidecar) -> str:
    # Branch and jump targets become labels. The sidecar's own names win, so a
    # routine that has been identified keeps its name instead of an L-address.
    # Collecting changes the emitter's framing (a label splits an instruction),
    # which can expose branches that were not visible on the previous pass, so
    # iterate until the label set stops growing.
    auto = collect_targets(data, result, sidecar)
    sidecar = dataclasses.replace(sidecar, labels={**auto, **sidecar.labels})
    names = {**sidecar.symbols, **sidecar.labels}


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
    # A site name needs an EQU of its own, and its value comes from the
    # instruction it names - so a site address that is not a decoded
    # instruction is an error rather than a silent no-op.
    aliases: dict[str, int] = {}
    for addr, name in sidecar.site_symbols.items():
        insn = result.insns.get(addr)
        if insn is None or insn.operand is None:
            raise ValueError(f"site_symbols: ${addr:04X} is not an instruction "
                             f"with an operand")
        aliases[name] = insn.operand
    lines += _equ_lines(sidecar, aliases)
    lines += [
        f"{'':{_INDENT}}{'ORG':{_MNEM_WIDTH}}${base:04X}",
        "",
    ]

    addr = base
    end = base + len(data)
    pending: list[int] = []  # unclassified bytes waiting to be flushed as FCB
    in_string = False        # the pending run sits in a string region
    text_width = BYTES_PER_FCB

    def flush() -> None:
        if in_string:
            _flush_text()
            return
        while pending:
            chunk = bytes(pending[:BYTES_PER_FCB])
            del pending[:BYTES_PER_FCB]
            lines.append(_fcb_line(chunk))

    def _flush_text() -> None:
        """Show text as text, dropping to FCB for the bytes that are not.

        A short printable run inside binary reads worse as a one-word string
        than as the bytes it sits among, so runs under four characters stay
        FCB."""
        while pending:
            run = 0
            while run < len(pending) and _printable(pending[run]):
                run += 1
            if run >= 4:
                text = bytes(pending[:run]).decode("ascii")
                del pending[:run]
                for i in range(0, len(text), text_width):
                    piece = text[i:i + text_width]
                    lines.append(f"{'':{_INDENT}}{'FCC':{_MNEM_WIDTH}}\"{piece}\"")
                continue
            take = max(run, 1)
            while take < len(pending) and not _printable(pending[take]):
                take += 1
            chunk = bytes(pending[:take])
            del pending[:take]
            for i in range(0, len(chunk), BYTES_PER_FCB):
                lines.append(_fcb_line(chunk[i:i + BYTES_PER_FCB]))

    while addr < end:
        c64 = sidecar.c64_block_at(addr)
        if c64 is not None and addr == c64.start:
            flush()
            lines.append("")
            # A label so the block has a name in this listing too - the 6502
            # source defines its own, but they live in a separate assembly.
            lines.append(f"{c64_block_label(c64.name)}:")
            lines.append(f"; {c64.name}: 6502 code, assembled separately at "
                         f"${c64.org:04X}. See out/c64_{c64.name}.asm.")
            lines.append(f"{'':{_INDENT}}BINCLUDE \"c64_{c64.name}.bin\"")
            lines.append("")
            addr = c64.end
            continue
        # Annotations inside a 6502 block belong to that block's own source,
        # not here, so nothing from it leaks into the 6801 listing.
        if sidecar.c64_block_at(addr) is not None:
            addr += 1
            continue

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


        if region is not None and region.kind == "words_le" and not is_code:
            # Stored low byte first, like the font rows, because the value goes
            # straight into a display word rather than through the CPU.
            flush()
            count = min(8, (min(region.end, end) - addr) // 2)
            if count:
                vals = [int.from_bytes(data[addr - base + 2*i: addr - base + 2*i + 2],
                                       "little") for i in range(count)]
                lines.append(f"{'':{_INDENT}}{'DW':{_MNEM_WIDTH}}"
                             + ",".join(f"${v:04X}" for v in vals))
                addr += 2 * count
                continue

        if (region is not None
                and region.kind in ("words", "words_raw", "ptr_table")
                and not is_code):
            flush()
            count = min(8, (min(region.end, end) - addr) // 2)
            if count:
                words = [
                    int.from_bytes(data[addr - base + 2 * i: addr - base + 2 * i + 2], "big")
                    for i in range(count)
                ]
                lines.append(_fdb_line(
                    words, names, sidecar.symbols,
                    resolve=region.kind != "words_raw",
                    offsets=region.kind == "ptr_table"))
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

        text_here = region is not None and region.kind == "string"
        width_here = (region.width or BYTES_PER_FCB) if text_here else BYTES_PER_FCB
        if text_here != in_string or width_here != text_width:
            flush()
            in_string, text_width = text_here, width_here
        pending.append(data[addr - base])
        addr += 1
        # Break the FCB run at any address that carries its own annotation.
        if addr < end and (addr in sidecar.labels or addr in sidecar.block_comments):
            flush()

    flush()
    lines.append("")
    lines.append(f"{'':{_INDENT}}END")
    return "\n".join(lines) + "\n"
