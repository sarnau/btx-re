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
    """The listing is pure 6801 now, so every target is inside the image and
    every one of them names a label."""
    offenders = [ln.strip() for ln in LISTING
                 if re.match(r"^\s+(B[A-Z]{2}|BRA|BSR|JMP|JSR)\s+\$[0-9A-F]{4}$",
                             ln.split(";")[0])]
    assert not offenders, offenders[:10]




def test_no_true_branch_uses_a_bare_address():
    branches = {"BPL", "BMI", "BVC", "BVS", "BCC", "BCS", "BNE", "BEQ", "BRA",
                "BRN", "BHI", "BLS", "BGE", "BLT", "BGT", "BLE", "BSR"}
    bad = [ln.strip() for ln in LISTING
           if (m := re.match(r"^\s+([A-Z]{2,4})\s+\$[0-9A-F]{2,4}$", ln.split(";")[0]))
           and m.group(1) in branches]
    assert not bad, bad[:10]


