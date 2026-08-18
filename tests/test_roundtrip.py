import build


def test_full_rom_round_trips_byte_identical():
    result = build.run(write=False)
    assert result.assembled == result.rom, (
        f"round-trip mismatch at offset {result.first_mismatch}"
    )


def test_coverage_is_reported():
    result = build.run(write=False)
    assert result.code_bytes + result.unknown_bytes == len(result.rom)
    assert result.code_bytes > 0


def test_listing_is_not_empty():
    result = build.run(write=False)
    assert "CPU     6801" in result.listing
    assert result.listing.count("\n") > 1000
