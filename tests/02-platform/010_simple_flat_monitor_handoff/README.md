# 010_simple_flat_monitor_handoff

Third and final isolated test in the BOOT -> MONITOR half of the boot ladder.

The test creates a real ARM `MONITOR.BIN` linked at `0x00020000` and pads it
to 1148 bytes, requiring three sectors.

The loader guest:

1. parses SIMPLE-FLAT;
2. finds MONITOR.BIN;
3. loads all three sectors at 0x00020000;
4. branches to 0x00020000.

The disk-loaded monitor then:

- writes unique execution markers into RAM;
- records its own linked address;
- writes `DISK-LOADED MONITOR OK` to the CRT;
- executes BKPT from the MONITOR region.

Run:

    make -C tests\02-platform\010_simple_flat_monitor_handoff test

If this passes, tests 008-010 form the proven reference implementation for
the real firmware/boot MONITOR loader.
