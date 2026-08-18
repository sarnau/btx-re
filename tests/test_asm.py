import pytest

from dis65xx.asm import assemble


def test_assembles_a_minimal_program():
    src = """
        CPU     6801
        ORG     $8000
start:
        LDAA    #$01
        RTS
        FCB     $FF,$FE
        END
"""
    assert assemble(src) == (0x8000, bytes([0x86, 0x01, 0x39, 0xFF, 0xFE]))


def test_forced_extended_beats_shortest_fit():
    src = "        ORG $8000\n        STD >$00F2\n        END\n"
    assert assemble(src)[1] == bytes([0xFD, 0x00, 0xF2])


def test_shortest_fit_chooses_direct():
    src = "        ORG $8000\n        STD $00F2\n        END\n"
    assert assemble(src)[1] == bytes([0xDD, 0xF2])


def test_mnemonic_without_direct_form_uses_extended():
    src = "        ORG $8000\n        CLR $0000\n        END\n"
    assert assemble(src)[1] == bytes([0x7F, 0x00, 0x00])


def test_resolves_backward_and_forward_labels():
    src = """
        ORG     $8000
loop:   BRA     ahead
        NOP
ahead:  BRA     loop
        END
"""
    assert assemble(src)[1] == bytes([0x20, 0x01, 0x01, 0x20, 0xFB])


def test_equates_define_symbols_usable_as_operands():
    src = """
PORT1   EQU     $02
        ORG     $8000
        STAA    PORT1
        END
"""
    assert assemble(src)[1] == bytes([0x97, 0x02])


def test_indexed_and_fdb():
    src = "        ORG $8000\n        JMP $00,X\n        FDB $1234,$5678\n        END\n"
    assert assemble(src)[1] == bytes([0x6E, 0x00, 0x12, 0x34, 0x56, 0x78])


def test_comments_and_blank_lines_are_ignored():
    src = """
; a comment

        ORG     $8000
        NOP             ; trailing comment
        END
"""
    assert assemble(src)[1] == bytes([0x01])


def test_unknown_mnemonic_reports_the_line_number():
    with pytest.raises(ValueError, match="line 2"):
        assemble("        ORG $8000\n        FROB $01\n        END\n")


def test_undefined_symbol_is_reported():
    with pytest.raises(ValueError, match="undefined symbol 'nowhere'"):
        assemble("        ORG $8000\n        JMP nowhere\n        END\n")
