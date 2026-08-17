from conftest import ROM_BASE


def test_rom_is_32k(rom):
    assert len(rom) == 32768


def test_reset_vector_is_b200(rom):
    # RESET vector lives at $FFFE, i.e. the last two bytes of the image.
    reset = int.from_bytes(rom[-2:], "big")
    assert reset == 0xB200
