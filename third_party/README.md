# Third-party sources

Kept here because both are needed to reproduce this project's verification and
neither has a stable versioned URL.

| File | Size | SHA-256 |
|---|---|---|
| `asl-current.tar.gz` | 3.4 MB | `b3213b8f6b9dace8eec06e1bdffdfa5a937fa1a6e588edf0205918220e67d6f8` |
| `c64rom.tar.gz` | 0.1 MB | `fa1bd2f0436ce03c17cf05dc014e8c3d02779998fa10ce0e0e2b1043836b724b` |

Retrieved 2026-08-19.

## `asl-current.tar.gz`

Alfred Arnold's Macro Assembler AS, from

    http://john.ccac.rwth-aachen.de:8000/ftp/as/source/c_version/asl-current.tar.gz

It is the independent assembler `tools/check_asl.sh` uses to confirm the
generated listings, and the second opinion the whole byte-identity claim rests
on — `dis65xx/asm.py` shares its opcode table with the disassembler, so it
cannot catch a wrong table entry on its own. asl has caught real errors here:
that `DW` follows the target's endianness on a 6801, and four cases where
`asm.py` accepted syntax asl rejects.

There is no versioned URL. "asl-current" is a moving target, the server was
unreachable for several minutes on the day this was archived, and it is not in
Homebrew — so a copy is the difference between the cross-check being
reproducible and not.

Build it with:

    tar xzf asl-current.tar.gz && cd asl-current
    cp Makefile.def-samples/Makefile.def-<platform> Makefile.def
    make

`Makefile.def-arm-osx` is the right pick on Apple silicon. The build produces
`asl` and `p2bin`, both of which `check_asl.sh` needs on PATH.

## `c64rom.tar.gz`

Michael Steil's reconstruction of the C64 ROM sources, from

    https://github.com/mist64/c64rom

at revision `5af165d73077625cbb0ebde68c603d7927a6110f`, `.git` removed.

Built with cc65, it is where every C64 ROM name in `dis65xx/c64kernal.py` comes
from — addresses read off the linker's symbol table rather than from memory.
That mattered: earlier versions of this project used invented names like
`IEC_CIOUT` and `KERNAL_CLOSE`, and the real ones are `CIOUT` and `CLSEI`. It
also settled `LDTB1`, `LDTB2` and `NNMI20`.

    tar xzf c64rom.tar.gz && cd c64rom && make

## What is not kept

The built binaries. They are platform-specific and rebuildable from these
sources in about a minute, and committing them would put several megabytes of
architecture-dependent object code into the history permanently.
