"""A two-pass MC6801 assembler.

Accepts exactly the syntax dis6801.emit produces, which is a subset of asl's.
Its only job is to prove the generated listing reassembles to the original ROM,
so it deliberately supports no macros, expressions, or conditional assembly.
"""

from __future__ import annotations

import re

from dis6801.encode import encode
from dis6801.opcodes import SIZE, Mode, modes_for

_LABEL = re.compile(r"^(?P<label>[A-Za-z_][A-Za-z0-9_]*):?\s*(?P<rest>.*)$")


class _Line:
    __slots__ = ("no", "label", "op", "operand")

    def __init__(self, no: int, label: str | None, op: str | None, operand: str):
        self.no = no
        self.label = label
        self.op = op
        self.operand = operand


def _parse(source: str) -> list[_Line]:
    out: list[_Line] = []
    for no, raw in enumerate(source.splitlines(), start=1):
        text = raw.split(";", 1)[0].rstrip()
        if not text.strip():
            continue

        label = None
        if not text[0].isspace():
            m = _LABEL.match(text.strip())
            if not m:
                raise ValueError(f"line {no}: cannot parse {raw!r}")
            label = m.group("label")
            text = m.group("rest")

        parts = text.strip().split(None, 1)
        op = parts[0].upper() if parts else None
        operand = parts[1].strip() if len(parts) > 1 else ""
        out.append(_Line(no, label, op, operand))
    return out


def _split_values(operand: str) -> list[str]:
    return [v.strip() for v in operand.split(",") if v.strip()]


def _value(token: str, symbols: dict[str, int], line_no: int) -> int:
    token = token.strip()
    if token.startswith("$"):
        return int(token[1:], 16)
    if token.startswith("%"):
        return int(token[1:], 2)
    if re.fullmatch(r"-?\d+", token):
        return int(token)
    if token in symbols:
        return symbols[token]
    raise ValueError(f"line {line_no}: undefined symbol {token!r}")


def _sizeof(line: _Line, symbols: dict[str, int], strict: bool) -> int:
    """Byte length of one line. On pass 1 unknown symbols are assumed 16-bit."""
    op = line.op
    if op in (None, "CPU", "ORG", "END", "EQU"):
        return 0
    if op == "FCB":
        return len(_split_values(line.operand))
    if op == "FDB":
        return 2 * len(_split_values(line.operand))

    mode, _ = _resolve_mode(line, symbols, strict=strict)
    return SIZE[mode]


def _resolve_mode(line: _Line, symbols: dict[str, int], *, strict: bool) -> tuple[Mode, int | None]:
    """Determine addressing mode and operand value for an instruction line."""
    mnemonic = line.op
    available = modes_for(mnemonic)
    if not available:
        raise ValueError(f"line {line.no}: unknown mnemonic {mnemonic!r}")

    text = line.operand

    if not text:
        if Mode.INH not in available:
            raise ValueError(f"line {line.no}: {mnemonic} requires an operand")
        return Mode.INH, None

    if text.startswith("#"):
        mode = Mode.IMM16 if Mode.IMM16 in available else Mode.IMM8
        if mode not in available:
            raise ValueError(f"line {line.no}: {mnemonic} has no immediate form")
        return mode, _resolve_value(text[1:], symbols, line, strict)

    if text.upper().endswith(",X"):
        if Mode.IDX not in available:
            raise ValueError(f"line {line.no}: {mnemonic} has no indexed form")
        return Mode.IDX, _resolve_value(text[:-2], symbols, line, strict)

    if Mode.REL in available:
        return Mode.REL, _resolve_value(text, symbols, line, strict)

    forced_ext = text.startswith(">")
    forced_dir = text.startswith("<")
    body = text[1:] if (forced_ext or forced_dir) else text
    value = _resolve_value(body, symbols, line, strict)

    if forced_ext:
        if Mode.EXT not in available:
            raise ValueError(f"line {line.no}: {mnemonic} has no extended form")
        return Mode.EXT, value
    if forced_dir:
        if Mode.DIR not in available:
            raise ValueError(f"line {line.no}: {mnemonic} has no direct form")
        return Mode.DIR, value

    if value is not None and value < 0x100 and Mode.DIR in available:
        return Mode.DIR, value
    if Mode.EXT in available:
        return Mode.EXT, value
    if Mode.DIR in available:
        return Mode.DIR, value
    raise ValueError(f"line {line.no}: {mnemonic} takes no memory operand")


def _resolve_value(token: str, symbols: dict[str, int], line: _Line, strict: bool) -> int | None:
    try:
        return _value(token, symbols, line.no)
    except ValueError:
        if strict:
            raise
        return None  # pass 1: unknown forward reference


def assemble(source: str) -> tuple[int, bytes]:
    """Assemble `source`, returning (origin, bytes)."""
    lines = _parse(source)
    symbols: dict[str, int] = {}

    # Pass 1: assign addresses to labels.
    pc = 0
    origin = None
    for line in lines:
        if line.op == "ORG":
            pc = _value(line.operand, symbols, line.no)
            if origin is None:
                origin = pc
        if line.label:
            if line.op == "EQU":
                symbols[line.label] = _value(line.operand, symbols, line.no)
                continue
            symbols[line.label] = pc
        if line.op == "EQU":
            continue
        pc += _sizeof(line, symbols, strict=False)

    if origin is None:
        raise ValueError("no ORG directive")

    # Pass 2: emit.
    out = bytearray()
    pc = origin
    for line in lines:
        op = line.op
        if op == "ORG":
            target = _value(line.operand, symbols, line.no)
            if target < pc:
                raise ValueError(f"line {line.no}: ORG moves backwards")
            out.extend(bytes(target - pc))
            pc = target
            continue
        if op in (None, "CPU", "END", "EQU"):
            continue
        if op == "FCB":
            for token in _split_values(line.operand):
                value = _value(token, symbols, line.no)
                if not 0 <= value <= 0xFF:
                    raise ValueError(f"line {line.no}: ${value:X} does not fit in a byte")
                out.append(value)
                pc += 1
            continue
        if op == "FDB":
            for token in _split_values(line.operand):
                value = _value(token, symbols, line.no)
                out.extend(value.to_bytes(2, "big"))
                pc += 2
            continue

        mode, operand = _resolve_mode(line, symbols, strict=True)
        try:
            out.extend(encode(op, mode, operand, addr=pc))
        except ValueError as exc:
            raise ValueError(f"line {line.no}: {exc}") from exc
        pc += SIZE[mode]

    return origin, bytes(out)
