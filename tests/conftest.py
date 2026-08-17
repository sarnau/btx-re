import hashlib
import pathlib

import pytest

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
ROM_PATH = REPO_ROOT.parent / "C64 BTX Decoder" / "c64_btx_decoder_ii.bin"
ROM_SHA256 = "2799910767fdb7067fb91f72aef691bf692a8f7a2f2b26acb71d3d1ca3f68926"
ROM_BASE = 0x8000


@pytest.fixture(scope="session")
def rom() -> bytes:
    data = ROM_PATH.read_bytes()
    assert hashlib.sha256(data).hexdigest() == ROM_SHA256, "ROM fixture does not match the spec"
    return data
