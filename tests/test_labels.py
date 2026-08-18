"""Branch and jump targets read as labels, and every label resolves."""

import re

import build

LISTING = build.run(write=False).listing.splitlines()
AUTO = re.compile(r"L[0-9A-F]{4}")


def _defined():
    return {m.group(1) for ln in LISTING
            if (m := re.match(r"^([A-Za-z_][A-Za-z0-9_]*):", ln))}


def test_every_auto_label_used_is_defined():
    defined = _defined()
    used = set()
    for ln in LISTING:
        if ln.startswith((" ", "\t")):
            used |= set(AUTO.findall(ln.split(";")[0]))
    assert used, "expected the listing to reference auto labels"
    assert not (used - defined), sorted(used - defined)[:10]


def test_no_6801_branch_uses_a_bare_address():
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


def test_6502_absolute_operands_stay_literal_but_are_cross_referenced():
    """The payload runs at $1000 and the bootstrap at $8000, so an absolute
    operand names a location in neither the listing's address space nor the
    other block's. Turning one into a label would assemble to the ROM address
    and change the bytes, so they stay literal and carry an xref comment."""
    body = "\n".join(LISTING)
    assert "JMP     $16AE                   ; c64ScreenInit ($BA56)" in body
    assert "JMP     $174C                   ; c64Vec02 ($BAF4)" in body
    # the bootstrap's byte-fetch routine, reached through the $8000 window
    assert "JSR     $804D                   ; LB37A ($B37A)" in body


def test_relative_branches_do_use_labels_on_both_cpus():
    body = "\n".join(LISTING)
    assert "BCS     LB372" in body          # 6502 bootstrap
    assert re.search(r"^\s+BNE\s+L[0-9A-F]{4}$", body, re.M)
