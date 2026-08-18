"""Branch and jump targets read as labels, and every label resolves."""

import re

import build

LISTING = build.run(write=False).listing.splitlines()
AUTO = re.compile(r"L[0-9A-F]{4}")


def _defined():
    """Listing labels plus EQU symbols - both are definitions to the assembler."""
    out = set()
    for ln in LISTING:
        if m := re.match(r"^([A-Za-z_][A-Za-z0-9_]*):", ln):
            out.add(m.group(1))
        elif m := re.match(r"^([A-Za-z_][A-Za-z0-9_]*)\s+EQU\s", ln):
            out.add(m.group(1))
    return out


def test_every_auto_label_used_is_defined():
    defined = _defined()
    used = set()
    for ln in LISTING:
        if ln.startswith((" ", "\t")):
            used |= set(AUTO.findall(ln.split(";")[0]))
    assert used, "expected the listing to reference auto labels"
    assert not (used - defined), sorted(used - defined)[:10]


def test_no_6801_control_transfer_uses_a_bare_address():
    """On the 6801 side every target is inside the image, so all of them name a
    label. The 6502 side is different - see the test below."""
    cpu = "6801"
    offenders = []
    for ln in LISTING:
        t = ln.strip()
        if t.startswith("CPU"):
            cpu = t.split()[1]
            continue
        if cpu == "6801" and re.match(r"^(B[A-Z]{2}|BRA|BSR|JMP|JSR)\s+\$[0-9A-F]{4}$", t):
            offenders.append(t)
    assert not offenders, offenders[:10]


def test_6502_absolute_targets_use_equ_symbols():
    """The payload runs at $1000 and the bootstrap at $8000, so an absolute
    operand names a C64 location, not a position in this listing. Defining it as
    a listing label would assemble to the ROM address and change the bytes, so
    it becomes an EQU to the address itself - symbolic, and byte-identical."""
    body = "\n".join(LISTING)
    assert "JMP     L16AE                   ; c64ScreenInit ($BA56)" in body
    assert "JMP     L174C                   ; c64Vec02 ($BAF4)" in body
    assert "JSR     L804D                   ; LB37A ($B37A)" in body
    assert re.search(r"^L174C\s+EQU\s+\$174C$", body, re.M)


def test_kernal_calls_use_kernal_names_not_l_addresses():
    """A KERNAL address must not borrow an L<addr> name: it would share a
    namespace with a listing label at the same ROM address on the 6801 side."""
    body = "\n".join(LISTING)
    for name, addr in (("IOINIT", 0xFDA3), ("CINT", 0xFF5B), ("RESTOR", 0xFD15),
                       ("IEC_CIOUT", 0xEDDD), ("GETIN", 0xFFE4)):
        assert re.search(rf"^{name}\s+EQU\s+\${addr:04X}$", body, re.M), name
        assert f"L{addr:04X}" not in body, f"L{addr:04X} should not exist"


def test_no_true_branch_uses_a_bare_address():
    branches = {"BPL", "BMI", "BVC", "BVS", "BCC", "BCS", "BNE", "BEQ", "BRA",
                "BRN", "BHI", "BLS", "BGE", "BLT", "BGT", "BLE", "BSR"}
    bad = [ln.strip() for ln in LISTING
           if (m := re.match(r"^\s+([A-Z]{2,4})\s+\$[0-9A-F]{2,4}$", ln.split(";")[0]))
           and m.group(1) in branches]
    assert not bad, bad[:10]


def test_relative_branches_do_use_labels_on_both_cpus():
    body = "\n".join(LISTING)
    assert "BCS     LB372" in body          # 6502 bootstrap
    assert re.search(r"^\s+BNE\s+L[0-9A-F]{4}$", body, re.M)
