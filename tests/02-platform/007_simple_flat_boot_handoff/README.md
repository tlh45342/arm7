# 007_simple_flat_boot_handoff

Fourth layer of the ARM7 SIMPLE-FLAT boot proof ladder.

This test proves execution handoff to a program that exists only as
`BOOT.BIN` inside a `flatdisk`-generated SIMPLE-FLAT image.

The loader at 0x00008000:

1. reads the SIMPLE-FLAT header;
2. locates BOOT.BIN in the directory;
3. loads its two sectors at 0x00010000;
4. branches to 0x00010000.

BOOT.BIN is a real ARM executable linked for 0x00010000. It writes unique
markers at 0x00030C00, records its own linked address, and executes BKPT.

The host verifies both the loader-side handoff evidence and the markers
written only by disk-loaded BOOT code.

Run:

    make -C tests\02-platform\007_simple_flat_boot_handoff test
