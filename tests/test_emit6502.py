"""The listing mixes two instruction sets; these guard the switch."""

import build
from dis65xx.asm import assemble


def test_listing_switches_cpu_around_the_6502_regions():
    listing = build.run(write=False).listing
    assert "CPU     6502" in listing
    assert listing.count("CPU     6502") == listing.count("CPU     6801") - 1  # 1 header


def test_6502_region_disassembles_rather_than_dumping_bytes():
    listing = build.run(write=False).listing
    body = listing.split("c64ColdStart:", 1)[1][:600]
    for expected in ("LDX     #$00", "STX     $D016", "JSR     $FDA3", "STA     ($61),Y"):
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
