"""The listing mixes two instruction sets; these guard the switch."""

import build
from dis65xx.asm import assemble


def test_cpu_switches_once_around_a_single_span():
    """One CPU 6502 span covers all the C64-side code, header and text alike.

    The directive is an assembler mode over a range, not a per-region marker -
    data inside the span still renders as FCB, just under CPU 6502.
    """
    lines = build.run(write=False).listing.splitlines()
    cpu = [(i, ln.split()[1]) for i, ln in enumerate(lines) if ln.strip().startswith("CPU")]
    assert [c for _, c in cpu] == ["6801", "6502", "6801"], cpu

    def line_of(label):
        return next(i for i, ln in enumerate(lines) if ln.startswith(label + ":"))

    # 6502 mode opens before the cartridge header and closes before ctrlTableC0
    assert cpu[1][0] < line_of("c64CartHeader")
    assert line_of("c64Payload") < cpu[2][0] < line_of("ctrlTableC0")


def test_data_inside_the_span_still_renders_as_fcb():
    lines = build.run(write=False).listing.splitlines()
    start = next(i for i, ln in enumerate(lines) if ln.startswith("c64CartHeader:"))
    assert "FCB" in lines[start + 1]


def test_6502_region_disassembles_rather_than_dumping_bytes():
    listing = build.run(write=False).listing
    body = listing.split("c64ColdStart:", 1)[1][:600]
    for expected in ("LDX     #$00", "STX     $D016", "JSR     IOINIT", "STA     ($61),Y"):
        assert expected in body, expected


def test_mixed_cpu_source_round_trips_through_our_assembler():
    src = """
        CPU     6502
        ORG     $8000
        LDA     #$01
        STA     >$0002
        JSR     $FDA3
        STA     ($61),Y
        ASL     A
        JMP     ($8000)
        CPU     6801
        LDAA    #$02
        STAA    >$0003
        END
"""
    _, out = assemble(src)
    assert out == bytes([0xA9, 0x01, 0x8D, 0x02, 0x00, 0x20, 0xA3, 0xFD,
                         0x91, 0x61, 0x0A, 0x6C, 0x00, 0x80,
                         0x86, 0x02, 0xB7, 0x00, 0x03])


def test_zero_page_versus_absolute_is_preserved():
    # $85 is zero-page STA, $8D absolute; the > prefix must force the long form.
    assert assemble("        CPU 6502\n        ORG $8000\n        STA $02\n        END\n")[1] \
        == bytes([0x85, 0x02])
    assert assemble("        CPU 6502\n        ORG $8000\n        STA >$0002\n        END\n")[1] \
        == bytes([0x8D, 0x02, 0x00])
