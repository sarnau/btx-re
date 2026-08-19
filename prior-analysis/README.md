# Prior analysis

An earlier IDA Pro pass over the same ROM, kept for reference. It is not part
of the build and nothing here reads it.

## Where it came from

The `C64-BTX-Module` repository (`github.com/sarnau/C64-BTX-Module`), commit
`f7b0416`, directory `C64 BTX Decoder/`. That repository once held this whole
project; its contents were moved here, but this directory was dropped from disk
during the move and survived only in git history. It was extracted with
`git archive` before that repository was retired locally.

The two ROM images that sat alongside it are not duplicated here — they are at
the repository root, and both were verified byte-identical to the copies in
that commit before this extraction.

## What is in it

    c64_btx_decoder_ii.bin.asm   11,876-line IDA listing
    c64_btx_decoder_ii.bin.idb   the IDA database it came from
    BTX_6502.gpr, BTX_6502.rep/  IDA project wrapper (.gpr is empty)

The listing records the same ROM this project disassembles: its header gives
`Input SHA256 : 2799910767FDB706...`, which is the hash `sidecar/decoder_ii.toml`
pins under `meta.rom`.

## How it relates to this project

It is independent evidence, not an authority. It covers the CEPT attribute
decoding in most detail — `PARSE_CODE_S_ABK`, `PARSE_CODE_S_MSR` and the
`COLOR_*` enumeration correspond to the `c1a*` handlers here — so it is useful
for cross-checking names in that area. It does not carry the byte-identity
guarantee this project holds itself to, and where the two disagree the
generated listing under `out/` is what has been verified against the ROM.
