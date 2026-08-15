# ARM7 BIOS v0.0.1

First bare-metal BIOS layer for ARM7.

Current contract:

- reset vector at `0x00000000`
- stack established at `0x001FF000`
- 80x25 text CRT at `0x0A000000`
- BIOS clears the CRT
- BIOS prints `ARM7 BIOS v0.0.1`
- BIOS transfers control to `BOOT.BIN` at `0x00010000`

The BIOS does not know about disks or filesystems yet. A platform/runner is
responsible for placing BOOT.BIN at the fixed boot address.

`make test` uses a tiny test-only boot stub at `0x00010000` to prove the handoff.
That stub is not the future real bootloader.
